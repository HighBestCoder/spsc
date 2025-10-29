#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <cassert>
#include <iomanip>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/thread_policy.h>
#endif
#include "chan.h"
#include "chan_soft_array.h"

const uint64_t NUM_ELEMENTS = 1 << 20; // 1MB elements
const int QUEUE_SIZE = 1024; // Queue capacity
const int NUM_RUNS = 5; // Number of test runs for averaging

struct TestResult {
    double producer_throughput;
    double consumer_throughput;
    double overall_throughput;
    double bandwidth_mbps;
    uint64_t total_time_us;
};

std::atomic<bool> producer_done{false};

// 获取CPU核心数量
int get_cpu_count() {
    return static_cast<int>(std::thread::hardware_concurrency());
}

// 将当前线程绑定到指定CPU核心
bool pin_thread_to_cpu(int cpu_id) {
#ifdef __APPLE__
    // macOS的线程亲和性设置方式
    thread_affinity_policy_data_t policy = { cpu_id };
    thread_port_t mach_thread = pthread_mach_thread_np(pthread_self());
    kern_return_t result = thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY, 
                                            (thread_policy_t)&policy, 
                                            THREAD_AFFINITY_POLICY_COUNT);
    if (result != KERN_SUCCESS) {
        // 在macOS上，尝试使用processor sets (如果可用)
        return false;
    }
    return true;
#elif __linux__
    // Linux使用sched_setaffinity
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    return sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) == 0;
#else
    // 其他平台暂不支持
    (void)cpu_id; // 避免未使用参数警告
    return false;
#endif
}

// 尝试设置线程优先级来改善性能
bool set_thread_priority() {
#ifdef __APPLE__
    // 在macOS上设置更高的线程优先级
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    return pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) == 0;
#elif __linux__
    // 在Linux上设置实时优先级
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    return sched_setscheduler(0, SCHED_FIFO, &param) == 0;
#else
    return false;
#endif
}

// 打印线程绑定信息
void print_thread_info(const std::string& thread_name, int cpu_id) {
    std::cout << thread_name << " pinned to CPU " << cpu_id << std::endl;
}

// Test function for original SPSCQueue
template<typename QueueType>
TestResult test_original_queue(QueueType& queue) {
    producer_done.store(false);
    
    int cpu_count = get_cpu_count();
    int producer_cpu = 0;
    int consumer_cpu = (cpu_count > 1) ? 1 : 0;
    
    auto overall_start = std::chrono::high_resolution_clock::now();
    
    // Producer thread
    std::thread producer_thread([&queue, producer_cpu]() {
        // 尝试绑定到指定CPU
        bool pinned = pin_thread_to_cpu(producer_cpu);
        bool high_priority = set_thread_priority();
        
        if (pinned) {
            print_thread_info("Producer", producer_cpu);
        } else {
            std::cout << "Producer: CPU pinning not supported/failed, using default scheduling" << std::endl;
        }
        
        if (high_priority) {
            std::cout << "Producer: High priority set successfully" << std::endl;
        }
        
        for (uint64_t i = 0; i < NUM_ELEMENTS; ++i) {
            while (!queue.push(i)) {
                std::this_thread::yield();
            }
        }
        
        producer_done.store(true, std::memory_order_release);
    });
    
    // Consumer thread
    std::thread consumer_thread([&queue, consumer_cpu]() {
        // 尝试绑定到指定CPU
        bool pinned = pin_thread_to_cpu(consumer_cpu);
        bool high_priority = set_thread_priority();
        
        if (pinned) {
            print_thread_info("Consumer", consumer_cpu);
        } else {
            std::cout << "Consumer: CPU pinning not supported/failed, using default scheduling" << std::endl;
        }
        
        if (high_priority) {
            std::cout << "Consumer: High priority set successfully" << std::endl;
        }
        
        uint64_t received_count = 0;
        uint64_t expected_value = 0;
        
        while (received_count < NUM_ELEMENTS) {
            uint64_t* data = queue.front();
            if (data != nullptr) {
                assert(*data == expected_value);
                expected_value++;
                queue.pop();
                received_count++;
            } else {
                std::this_thread::yield();
            }
        }
    });
    
    producer_thread.join();
    consumer_thread.join();
    
    auto overall_end = std::chrono::high_resolution_clock::now();
    auto overall_duration = std::chrono::duration_cast<std::chrono::microseconds>(overall_end - overall_start);
    
    TestResult result;
    result.total_time_us = overall_duration.count();
    result.overall_throughput = NUM_ELEMENTS * 1000000.0 / overall_duration.count();
    result.bandwidth_mbps = (NUM_ELEMENTS * sizeof(uint64_t) * 1000000.0 / overall_duration.count()) / (1024 * 1024);
    result.producer_throughput = result.overall_throughput; // Approximation
    result.consumer_throughput = result.overall_throughput; // Approximation
    
    return result;
}

// Test function for soft array SPSCQueue
TestResult test_soft_array_queue() {
    producer_done.store(false);
    
    auto* queue = SPSCQueueSoftArray<uint64_t>::create(QUEUE_SIZE);
    if (!queue) {
        throw std::runtime_error("Failed to create soft array queue");
    }
    
    int cpu_count = get_cpu_count();
    int producer_cpu = 0;
    int consumer_cpu = (cpu_count > 1) ? 1 : 0;
    
    auto overall_start = std::chrono::high_resolution_clock::now();
    
    // Producer thread
    std::thread producer_thread([queue, producer_cpu]() {
        // 尝试绑定到指定CPU
        bool pinned = pin_thread_to_cpu(producer_cpu);
        bool high_priority = set_thread_priority();
        
        if (pinned) {
            print_thread_info("Producer", producer_cpu);
        } else {
            std::cout << "Producer: CPU pinning not supported/failed, using default scheduling" << std::endl;
        }
        
        if (high_priority) {
            std::cout << "Producer: High priority set successfully" << std::endl;
        }
        
        for (uint64_t i = 0; i < NUM_ELEMENTS; ++i) {
            while (!queue->push(i)) {
                std::this_thread::yield();
            }
        }
        
        producer_done.store(true, std::memory_order_release);
    });
    
    // Consumer thread
    std::thread consumer_thread([queue, consumer_cpu]() {
        // 尝试绑定到指定CPU
        bool pinned = pin_thread_to_cpu(consumer_cpu);
        bool high_priority = set_thread_priority();
        
        if (pinned) {
            print_thread_info("Consumer", consumer_cpu);
        } else {
            std::cout << "Consumer: CPU pinning not supported/failed, using default scheduling" << std::endl;
        }
        
        if (high_priority) {
            std::cout << "Consumer: High priority set successfully" << std::endl;
        }
        
        uint64_t received_count = 0;
        uint64_t expected_value = 0;
        
        while (received_count < NUM_ELEMENTS) {
            uint64_t* data = queue->front();
            if (data != nullptr) {
                assert(*data == expected_value);
                expected_value++;
                queue->pop();
                received_count++;
            } else {
                std::this_thread::yield();
            }
        }
    });
    
    producer_thread.join();
    consumer_thread.join();
    
    auto overall_end = std::chrono::high_resolution_clock::now();
    auto overall_duration = std::chrono::duration_cast<std::chrono::microseconds>(overall_end - overall_start);
    
    TestResult result;
    result.total_time_us = overall_duration.count();
    result.overall_throughput = NUM_ELEMENTS * 1000000.0 / overall_duration.count();
    result.bandwidth_mbps = (NUM_ELEMENTS * sizeof(uint64_t) * 1000000.0 / overall_duration.count()) / (1024 * 1024);
    result.producer_throughput = result.overall_throughput; // Approximation
    result.consumer_throughput = result.overall_throughput; // Approximation
    
    SPSCQueueSoftArray<uint64_t>::destroy(queue);
    
    return result;
}

TestResult average_results(const std::vector<TestResult>& results) {
    TestResult avg = {0, 0, 0, 0, 0};
    
    for (const auto& result : results) {
        avg.producer_throughput += result.producer_throughput;
        avg.consumer_throughput += result.consumer_throughput;
        avg.overall_throughput += result.overall_throughput;
        avg.bandwidth_mbps += result.bandwidth_mbps;
        avg.total_time_us += result.total_time_us;
    }
    
    size_t count = results.size();
    avg.producer_throughput /= count;
    avg.consumer_throughput /= count;
    avg.overall_throughput /= count;
    avg.bandwidth_mbps /= count;
    avg.total_time_us /= count;
    
    return avg;
}

void print_result(const std::string& name, const TestResult& result) {
    std::cout << std::fixed << std::setprecision(2);
    std::cout << name << ":" << std::endl;
    std::cout << "  Total time: " << result.total_time_us << " microseconds" << std::endl;
    std::cout << "  Throughput: " << result.overall_throughput << " elements/second" << std::endl;
    std::cout << "  Bandwidth: " << result.bandwidth_mbps << " MB/s" << std::endl;
    std::cout << std::endl;
}

int main() {
    int cpu_count = get_cpu_count();
    
    std::cout << "SPSC Queue Performance Comparison with CPU Pinning" << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "Available CPU cores: " << cpu_count << std::endl;
    std::cout << "Producer CPU: 0" << std::endl;
    std::cout << "Consumer CPU: " << ((cpu_count > 1) ? 1 : 0) << std::endl;
    std::cout << "Number of elements: " << NUM_ELEMENTS << std::endl;
    std::cout << "Queue capacity: " << QUEUE_SIZE << std::endl;
    std::cout << "Element size: " << sizeof(uint64_t) << " bytes" << std::endl;
    std::cout << "Total data size: " << (NUM_ELEMENTS * sizeof(uint64_t) / 1024 / 1024) << " MB" << std::endl;
    std::cout << "Number of runs: " << NUM_RUNS << std::endl;
    std::cout << std::endl;

    // Test original implementation
    std::cout << "Testing original SPSCQueue..." << std::endl;
    std::vector<TestResult> original_results;
    
    for (int i = 0; i < NUM_RUNS; ++i) {
        SPSCQueue<uint64_t> queue(QUEUE_SIZE);
        auto result = test_original_queue(queue);
        original_results.push_back(result);
        std::cout << "  Run " << (i+1) << ": " << result.bandwidth_mbps << " MB/s" << std::endl;
    }
    
    TestResult original_avg = average_results(original_results);
    
    std::cout << std::endl;
    
    // Test soft array implementation
    std::cout << "Testing soft array SPSCQueue..." << std::endl;
    std::vector<TestResult> soft_array_results;
    
    for (int i = 0; i < NUM_RUNS; ++i) {
        auto result = test_soft_array_queue();
        soft_array_results.push_back(result);
        std::cout << "  Run " << (i+1) << ": " << result.bandwidth_mbps << " MB/s" << std::endl;
    }
    
    TestResult soft_array_avg = average_results(soft_array_results);
    
    std::cout << std::endl;
    std::cout << "Average Results:" << std::endl;
    std::cout << "===============" << std::endl;
    
    print_result("Original SPSCQueue", original_avg);
    print_result("Soft Array SPSCQueue", soft_array_avg);
    
    // Performance comparison
    double improvement = ((soft_array_avg.bandwidth_mbps - original_avg.bandwidth_mbps) / original_avg.bandwidth_mbps) * 100.0;
    
    std::cout << "Performance Comparison:" << std::endl;
    std::cout << "=====================" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    
    if (improvement > 0) {
        std::cout << "Soft array implementation is " << improvement << "% faster" << std::endl;
    } else {
        std::cout << "Original implementation is " << (-improvement) << "% faster" << std::endl;
    }
    
    std::cout << "Soft array bandwidth: " << soft_array_avg.bandwidth_mbps << " MB/s" << std::endl;
    std::cout << "Original bandwidth: " << original_avg.bandwidth_mbps << " MB/s" << std::endl;
    
    return 0;
}