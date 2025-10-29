#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <iomanip>
#include "chan_soft_array.h"

// 测试参数
constexpr int TEST_COUNT = 1000000;  // 测试次数
constexpr int WARMUP_COUNT = 100000; // 预热次数

// 定义不同缓存行大小的类型别名
using Queue32 = SPSCQueueSoftArray<int, 32>;
using Queue64 = SPSCQueueSoftArray<int, 64>;
using Queue128 = SPSCQueueSoftArray<int, 128>;
using Queue256 = SPSCQueueSoftArray<int, 256>;

template<typename QueueType>
void producer_consumer_test(QueueType* queue, const std::string& name) {
    std::cout << "\n测试缓存行大小: " << name << std::endl;
    
    // 预热
    std::thread producer([queue]() {
        for (int i = 0; i < WARMUP_COUNT; ++i) {
            queue->push(i);
        }
    });
    
    std::thread consumer([queue]() {
        int consumed = 0;
        while (consumed < WARMUP_COUNT) {
            auto* item = queue->front();
            if (item) {
                queue->pop();
                consumed++;
            }
        }
    });
    
    producer.join();
    consumer.join();
    
    // 正式测试
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::thread test_producer([queue]() {
        for (int i = 0; i < TEST_COUNT; ++i) {
            queue->push(i);
        }
    });
    
    std::thread test_consumer([queue]() {
        int consumed = 0;
        while (consumed < TEST_COUNT) {
            auto* item = queue->front();
            if (item) {
                queue->pop();
                consumed++;
            }
        }
    });
    
    test_producer.join();
    test_consumer.join();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    double ops_per_second = (double)TEST_COUNT * 1000000.0 / duration.count();
    
    std::cout << "  执行时间: " << duration.count() << " 微秒" << std::endl;
    std::cout << "  吞吐量: " << std::fixed << std::setprecision(0) << ops_per_second << " ops/sec" << std::endl;
    std::cout << "  每次操作延迟: " << std::fixed << std::setprecision(3) 
              << (double)duration.count() / TEST_COUNT << " 微秒" << std::endl;
}

template<typename QueueType>
void latency_test(QueueType* queue, const std::string& name) {
    std::cout << "\n延迟测试 - 缓存行大小: " << name << std::endl;
    
    std::vector<double> latencies;
    latencies.reserve(1000);
    
    for (int i = 0; i < 1000; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        
        queue->push(i);
        auto* item = queue->front();
        if (item) {
            queue->pop();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        latencies.push_back(latency);
    }
    
    // 计算统计信息
    std::sort(latencies.begin(), latencies.end());
    double avg = 0;
    for (auto lat : latencies) {
        avg += lat;
    }
    avg /= latencies.size();
    
    std::cout << "  平均延迟: " << std::fixed << std::setprecision(1) << avg << " 纳秒" << std::endl;
    std::cout << "  中位数延迟: " << std::fixed << std::setprecision(1) 
              << latencies[latencies.size() / 2] << " 纳秒" << std::endl;
    std::cout << "  99分位延迟: " << std::fixed << std::setprecision(1) 
              << latencies[latencies.size() * 99 / 100] << " 纳秒" << std::endl;
}

int main() {
    std::cout << "SPSC 队列缓存行大小性能测试" << std::endl;
    std::cout << "=================================" << std::endl;
    std::cout << "测试次数: " << TEST_COUNT << std::endl;
    std::cout << "预热次数: " << WARMUP_COUNT << std::endl;
    
    // 创建不同缓存行大小的队列
    auto* queue32 = Queue32::create(1024);
    auto* queue64 = Queue64::create(1024);
    auto* queue128 = Queue128::create(1024);
    auto* queue256 = Queue256::create(1024);
    
    if (!queue32 || !queue64 || !queue128 || !queue256) {
        std::cerr << "队列创建失败!" << std::endl;
        return 1;
    }
    
    std::cout << "\n=== 吞吐量测试 ===" << std::endl;
    
    // 进行吞吐量测试
    producer_consumer_test(queue32, "32字节");
    producer_consumer_test(queue64, "64字节");
    producer_consumer_test(queue128, "128字节");
    producer_consumer_test(queue256, "256字节");
    
    std::cout << "\n=== 延迟测试 ===" << std::endl;
    
    // 进行延迟测试
    latency_test(queue32, "32字节");
    latency_test(queue64, "64字节");
    latency_test(queue128, "128字节");
    latency_test(queue256, "256字节");
    
    // 内存使用情况分析
    std::cout << "\n=== 内存使用分析 ===" << std::endl;
    std::cout << "Queue32 对象大小: " << sizeof(Queue32) << " 字节" << std::endl;
    std::cout << "Queue64 对象大小: " << sizeof(Queue64) << " 字节" << std::endl;
    std::cout << "Queue128 对象大小: " << sizeof(Queue128) << " 字节" << std::endl;
    std::cout << "Queue256 对象大小: " << sizeof(Queue256) << " 字节" << std::endl;
    
    // 清理
    Queue32::destroy(queue32);
    Queue64::destroy(queue64);
    Queue128::destroy(queue128);
    Queue256::destroy(queue256);
    
    return 0;
}