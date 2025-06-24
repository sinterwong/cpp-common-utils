#include "gtest/gtest.h"
#include "time_utils.hpp"
#include <thread>
#include <chrono>
#include <atomic>
#include <functional> // For std::ref

// Test for getCurrentTimestamp
TEST(TimeUtilsTest, GetCurrentTimestamp) {
    long long ts1 = common_utils::getCurrentTimestamp();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    long long ts2 = common_utils::getCurrentTimestamp();

    EXPECT_GT(ts2, ts1);
    // Check if the difference is roughly in the expected range (e.g., 40-150ms to account for sleep inaccuracies and overhead)
    // This is a sanity check, not a precise timing test.
    EXPECT_GE(ts2 - ts1, 40);
    EXPECT_LT(ts2 - ts1, 250); // Increased upper bound for CI environments
}

// Test for measureTime
void dummy_function_for_measure(int sleep_ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
}

int dummy_function_with_return(int val) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return val * 2;
}

TEST(TimeUtilsTest, MeasureTime) {
    int sleep_duration_ms = 50;
    long long measured_us = common_utils::measureTime(dummy_function_for_measure, sleep_duration_ms);

    // Expected time in microseconds
    long long expected_us_min = sleep_duration_ms * 1000;
    // Allow for some overhead, but it shouldn't be drastically more or less
    // The actual measured time can be noisy.
    EXPECT_GE(measured_us, expected_us_min * 0.9);
    EXPECT_LT(measured_us, expected_us_min * 2.5); // Increased upper bound

    // Test with a function that returns a value and takes args
    // Note: measureTime itself doesn't return the function's result, only time.
    measured_us = common_utils::measureTime(dummy_function_with_return, 5);
    EXPECT_GE(measured_us, 10000 * 0.9); // 10ms
    EXPECT_LT(measured_us, 10000 * 3.0); // Increased upper bound
}


// Test for periodicTask (controlled execution for testing)
// We need a way to stop the periodic task for the test to finish.
// We'll pass an atomic flag or a counter.

std::atomic<int> periodic_counter(0);
std::atomic<bool> stop_periodic_task_flag(false);

void task_to_be_called_periodically(std::atomic<int>& counter) {
    counter++;
}

// This version of periodicTask is modified for testing: it runs for a limited number of iterations.
template <typename F, typename... Args>
inline void periodicTaskTestWrapper(std::chrono::milliseconds interval, int max_executions, F &&f,
                         Args &&...args) {
  auto lastExecTime = std::chrono::steady_clock::now();
  int executions = 0;
  while (executions < max_executions) {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - lastExecTime);
    if (elapsed >= interval) {
      std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
      lastExecTime = now; // Reset lastExecTime to now after execution
      executions++;
    }
    // Short sleep to prevent busy waiting and allow time to advance
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}


TEST(TimeUtilsTest, PeriodicTaskControlled) {
    periodic_counter.store(0);
    int max_calls = 3;
    std::chrono::milliseconds interval(50ms);

    // Run the test wrapper in a separate thread because it blocks
    std::thread t(periodicTaskTestWrapper<void (*)(std::atomic<int>&), std::atomic<int>&>,
                  interval, max_calls, task_to_be_called_periodically, std::ref(periodic_counter));

    // Wait for the thread to complete. Max duration should be roughly max_calls * interval + some overhead.
    // If it takes too long, the test will time out via test runner's global timeout.
    t.join();

    EXPECT_EQ(periodic_counter.load(), max_calls);
}

TEST(TimeUtilsTest, PeriodicTaskTimingSanity) {
    periodic_counter.store(0);
    int max_calls = 2;
    std::chrono::milliseconds interval(100ms); // 100 ms interval

    auto test_start_time = std::chrono::steady_clock::now();

    std::thread t(periodicTaskTestWrapper<void (*)(std::atomic<int>&), std::atomic<int>&>,
                  interval, max_calls, task_to_be_called_periodically, std::ref(periodic_counter));
    t.join();

    auto test_end_time = std::chrono::steady_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(test_end_time - test_start_time);

    EXPECT_EQ(periodic_counter.load(), max_calls);
    // Expected duration is roughly (max_calls -1) * interval because the first call happens almost immediately
    // if the condition is met, and the loop exits after the last call.
    // Or, more accurately, the time until the *last* execution completes.
    // The loop runs `max_calls` times. The first call is after ~interval, second after ~2*interval etc.
    // So total time is roughly max_calls * interval.
    long long expected_min_duration_ms = max_calls * interval.count();

    // Allow for some reasonable deviation due to thread scheduling and sleep inaccuracies.
    // The first execution might happen very quickly if elapsed >= interval is met on first check.
    // The test wrapper's logic: it calls, then updates lastExecTime.
    // So, for 2 calls with 100ms interval:
    // Call 1: after ~100ms. lastExecTime updated.
    // Call 2: after another ~100ms from previous call. lastExecTime updated. Loop ends.
    // Total time ~200ms.
    EXPECT_GE(total_duration.count(), expected_min_duration_ms * 0.80);
    EXPECT_LT(total_duration.count(), expected_min_duration_ms * 2.5); // Generous upper bound
}
