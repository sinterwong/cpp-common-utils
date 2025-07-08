/**
 * @file thread_safe_slot.hpp
 * @author Sinter Wong (sintercver@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-07-08
 *
 * @copyright Copyright (c) 2025
 *
 */
#ifndef __THREAD_SAFE_SLOT_HPP__
#define __THREAD_SAFE_SLOT_HPP__

#include <condition_variable>
#include <mutex>
#include <optional>

namespace common_utils {
template <typename T> class ThreadSafeSlot {
public:
  ThreadSafeSlot() = default;

  ThreadSafeSlot(const ThreadSafeSlot &) = delete;
  ThreadSafeSlot &operator=(const ThreadSafeSlot &) = delete;

  void set(T newValue) {
    {
      std::lock_guard lk(mutex_);
      value_ = std::move(newValue);
      hasNewValueSinceLastGet_ = true;
    }
    cv_.notify_one();
  }

  std::optional<T> wait_and_get() {
    std::unique_lock<std::mutex> lk(mutex_);

    //  there is a new value or a stop signal has been received
    cv_.wait(lk, [this] { return hasNewValueSinceLastGet_ || isStopped_; });

    if (isStopped_ && !hasNewValueSinceLastGet_) {
      return std::nullopt;
    }

    hasNewValueSinceLastGet_ = false;
    return std::move(value_);
  }

  template <class Rep, class Period>
  std::optional<T>
  wait_and_get_for(const std::chrono::duration<Rep, Period> &timeout) {
    std::unique_lock<std::mutex> lk(mutex_);

    if (!cv_.wait_for(lk, timeout, [this] {
          return hasNewValueSinceLastGet_ || isStopped_;
        })) {
      return std::nullopt;
    }
    if (isStopped_ && !hasNewValueSinceLastGet_) {
      return std::nullopt;
    }

    hasNewValueSinceLastGet_ = false;
    return std::move(value_);
  }

  std::optional<T> try_get() {
    std::unique_lock<std::mutex> lk(mutex_);
    if (!hasNewValueSinceLastGet_) {
      return std::nullopt;
    }
    hasNewValueSinceLastGet_ = false;
    return std::move(value_);
  }

  void stop() {
    {
      std::lock_guard<std::mutex> lk(mutex_);
      isStopped_ = true;
    }
    cv_.notify_all();
  }

  bool is_stopped() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return isStopped_;
  }

  void reset() {
    std::lock_guard<std::mutex> lk(mutex_);
    value_.reset();
    isStopped_ = false;
    hasNewValueSinceLastGet_ = false;
  }

private:
  mutable std::mutex mutex_;
  std::condition_variable cv_;
  std::optional<T> value_;
  bool isStopped_ = false;
  bool hasNewValueSinceLastGet_ = false;
};

} // namespace common_utils

#endif