/**
 * @file delay_action_trigger.hpp
 * @author Sinter Wong (sintercver@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-04-21
 *
 * @copyright Copyright (c) 2025
 *
 */
#ifndef __DELAYED_ACTION_TRIGGER_HPP__
#define __DELAYED_ACTION_TRIGGER_HPP__
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

namespace utils {
class DelayedActionTrigger {
public:
  using ActionCallback = std::function<void()>;

  explicit DelayedActionTrigger(ActionCallback onTimeout)
      : actionCallback_(std::move(onTimeout)), running_(false),
        stopRequested_(false) {}

  ~DelayedActionTrigger() { stop(); }

  DelayedActionTrigger(const DelayedActionTrigger &) = delete;
  DelayedActionTrigger &operator=(const DelayedActionTrigger &) = delete;
  DelayedActionTrigger(DelayedActionTrigger &&) = delete;
  DelayedActionTrigger &operator=(DelayedActionTrigger &&) = delete;

  void start(int delaySeconds) {
    // stop the timer that might be running at present first
    stop();

    std::unique_lock<std::mutex> lock(mutex_);

    running_ = true;
    stopRequested_ = false;
    timerThread_ = std::thread(&DelayedActionTrigger::run, this,
                               std::chrono::seconds(delaySeconds));
  }

  void stop() {
    std::unique_lock<std::mutex> lock(mutex_);
    stopInternal(lock);

    lock.unlock();

    if (timerThread_.joinable()) {
      timerThread_.join();
    }
  }

  bool isRunning() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return running_;
  }

private:
  void run(std::chrono::seconds delay) {
    std::unique_lock<std::mutex> lock(mutex_);

    // wait for the delay or until stop()
    if (!cv_.wait_for(lock, delay, [this] { return stopRequested_; })) {
      if (actionCallback_) {
        lock.unlock();
        try {
          actionCallback_();
        } catch (const std::exception &e) {
          std::cerr << "Exception in action callback: " << e.what()
                    << std::endl;
          ;
        } catch (...) {
          std::cerr << "Unknown exception in action callback" << std::endl;
        }
        lock.lock();
      }
    }
    running_ = false;
  }

  void stopInternal(std::unique_lock<std::mutex> &lock) {
    // need to be invoked when the lock is held
    if (!running_) {
      return;
    }
    stopRequested_ = true;
    cv_.notify_one();
  }

private:
  ActionCallback actionCallback_;
  std::thread timerThread_;
  mutable std::mutex mutex_;
  std::condition_variable cv_;
  bool running_;
  bool stopRequested_;
};
} // namespace utils

#endif