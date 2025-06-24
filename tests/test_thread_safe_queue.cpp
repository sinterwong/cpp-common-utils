#include "gtest/gtest.h"
#include "thread_safe_queue.hpp"
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>

// --- Tests for ThreadSafeQueue ---
class ThreadSafeQueueTest : public ::testing::Test {
protected:
    common_utils::ThreadSafeQueue<int> queue;
};

TEST_F(ThreadSafeQueueTest, PushAndTryPopSingleThread) {
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);

    queue.push(1);
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 1);

    auto val = queue.try_pop();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), 1);
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
}

TEST_F(ThreadSafeQueueTest, TryPopEmpty) {
    auto val = queue.try_pop();
    EXPECT_FALSE(val.has_value());
}

TEST_F(ThreadSafeQueueTest, WaitPopSingleThread) {
    queue.push(10);
    int val = queue.wait_pop();
    EXPECT_EQ(val, 10);
    EXPECT_TRUE(queue.empty());
}

TEST_F(ThreadSafeQueueTest, WaitPopForSingleThread) {
    queue.push(20);
    auto val = queue.wait_pop_for(std::chrono::milliseconds(100));
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), 20);
    EXPECT_TRUE(queue.empty());
}

TEST_F(ThreadSafeQueueTest, WaitPopForTimeout) {
    auto val = queue.wait_pop_for(std::chrono::milliseconds(10));
    EXPECT_FALSE(val.has_value());
}

TEST_F(ThreadSafeQueueTest, ClearQueue) {
    queue.push(1);
    queue.push(2);
    ASSERT_EQ(queue.size(), 2);
    queue.clear();
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
    auto val = queue.try_pop();
    EXPECT_FALSE(val.has_value());
}

TEST_F(ThreadSafeQueueTest, MultipleProducersSingleConsumer) {
    std::vector<std::thread> producers;
    const int items_per_producer = 50;
    const int num_producers = 4;
    std::atomic<int> sum_produced(0);

    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&, i]() {
            for (int j = 0; j < items_per_producer; ++j) {
                int val = i * items_per_producer + j;
                queue.push(val);
                sum_produced += val;
            }
        });
    }

    std::atomic<int> sum_consumed(0);
    int items_consumed = 0;
    const int total_items = num_producers * items_per_producer;
    while (items_consumed < total_items) {
        auto val = queue.wait_pop_for(std::chrono::milliseconds(200)); // Wait with timeout
        if (val.has_value()) {
            sum_consumed += val.value();
            items_consumed++;
        }
    }

    for (auto& t : producers) {
        t.join();
    }

    EXPECT_EQ(items_consumed, total_items);
    EXPECT_EQ(sum_consumed.load(), sum_produced.load());
    EXPECT_TRUE(queue.empty());
}

TEST_F(ThreadSafeQueueTest, SingleProducerMultipleConsumers) {
    const int total_items = 200;
    std::atomic<int> sum_produced(0);

    std::thread producer([&]() {
        for (int i = 0; i < total_items; ++i) {
            queue.push(i);
            sum_produced += i;
        }
    });

    std::vector<std::thread> consumers;
    const int num_consumers = 4;
    std::atomic<int> items_consumed_count(0);
    std::atomic<int> sum_consumed(0);

    for (int i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([&]() {
            while(items_consumed_count.load() < total_items) {
                auto val = queue.try_pop(); // Consumers actively try_pop
                if (val.has_value()) {
                    sum_consumed += val.value();
                    items_consumed_count++;
                } else if (items_consumed_count.load() >= total_items && queue.empty()) {
                    // All items potentially consumed by other threads and queue is empty
                    break;
                }
                std::this_thread::yield(); // Give other threads a chance
            }
        });
    }

    producer.join();
    // Ensure producer has finished before checking consumer counts, consumers might still be running.
    // Wait for consumers to finish or a timeout
    for (auto& t : consumers) {
       if(t.joinable()) t.join(); //This might hang if consumers don't exit, try_pop logic is key
    }

    // Final check after all threads attempt to join
    // It's possible some consumers might exit early if queue appears empty and total count is met.
    // The crucial part is that all items are consumed *eventually*.
    // A more robust check might involve waiting until items_consumed_count == total_items with a timeout.
    // For now, let's assume they complete. If this test becomes flaky, consumer loop needs refinement.

    // Spin-wait for consumers to complete processing, with a timeout
    auto start_time = std::chrono::steady_clock::now();
    while (items_consumed_count.load() < total_items &&
           std::chrono::steady_clock::now() - start_time < std::chrono::seconds(5)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }


    EXPECT_EQ(items_consumed_count.load(), total_items);
    EXPECT_EQ(sum_consumed.load(), sum_produced.load());
    EXPECT_TRUE(queue.empty());
}


// --- Tests for ThreadSafePriorityQueue ---
struct CustomType {
    int priority;
    std::string data;

    // For max-heap (std::priority_queue default)
    bool operator<(const CustomType& other) const {
        return priority < other.priority;
    }
     // For min-heap (if std::greater is used)
    bool operator>(const CustomType& other) const {
        return priority > other.priority;
    }
};

class ThreadSafePriorityQueueTest : public ::testing::Test {
protected:
    // Max-heap by default
    common_utils::ThreadSafePriorityQueue<CustomType> pq_max;
    // Min-heap
    common_utils::ThreadSafePriorityQueue<CustomType, std::greater<CustomType>> pq_min;
    common_utils::ThreadSafePriorityQueue<int> pq_int_max; // Default less<int> -> max-heap
};

TEST_F(ThreadSafePriorityQueueTest, PushAndTryPopSingleThreadMaxHeap) {
    pq_max.push({1, "low"});
    pq_max.push({3, "high"});
    pq_max.push({2, "mid"});

    ASSERT_EQ(pq_max.size(), 3);
    auto val1 = pq_max.try_pop();
    ASSERT_TRUE(val1.has_value());
    EXPECT_EQ(val1.value().priority, 3); // Max priority

    auto val2 = pq_max.try_pop();
    ASSERT_TRUE(val2.has_value());
    EXPECT_EQ(val2.value().priority, 2);

    auto val3 = pq_max.try_pop();
    ASSERT_TRUE(val3.has_value());
    EXPECT_EQ(val3.value().priority, 1);

    EXPECT_TRUE(pq_max.empty());
}

TEST_F(ThreadSafePriorityQueueTest, PushAndTryPopSingleThreadMinHeap) {
    pq_min.push({1, "low"});
    pq_min.push({3, "high"});
    pq_min.push({2, "mid"});

    ASSERT_EQ(pq_min.size(), 3);
    auto val1 = pq_min.try_pop();
    ASSERT_TRUE(val1.has_value());
    EXPECT_EQ(val1.value().priority, 1); // Min priority

    auto val2 = pq_min.try_pop();
    ASSERT_TRUE(val2.has_value());
    EXPECT_EQ(val2.value().priority, 2);

    auto val3 = pq_min.try_pop();
    ASSERT_TRUE(val3.has_value());
    EXPECT_EQ(val3.value().priority, 3);

    EXPECT_TRUE(pq_min.empty());
}

TEST_F(ThreadSafePriorityQueueTest, TryPopEmpty) {
    auto val = pq_int_max.try_pop();
    EXPECT_FALSE(val.has_value());
}

TEST_F(ThreadSafePriorityQueueTest, WaitPopForTimeout) {
    auto val = pq_int_max.wait_pop_for(std::chrono::milliseconds(10));
    EXPECT_FALSE(val.has_value());
}

TEST_F(ThreadSafePriorityQueueTest, ClearQueue) {
    pq_int_max.push(10);
    pq_int_max.push(20);
    ASSERT_EQ(pq_int_max.size(), 2);
    pq_int_max.clear();
    EXPECT_TRUE(pq_int_max.empty());
    EXPECT_EQ(pq_int_max.size(), 0);
}
