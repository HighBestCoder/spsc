#include <algorithm>
#ifndef _PERF_TEST_CHAN_SOFT_ARRAY_H_
#define _PERF_TEST_CHAN_SOFT_ARRAY_H_

#include <atomic>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include <iostream>
#include <new>
#include <cstdint>

template <typename T, uint32_t kCacheLineSize = 64>
class SPSCQueueSoftArray {
 public:
  // 使用placement new创建SPSC队列
  static SPSCQueueSoftArray* create(const int cap) noexcept {
    int actual_cap = std::max<int>(cap + 1, 4);
    
    // 计算需要的总内存大小
    size_t base_size = sizeof(SPSCQueueSoftArray);
    size_t array_size = sizeof(T) * actual_cap;
    size_t total_size = base_size + array_size;
    
    // 分配内存，确保对齐
    void* raw_memory = operator new(total_size, std::align_val_t(kCacheLineSize));
    if (!raw_memory) {
      return nullptr;
    }
    
    // 使用placement new构造对象
    SPSCQueueSoftArray* queue = new(raw_memory) SPSCQueueSoftArray(actual_cap);
    return queue;
  }
  
  // 自定义删除函数
  static void destroy(SPSCQueueSoftArray* queue) noexcept {
    if (queue) {
      // 清空所有元素
      while (queue->front()) {
        queue->pop();
      }
      
      // 调用析构函数
      queue->~SPSCQueueSoftArray();
      
      // 释放内存
      operator delete(queue, std::align_val_t(kCacheLineSize));
    }
  }

  template <typename... Args>
  bool push(Args &&...args) noexcept {
    auto const head = head_.load(std::memory_order_relaxed);
    auto next_head = head + 1;
    if (next_head == cap_) {
      next_head = 0;
    }

    // 如果队列满了，忙等待
    while (next_head == tail_.load(std::memory_order_acquire)) {
      // nothing
    }

    // 使用placement new构造元素
    new (&buf_[head]) T(std::forward<Args>(args)...);
    head_.store(next_head, std::memory_order_release);
    return true;
  }

  T *front() noexcept {
    auto tail = tail_.load(std::memory_order_relaxed);
    if (head_.load(std::memory_order_acquire) == tail) {
      return nullptr;
    }
    return &buf_[tail];
  }

  void pop() noexcept {
    auto tail = tail_.load(std::memory_order_relaxed);
    auto next_tail = tail + 1;
    if (next_tail == cap_) {
      next_tail = 0;
    }
    buf_[tail].~T();
    tail_.store(next_tail, std::memory_order_release);
  }

  size_t size() const noexcept {
    int head = head_.load(std::memory_order_acquire);
    int tail = tail_.load(std::memory_order_acquire);
    int diff = head - tail;
    if (diff < 0) {
      diff += cap_;
    }
    return static_cast<size_t>(diff);
  }

  int capacity() const noexcept {
    return cap_;
  }

 private:
  // 私有构造函数，只能通过create方法创建
  explicit SPSCQueueSoftArray(const int cap) noexcept : cap_(cap) {}
  
  // 私有析构函数，只能通过destroy方法销毁
  ~SPSCQueueSoftArray() = default;

  // 禁止拷贝和移动
  SPSCQueueSoftArray(const SPSCQueueSoftArray&) = delete;
  SPSCQueueSoftArray& operator=(const SPSCQueueSoftArray&) = delete;
  SPSCQueueSoftArray(SPSCQueueSoftArray&&) = delete;
  SPSCQueueSoftArray& operator=(SPSCQueueSoftArray&&) = delete;

  alignas(kCacheLineSize) int cap_ = 0;
  alignas(kCacheLineSize) std::atomic<int> head_{0};
  alignas(kCacheLineSize) std::atomic<int> tail_{0};
  alignas(kCacheLineSize) T buf_[0];  // 柔性数组成员
};

#endif  // _PERF_TEST_CHAN_SOFT_ARRAY_H_