/**
 * @file alarm_clock.hpp
 * @author Sinter Wong (sintercver@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-02-25
 *
 * @copyright Copyright (c) 2025
 *
 */
#ifndef __UTILS_ALARM_CLOCK_HPP__
#define __UTILS_ALARM_CLOCK_HPP__
#include <condition_variable>
#include <mutex>
#include <thread>

namespace utils {

class AlarmClock {
public:
  AlarmClock(int initialDelaySeconds)
      : continueCountdown(true), nextDelaySeconds(initialDelaySeconds),
        stopped(false), timerExpired(false) {}

  void start() { timerThread = std::thread(&AlarmClock::timerLoop, this); }

  void stop() {
    {
      std::unique_lock<std::mutex> lock(mtx);
      stopped = true;
      timerExpired = true;
      cv.notify_all();
    }
    if (timerThread.joinable()) {
      timerThread.join();
    }
  }

  void setNextAction(bool continueC, int delaySeconds) {
    std::unique_lock<std::mutex> lock(mtx);
    continueCountdown = continueC;
    if (continueC) {
      nextDelaySeconds = delaySeconds;
      timerExpired = false;
    }
    stopped = false;
    cv.notify_all();
  }

  void waitForNotification() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this] { return timerExpired || stopped; });
  }

  bool isRunning() const {
    std::lock_guard<std::mutex> lock(mtx);
    return !stopped;
  }

  ~AlarmClock() { stop(); }

private:
  void timerLoop() {
    while (!stopped) {
      std::unique_lock<std::mutex> lock(mtx);
      if (continueCountdown) {
        if (cv.wait_for(lock, std::chrono::seconds(nextDelaySeconds),
                        [this] { return stopped; })) {
          break;
        }

        if (!stopped) {
          timerExpired = true;
          continueCountdown = false;
          cv.notify_all();
        }
      } else {
        cv.wait(lock, [this] { return continueCountdown || stopped; });
        timerExpired = false;
      }
    }
  }

  mutable std::mutex mtx;
  std::condition_variable cv;
  bool continueCountdown;
  int nextDelaySeconds;
  std::thread timerThread;
  bool stopped;
  bool timerExpired;
};

} // namespace utils

#endif