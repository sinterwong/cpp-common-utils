/**
 * @file topk_heap.hpp
 * @author Sinter Wong (sintercver@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-03-18
 *
 * @copyright Copyright (c) 2025
 *
 */
#ifndef _TOP_K_HEAP_HPP__
#define _TOP_K_HEAP_HPP__

#include <algorithm>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <utility>
#include <vector>

namespace utils {
template <typename T, typename Compare = std::greater<T>> class TopKHeap {
private:
  size_t capacity_ = 15;
  std::priority_queue<T, std::vector<T>, Compare> heap;
  mutable std::unique_ptr<std::mutex> mtx_ptr;

private:
private:
  void push_no_lock(const T &item) {
    heap.push(item);
    if (heap.size() > capacity_) {
      heap.pop();
    }
  }

public:
  TopKHeap() : mtx_ptr(std::make_unique<std::mutex>()) {}

  explicit TopKHeap(size_t capacity)
      : capacity_(capacity), mtx_ptr(std::make_unique<std::mutex>()) {}

  explicit TopKHeap(const Compare &comp, size_t capacity = 15)
      : capacity_(capacity), heap(comp),
        mtx_ptr(std::make_unique<std::mutex>()) {}

  TopKHeap(const TopKHeap &other) : mtx_ptr(std::make_unique<std::mutex>()) {
    std::lock_guard<std::mutex> lock(*other.mtx_ptr);
    capacity_ = other.capacity_;
    heap = other.heap;
  }

  TopKHeap &operator=(const TopKHeap &other) {
    if (this != &other) {
      const TopKHeap *first = this < &other ? this : &other;
      const TopKHeap *second = this < &other ? &other : this;

      std::lock_guard<std::mutex> lock_first(*first->mtx_ptr);
      std::lock_guard<std::mutex> lock_second(*second->mtx_ptr);

      capacity_ = other.capacity_;
      heap = other.heap;
    }
    return *this;
  }

  TopKHeap(TopKHeap &&other) noexcept
      : capacity_(other.capacity_), heap(std::move(other.heap)),
        mtx_ptr(std::move(other.mtx_ptr)) {}

  TopKHeap &operator=(TopKHeap &&other) noexcept {
    if (this != &other) {
      capacity_ = other.capacity_;
      heap = std::move(other.heap);
      mtx_ptr = std::move(other.mtx_ptr);
    }
    return *this;
  }

  void setCapacity(size_t capacity) {
    std::lock_guard<std::mutex> lock(*mtx_ptr);
    capacity_ = capacity;

    while (heap.size() > capacity_) {
      heap.pop();
    }
  }

  size_t capacity() const {
    std::lock_guard<std::mutex> lock(*mtx_ptr);
    return capacity_;
  }

  bool full() const {
    std::lock_guard<std::mutex> lock(*mtx_ptr);
    return heap.size() >= capacity_;
  }

  void push(const T &item) {
    std::lock_guard<std::mutex> lock(*mtx_ptr);
    push_no_lock(item);
  }

  void push_many(const std::vector<T> &items) {
    std::lock_guard<std::mutex> lock(*mtx_ptr);
    for (const auto &item : items) {
      push_no_lock(item);
    }
  }

  std::vector<T> getTopK() {
    std::lock_guard<std::mutex> lock(*mtx_ptr);

    std::vector<T> result;
    result.reserve(heap.size());

    auto tempHeap = heap;

    while (!tempHeap.empty()) {
      result.push_back(tempHeap.top());
      tempHeap.pop();
    }

    std::reverse(result.begin(), result.end());
    return result;
  }

  size_t size() const {
    std::lock_guard<std::mutex> lock(*mtx_ptr);
    return heap.size();
  }

  bool empty() const {
    std::lock_guard<std::mutex> lock(*mtx_ptr);
    return heap.empty();
  }

  void clear() {
    std::lock_guard<std::mutex> lock(*mtx_ptr);
    while (!heap.empty()) {
      heap.pop();
    }
  }

  std::optional<T> top() const {
    std::lock_guard<std::mutex> lock(*mtx_ptr);
    if (heap.empty()) {
      return std::nullopt;
    }
    return heap.top();
  }

  void reserve(size_t capacity) {
    std::lock_guard<std::mutex> lock(*mtx_ptr);
    std::vector<T> temp;
    temp.reserve(capacity);

    auto tempHeap = heap;
    while (!tempHeap.empty()) {
      temp.push_back(tempHeap.top());
      tempHeap.pop();
    }
    heap = std::priority_queue<T, std::vector<T>, Compare>(Compare(),
                                                           std::move(temp));
  }
};

} // namespace utils
#endif