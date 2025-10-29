#ifndef _PERF_TEST_CHAN_FENCE_H_
#define _PERF_TEST_CHAN_FENCE_H_

#include <stdexcept>
#include <type_traits>
#include <iostream>
#include <new>
#include <cstdint>

// 借用NanoLog的Fence实现
namespace Fence {
    // Load Fence: 防止load操作重排
    static void inline lfence() {
        __asm__ __volatile__("lfence" ::: "memory");
    }

    // Store Fence: 防止store操作重排
    static void inline sfence() {
        __asm__ __volatile__("sfence" ::: "memory");
    }

    // Memory Fence: 防止所有内存操作重排
    static void inline mfence() {
        __asm__ __volatile__("mfence" ::: "memory");
    }

    // Compiler Fence: 防止编译器重排，不影响CPU
    static void inline compiler_fence() {
        __asm__ __volatile__("" ::: "memory");
    }
}

template <typename T, uint32_t Capacity, uint32_t kCacheLineSize = 64>
class SPSCQueueFence {
public:
    // 使用placement new创建SPSC队列
    static SPSCQueueFence* create() noexcept {
        // 计算需要的总内存大小
        size_t total_size = sizeof(SPSCQueueFence);
        
        // 分配内存，确保对齐
        void* raw_memory = operator new(total_size, std::align_val_t(kCacheLineSize));
        if (!raw_memory) {
            return nullptr;
        }
        
        // 使用placement new构造对象
        SPSCQueueFence* queue = new(raw_memory) SPSCQueueFence();
        return queue;
    }
    
    // 自定义删除函数
    static void destroy(SPSCQueueFence* queue) noexcept {
        if (queue) {
            // 清空所有元素
            while (queue->front()) {
                queue->pop();
            }
            
            // 调用析构函数
            queue->~SPSCQueueFence();
            
            // 释放内存
            operator delete(queue, std::align_val_t(kCacheLineSize));
        }
    }

    template <typename... Args>
    bool push(Args &&...args) noexcept {
        // 读取当前head位置 (只有producer会写这个值，可以直接读取)
        const int head = head_;
        int next_head = head + 1;
        if (next_head == static_cast<int>(Capacity)) {
            next_head = 0;
        }

        // 检查队列是否满了
        // 需要读取consumer更新的tail值，使用lfence确保读取最新值
        int current_tail;
        do {
            Fence::lfence();  // 确保读取到最新的tail值
            current_tail = tail_;
            if (next_head != current_tail) {
                break;  // 队列未满，可以继续
            }
            // 队列满了，继续等待
            Fence::compiler_fence();  // 防止编译器优化掉循环
        } while (true);

        // 使用placement new构造元素
        new (&buf_[head]) T(std::forward<Args>(args)...);
        
        // 确保元素构造完成后再更新head指针
        // sfence确保数据写入在head更新之前完成
        Fence::sfence();
        head_ = next_head;
        
        return true;
    }

    T* front() noexcept {
        // 读取当前tail位置 (只有consumer会写这个值)
        const int tail = tail_;
        
        // 读取head位置，需要确保读取到producer最新的值
        Fence::lfence();  // 确保读取到最新的head值
        const int head = head_;
        
        if (head == tail) {
            return nullptr;  // 队列为空
        }
        return &buf_[tail];
    }

    void pop() noexcept {
        const int tail = tail_;
        int next_tail = tail + 1;
        if (next_tail == static_cast<int>(Capacity)) {
            next_tail = 0;
        }
        
        // 先析构元素
        buf_[tail].~T();
        
        // 确保析构完成后再更新tail指针
        // sfence确保析构操作在tail更新之前完成
        Fence::sfence();
        tail_ = next_tail;
    }

    size_t size() const noexcept {
        // 需要获取一致的快照，但对于SPSC来说可以简化
        Fence::lfence();  // 确保读取到最新值
        const int head = head_;
        const int tail = tail_;
        
        int diff = head - tail;
        if (diff < 0) {
            diff += static_cast<int>(Capacity);
        }
        return static_cast<size_t>(diff);
    }

    int capacity() const noexcept {
        return static_cast<int>(Capacity);
    }

    // 获取队列类型名称，用于测试识别
    static const char* queue_type() {
        return "SPSCQueueFence";
    }

private:
    // 私有构造函数，只能通过create方法创建
    SPSCQueueFence() noexcept 
        : head_(0), tail_(0) {
        // 缓冲区不需要初始化，等待placement new
    }
    
    // 私有析构函数，只能通过destroy方法销毁
    ~SPSCQueueFence() = default;

    // 禁止拷贝和移动
    SPSCQueueFence(const SPSCQueueFence&) = delete;
    SPSCQueueFence& operator=(const SPSCQueueFence&) = delete;
    SPSCQueueFence(SPSCQueueFence&&) = delete;
    SPSCQueueFence& operator=(SPSCQueueFence&&) = delete;

    // Producer端变量 (主要由producer线程访问)
    // 使用volatile确保每次都从内存读取，配合fence使用
    alignas(kCacheLineSize) volatile int head_;

    // Cache line分隔符，避免false sharing
    // 确保head_和tail_在不同的缓存行中
    char cache_line_spacer_[kCacheLineSize - sizeof(volatile int)];

    // Consumer端变量 (主要由consumer线程访问)
    alignas(kCacheLineSize) volatile int tail_;
    
    // 数据缓冲区，独立的缓存行
    alignas(kCacheLineSize) T buf_[Capacity];
};

#endif  // _PERF_TEST_CHAN_FENCE_H_