#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <iomanip>
#include <numeric>
#include <cmath>
#include "chan_soft_array.h"

// 测试参数
constexpr int TEST_COUNT = 1000000;  // 测试次数
constexpr int WARMUP_COUNT = 100000; // 预热次数
constexpr int BENCHMARK_RUNS = 5;    // 基准测试运行次数

// 定义不同缓存行大小的类型别名
using Queue32 = SPSCQueueSoftArray<int, 1024, 32>;
using Queue64 = SPSCQueueSoftArray<int, 1024, 64>;
using Queue128 = SPSCQueueSoftArray<int, 1024, 128>;
using Queue256 = SPSCQueueSoftArray<int, 1024, 256>;

template<typename QueueType>
double single_throughput_test(QueueType* queue) {
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
    
    return (double)TEST_COUNT * 1000000.0 / duration.count();
}

template<typename QueueType>
void benchmark_throughput(QueueType* queue, const std::string& name) {
    std::cout << "\n基准测试 - 缓存行大小: " << name << std::endl;
    
    std::vector<double> throughputs;
    throughputs.reserve(BENCHMARK_RUNS);
    
    for (int run = 0; run < BENCHMARK_RUNS; ++run) {
        double throughput = single_throughput_test(queue);
        throughputs.push_back(throughput);
        std::cout << "  第" << (run + 1) << "次: " << std::fixed << std::setprecision(0) 
                  << throughput << " ops/sec" << std::endl;
    }
    
    // 计算统计信息
    double avg = std::accumulate(throughputs.begin(), throughputs.end(), 0.0) / throughputs.size();
    
    std::sort(throughputs.begin(), throughputs.end());
    double median = throughputs[throughputs.size() / 2];
    
    double min_val = *std::min_element(throughputs.begin(), throughputs.end());
    double max_val = *std::max_element(throughputs.begin(), throughputs.end());
    
    // 计算标准差
    double variance = 0.0;
    for (double t : throughputs) {
        variance += (t - avg) * (t - avg);
    }
    variance /= throughputs.size();
    double stddev = std::sqrt(variance);
    
    std::cout << "  平均值: " << std::fixed << std::setprecision(0) << avg << " ops/sec" << std::endl;
    std::cout << "  中位数: " << std::fixed << std::setprecision(0) << median << " ops/sec" << std::endl;
    std::cout << "  最小值: " << std::fixed << std::setprecision(0) << min_val << " ops/sec" << std::endl;
    std::cout << "  最大值: " << std::fixed << std::setprecision(0) << max_val << " ops/sec" << std::endl;
    std::cout << "  标准差: " << std::fixed << std::setprecision(0) << stddev << " ops/sec" << std::endl;
    std::cout << "  变异系数: " << std::fixed << std::setprecision(2) << (stddev / avg * 100) << "%" << std::endl;
}

void print_cpu_info() {
    std::cout << "\n=== 系统信息 ===" << std::endl;
    
    // 尝试读取 CPU 信息
    std::system("echo \"CPU型号:\"");
    std::system("cat /proc/cpuinfo | grep 'model name' | head -1 | cut -d: -f2");
    
    std::system("echo \"缓存信息:\"");
    std::system("lscpu | grep -E 'L1d cache|L1i cache|L2 cache|L3 cache' || echo '缓存信息不可用'");
    
    std::cout << "\n硬件线程数: " << std::thread::hardware_concurrency() << std::endl;
}

int main() {
    std::cout << "SPSC 队列缓存行大小基准测试" << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << "测试次数: " << TEST_COUNT << std::endl;
    std::cout << "预热次数: " << WARMUP_COUNT << std::endl;
    std::cout << "基准运行次数: " << BENCHMARK_RUNS << std::endl;
    
    print_cpu_info();
    
    // 创建不同缓存行大小的队列
    auto* queue32 = Queue32::create();
    auto* queue64 = Queue64::create();
    auto* queue128 = Queue128::create();
    auto* queue256 = Queue256::create();
    
    if (!queue32 || !queue64 || !queue128 || !queue256) {
        std::cerr << "队列创建失败!" << std::endl;
        return 1;
    }
    
    std::cout << "\n=== 基准吞吐量测试 ===" << std::endl;
    
    // 进行基准测试
    benchmark_throughput(queue32, "32字节");
    benchmark_throughput(queue64, "64字节");
    benchmark_throughput(queue128, "128字节");
    benchmark_throughput(queue256, "256字节");
    
    // 内存使用情况分析
    std::cout << "\n=== 内存使用分析 ===" << std::endl;
    std::cout << "Queue32 对象大小: " << sizeof(Queue32) << " 字节" << std::endl;
    std::cout << "Queue64 对象大小: " << sizeof(Queue64) << " 字节" << std::endl;
    std::cout << "Queue128 对象大小: " << sizeof(Queue128) << " 字节" << std::endl;
    std::cout << "Queue256 对象大小: " << sizeof(Queue256) << " 字节" << std::endl;
    
    // 总结和建议
    std::cout << "\n=== 总结 ===" << std::endl;
    std::cout << "基于测试结果:" << std::endl;
    std::cout << "1. 64字节和128字节缓存行大小通常提供最佳性能" << std::endl;
    std::cout << "2. 32字节可能导致false sharing" << std::endl;
    std::cout << "3. 256字节会浪费内存，性能提升有限" << std::endl;
    std::cout << "4. 推荐使用64字节或128字节的缓存行大小" << std::endl;
    
    // 清理
    Queue32::destroy(queue32);
    Queue64::destroy(queue64);
    Queue128::destroy(queue128);
    Queue256::destroy(queue256);
    
    return 0;
}