#include "gtest/gtest.h"
#include "thread_pool.hpp"
#include <future>
#include <chrono>
#include <atomic>
#include <vector>

// Helper function for simple tasks
int simple_task_no_args() {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return 42;
}

int task_with_arg(int x) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return x * 2;
}

void void_task(std::atomic<int>& counter) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    counter++;
}

class ThreadPoolTest : public ::testing::Test {
protected:
    common_utils::thread_pool pool;

    void SetUp() override {
        // Default to 2 threads for most tests, can be overridden
        pool.start(2);
    }

    void TearDown() override {
        pool.stop();
    }
};

TEST_F(ThreadPoolTest, StartAndStop) {
    // Pool is started in SetUp, stop it
    pool.stop();
    // Restart with a different number of threads
    pool.start(4);
    // Submit a simple task to ensure it's running
    auto future = pool.submit(simple_task_no_args);
    EXPECT_EQ(future.get(), 42);
}

TEST_F(ThreadPoolTest, SubmitTaskNoArgs) {
    auto future = pool.submit(simple_task_no_args);
    EXPECT_EQ(future.get(), 42);
}

TEST_F(ThreadPoolTest, SubmitTaskWithArgs) {
    auto future = pool.submit(task_with_arg, 21);
    EXPECT_EQ(future.get(), 42);
}

TEST_F(ThreadPoolTest, SubmitVoidTask) {
    std::atomic<int> counter(0);
    auto future = pool.submit(void_task, std::ref(counter));
    future.get(); // Wait for task to complete
    EXPECT_EQ(counter.load(), 1);
}

TEST_F(ThreadPoolTest, SubmitMultipleTasks) {
    const int num_tasks = 10;
    std::vector<std::future<int>> futures;
    for (int i = 0; i < num_tasks; ++i) {
        futures.push_back(pool.submit(task_with_arg, i));
    }

    for (int i = 0; i < num_tasks; ++i) {
        EXPECT_EQ(futures[i].get(), i * 2);
    }
}

TEST_F(ThreadPoolTest, StopWithPendingTasks) {
    pool.stop(); // Stop the default pool
    pool.start(1); // Start with 1 thread for controlled testing

    std::atomic<int> counter(0);
    // Submit a task that takes a bit longer
    auto slow_task = [&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        counter++;
        return true;
    };
    pool.submit(slow_task);

    // Submit more tasks that might not get to run if stop is too quick
    for (int i = 0; i < 5; ++i) {
        pool.submit([&]() { counter++; });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Give some time for tasks to queue
    pool.stop(); // Stop should wait for running tasks and clear the queue

    // The exact number of tasks completed can vary depending on timing and how stop clears the queue.
    // The current thread_pool implementation's stop() waits for threads to join,
    // implying running tasks complete. The queue is then cleared.
    // So, at least the slow_task should have run if it started.
    // Given the sleeps, it is highly probable the slow_task has started.
    // The behavior of tasks in queue that haven't started depends on the pool's stop logic.
    // The provided pool clears the queue *after* joining threads.
    // This test mainly ensures stop() works without deadlocking or crashing with tasks in flight/queue.
    EXPECT_GE(counter.load(), 1) << "At least the initially submitted slow task should have run.";
    // We cannot deterministically say how many of the fast tasks ran.
}


TEST_F(ThreadPoolTest, SubmitToStoppedPoolThrows) {
    pool.stop();
    EXPECT_THROW(pool.submit(simple_task_no_args), std::runtime_error);
}

TEST_F(ThreadPoolTest, SubmitToStoppingPoolThrows) {
    // This test is tricky due to the timing of the state change.
    // We'll try to submit while the pool is in the process of stopping.
    pool.stop(); // Stop the default pool
    pool.start(1);

    // Submit a task that blocks one thread
    std::promise<void> block_promise;
    std::future<void> block_future = block_promise.get_future();
    pool.submit([&]() { block_future.wait(); });

    // In a separate thread, initiate stop
    std::thread stopper([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // give submit a chance
        pool.stop();
    });

    // Try to submit tasks. Some might go through, some might throw.
    // The goal is to observe the throw when state is STOPPING or STOPPED.
    bool threw = false;
    for (int i = 0; i < 100; ++i) { // Try submitting many times
        try {
            pool.submit(simple_task_no_args);
            std::this_thread::sleep_for(std::chrono::microseconds(100)); // Small delay
        } catch (const std::runtime_error& e) {
            std::string msg = e.what();
            if (msg == "ThreadPool is not running" || msg == "ThreadPool is stopping") {
                threw = true;
                break;
            }
        }
    }
    block_promise.set_value(); // Unblock the worker
    stopper.join();
    EXPECT_TRUE(threw) << "Expected a runtime_error when submitting to a stopping/stopped pool.";
}


// Test for queue full scenario
class ThreadPoolQueueFullTest : public ::testing::Test {
protected:
    common_utils::thread_pool pool; // Max queue size 1 by default in this pool
};

TEST_F(ThreadPoolQueueFullTest, SubmitToFullQueueThrowsAfterTimeout) {
    // The provided thread_pool has a max_queue_size_ constructor argument.
    // The default is 1024. Let's create one with a small queue for testing.
    common_utils::thread_pool small_queue_pool(1); // Max 1 item in queue
    small_queue_pool.start(1); // 1 worker thread

    std::promise<void> task1_can_finish_promise;
    std::future<void> task1_can_finish_future = task1_can_finish_promise.get_future();

    // Task 1: Occupies the worker thread
    small_queue_pool.submit([&]() {
        task1_can_finish_future.wait(); // Wait until told to finish
    });

    // Task 2: Occupies the queue (size 1)
    small_queue_pool.submit([]() { /* dummy task */ });

    // Task 3: Should cause a timeout/throw because queue is full
    // The submit has a 5-second timeout internally for not_full_ condition.
    // We expect this to throw std::runtime_error("Queue is full")
    bool threw_queue_full = false;
    try {
        small_queue_pool.submit([]() { /* this should not run */ });
    } catch (const std::runtime_error& e) {
        if (std::string(e.what()) == "Queue is full") {
            threw_queue_full = true;
        } else {
            FAIL() << "Expected 'Queue is full' exception, got: " << e.what();
        }
    } catch (...) {
        FAIL() << "Expected std::runtime_error for full queue";
    }

    task1_can_finish_promise.set_value(); // Allow task1 to finish
    small_queue_pool.stop();
    EXPECT_TRUE(threw_queue_full);
}
