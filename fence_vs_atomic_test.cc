#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <iomanip>
#include <numeric>
#include <cmath>
#include "chan_soft_array.h"
#include "chan_fence.h"

// 测试参数
constexpr int TEST_COUNT = 1000000;  // 测试次数
constexpr int WARMUP_COUNT = 100000; // 预热次数
constexpr int BENCHMARK_RUNS = 5;    // 基准测试运行次数

// 定义不同实现的类型别名
using QueueSoftArray64 = SPSCQueueSoftArray<int, 1024, 64>;
using QueueSoftArray128 = SPSCQueueSoftArray<int, 1024, 128>;
using QueueFence64 = SPSCQueueFence<int, 1024, 64>;
using QueueFence128 = SPSCQueueFence<int, 1024, 128>;

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
void benchmark_implementation(QueueType* queue, const std::string& name) {
    std::cout << "\n=== " << name << " ===" << std::endl;
    
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

void print_system_info() {
    std::cout << "\n=== 系统信息 ===" << std::endl;
    
    std::system("echo \"CPU型号:\"");
    std::system("cat /proc/cpuinfo | grep 'model name' | head -1 | cut -d: -f2");
    
    std::system("echo \"缓存信息:\"");
    std::system("lscpu | grep -E 'L1d cache|L1i cache|L2 cache|L3 cache' || echo '缓存信息不可用'");
    
    std::cout << "硬件线程数: " << std::thread::hardware_concurrency() << std::endl;
}

int main() {
    std::cout << "SPSC队列实现对比测试: Fence vs Atomic" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "测试次数: " << TEST_COUNT << std::endl;
    std::cout << "预热次数: " << WARMUP_COUNT << std::endl;
    std::cout << "基准运行次数: " << BENCHMARK_RUNS << std::endl;
    
    print_system_info();
    
    // 创建不同实现的队列
    auto* queue_soft_64 = QueueSoftArray64::create();
    auto* queue_soft_128 = QueueSoftArray128::create();
    auto* queue_fence_64 = QueueFence64::create();
    auto* queue_fence_128 = QueueFence128::create();
    
    if (!queue_soft_64 || !queue_soft_128 || !queue_fence_64 || !queue_fence_128) {
        std::cerr << "队列创建失败!" << std::endl;
        return 1;
    }
    
    std::cout << "\n=== 基准吞吐量对比测试 ===" << std::endl;
    
    // 测试不同实现
    benchmark_implementation(queue_soft_64, "SoftArray + Atomic (64字节缓存行)");
    benchmark_implementation(queue_soft_128, "SoftArray + Atomic (128字节缓存行)");
    benchmark_implementation(queue_fence_64, "Fence实现 (64字节缓存行)");
    benchmark_implementation(queue_fence_128, "Fence实现 (128字节缓存行)");
    
    // 内存使用情况分析
    std::cout << "\n=== 内存使用分析 ===" << std::endl;
    std::cout << "SoftArray64 对象大小: " << sizeof(QueueSoftArray64) << " 字节" << std::endl;
    std::cout << "SoftArray128 对象大小: " << sizeof(QueueSoftArray128) << " 字节" << std::endl;
    std::cout << "Fence64 对象大小: " << sizeof(QueueFence64) << " 字节" << std::endl;
    std::cout << "Fence128 对象大小: " << sizeof(QueueFence128) << " 字节" << std::endl;
    
    // 技术分析
    std::cout << "\n=== 技术分析 ===" << std::endl;
    std::cout << "Atomic实现优势:" << std::endl;
    std::cout << "  + 使用C++标准atomic，保证跨平台兼容性" << std::endl;
    std::cout << "  + 内存序语义明确，易于理解和维护" << std::endl;
    std::cout << "  + 编译器和CPU能更好地优化atomic操作" << std::endl;
    std::cout << "\nFence实现特点:" << std::endl;
    std::cout << "  + 更细粒度的内存屏障控制" << std::endl;
    std::cout << "  + 可能在某些特定场景下有性能优势" << std::endl;
    std::cout << "  - 平台相关性强，可移植性较差" << std::endl;
    std::cout << "  - 需要深入理解CPU内存模型" << std::endl;
    
    // 清理资源
    QueueSoftArray64::destroy(queue_soft_64);
    QueueSoftArray128::destroy(queue_soft_128);
    QueueFence64::destroy(queue_fence_64);
    QueueFence128::destroy(queue_fence_128);
    
    std::cout << "\n=== 结论建议 ===" << std::endl;
    std::cout << "• 对于生产环境，推荐使用Atomic实现" << std::endl;
    std::cout << "• Fence实现可作为研究和学习CPU内存模型的参考" << std::endl;
    std::cout << "• 具体选择应基于实际性能测试结果" << std::endl;
    
    return 0;
}