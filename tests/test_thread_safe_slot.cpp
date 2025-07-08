/**
 * @file test_thread_safe_slot.cpp
 * @author Sinter Wong (sintercver@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-07-08
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "thread_safe_slot.hpp"
#include "gtest/gtest.h"
#include <atomic>
#include <chrono>
#include <string>
#include <thread>

using namespace common_utils;

TEST(ThreadSafeSlotTest, SetAndGet) {
  ThreadSafeSlot<int> slot;
  slot.set(10);
  std::optional<int> val = slot.wait_and_get();
  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(val.value(), 10);
}

TEST(ThreadSafeSlotTest, TryGetEmpty) {
  ThreadSafeSlot<int> slot;
  std::optional<int> val = slot.try_get();
  EXPECT_FALSE(val.has_value());
}

TEST(ThreadSafeSlotTest, TryGetAfterSet) {
  ThreadSafeSlot<std::string> slot;
  slot.set("hello");
  std::optional<std::string> val = slot.try_get();
  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(val.value(), "hello");
  // After try_get, hasNewValueSinceLastGet_ should be false
  EXPECT_FALSE(slot.try_get().has_value());
}

TEST(ThreadSafeSlotTest, WaitAndGetBlocking) {
  ThreadSafeSlot<int> slot;
  std::atomic<bool> setter_started(false);
  std::atomic<bool> getter_finished(false);

  std::thread setter_thread([&]() {
    setter_started = true;
    std::this_thread::sleep_for(
        std::chrono::milliseconds(50)); // Give getter a chance to wait
    slot.set(20);
  });

  auto start_time = std::chrono::steady_clock::now();
  // Ensure setter thread has started before calling wait_and_get
  while (!setter_started) {
    std::this_thread::yield();
  }

  std::optional<int> val = slot.wait_and_get();
  auto end_time = std::chrono::steady_clock::now();

  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(val.value(), 20);
  EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                                  start_time)
                .count(),
            40);

  setter_thread.join();
}

TEST(ThreadSafeSlotTest, WaitAndGetForTimeout) {
  ThreadSafeSlot<int> slot;
  auto start_time = std::chrono::steady_clock::now();
  std::optional<int> val =
      slot.wait_and_get_for(std::chrono::milliseconds(100));
  auto end_time = std::chrono::steady_clock::now();

  EXPECT_FALSE(val.has_value());
  EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                                  start_time)
                .count(),
            90);
}

TEST(ThreadSafeSlotTest, WaitAndGetForSuccess) {
  ThreadSafeSlot<int> slot;
  std::atomic<bool> setter_started(false);

  std::thread setter_thread([&]() {
    setter_started = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    slot.set(30);
  });

  auto start_time = std::chrono::steady_clock::now();
  while (!setter_started) {
    std::this_thread::yield();
  }

  std::optional<int> val =
      slot.wait_and_get_for(std::chrono::milliseconds(500));
  auto end_time = std::chrono::steady_clock::now();

  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(val.value(), 30);
  EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                                  start_time)
                .count(),
            40);
  EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                                  start_time)
                .count(),
            500);

  setter_thread.join();
}

TEST(ThreadSafeSlotTest, StopSignal) {
  ThreadSafeSlot<int> slot;
  std::atomic<bool> getter_waiting(false);

  std::thread getter_thread([&]() {
    getter_waiting = true;
    std::optional<int> val = slot.wait_and_get();
    EXPECT_FALSE(val.has_value()); // Should be nullopt due to stop
  });

  // Wait until the getter thread is waiting
  while (!getter_waiting) {
    std::this_thread::yield();
  }

  slot.stop(); // Signal stop

  getter_thread.join();
  EXPECT_TRUE(slot.is_stopped());
}

TEST(ThreadSafeSlotTest, StopSignalWithPendingValue) {
  ThreadSafeSlot<int> slot;
  std::atomic<bool> getter_waiting(false);

  std::thread getter_thread([&]() {
    getter_waiting = true;
    std::optional<int> val = slot.wait_and_get();
    ASSERT_TRUE(val.has_value()); // Should get the value
    EXPECT_EQ(val.value(), 99);
  });

  // Wait until the getter thread is waiting
  while (!getter_waiting) {
    std::this_thread::yield();
  }

  slot.set(99); // Set a value
  slot.stop();  // Then signal stop

  getter_thread.join();
  EXPECT_TRUE(slot.is_stopped());
}

TEST(ThreadSafeSlotTest, ResetFunctionality) {
  ThreadSafeSlot<int> slot;
  slot.set(100);
  ASSERT_TRUE(slot.try_get().has_value());

  slot.stop();
  EXPECT_TRUE(slot.is_stopped());

  slot.reset();
  EXPECT_FALSE(slot.is_stopped());
  EXPECT_FALSE(slot.try_get().has_value()); // Should be empty after reset
}

TEST(ThreadSafeSlotTest, MultipleSetAndGet) {
  ThreadSafeSlot<int> slot;
  const int num_iterations = 100;

  for (int i = 0; i < num_iterations; ++i) {
    slot.set(i);
    std::optional<int> val = slot.wait_and_get();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), i);
  }
}

TEST(ThreadSafeSlotTest, IsStopped) {
  ThreadSafeSlot<int> slot;
  EXPECT_FALSE(slot.is_stopped());
  slot.stop();
  EXPECT_TRUE(slot.is_stopped());
  slot.reset();
  EXPECT_FALSE(slot.is_stopped());
}

TEST(ThreadSafeSlotTest, MoveSemanticsForValue) {
  ThreadSafeSlot<std::unique_ptr<int>> slot;
  auto ptr = std::make_unique<int>(123);
  int *raw_ptr = ptr.get();

  slot.set(std::move(ptr)); // Move into the slot

  auto retrieved_ptr = slot.wait_and_get(); // Move out of the slot
  ASSERT_TRUE(retrieved_ptr.has_value());
  ASSERT_NE(retrieved_ptr.value(), nullptr);
  EXPECT_EQ(*retrieved_ptr.value(), 123);
  EXPECT_EQ(retrieved_ptr.value().get(),
            raw_ptr); // Should be the same raw pointer
}

TEST(ThreadSafeSlotTest, WaitAndGetForStopSignal) {
  ThreadSafeSlot<int> slot;
  std::atomic<bool> getter_waiting(false);

  std::thread getter_thread([&]() {
    getter_waiting = true;
    std::optional<int> val = slot.wait_and_get_for(std::chrono::seconds(5));
    EXPECT_FALSE(val.has_value()); // Should be nullopt due to stop
  });

  // Wait until the getter thread is waiting
  while (!getter_waiting) {
    std::this_thread::yield();
  }

  slot.stop(); // Signal stop

  getter_thread.join();
  EXPECT_TRUE(slot.is_stopped());
}
