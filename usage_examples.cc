// SPSC队列使用示例和最佳实践
// ================================

#include "chan_soft_array.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <iomanip>
#include <functional>

// 示例1: 基础使用
void basic_usage_example() {
    std::cout << "\n=== 基础使用示例 ===" << std::endl;
    
    // 创建推荐配置的队列 (64字节缓存行)
    using Queue = SPSCQueueSoftArray<int, 64>;
    auto* queue = Queue::create(1024);
    
    if (!queue) {
        std::cerr << "队列创建失败!" << std::endl;
        return;
    }
    
    // 生产者：发送1000个数字
    std::thread producer([queue]() {
        for (int i = 0; i < 1000; ++i) {
            queue->push(i);
            if (i % 100 == 0) {
                std::cout << "生产者发送: " << i << std::endl;
            }
        }
        std::cout << "生产者完成" << std::endl;
    });
    
    // 消费者：接收并处理数据
    std::thread consumer([queue]() {
        int received = 0;
        while (received < 1000) {
            auto* item = queue->front();
            if (item) {
                if (*item % 100 == 0) {
                    std::cout << "消费者接收: " << *item << std::endl;
                }
                queue->pop();
                received++;
            }
        }
        std::cout << "消费者完成" << std::endl;
    });
    
    producer.join();
    consumer.join();
    
    std::cout << "队列大小: " << queue->size() << std::endl;
    Queue::destroy(queue);
}

// 示例2: 高性能配置
void high_performance_example() {
    std::cout << "\n=== 高性能配置示例 ===" << std::endl;
    
    // 使用128字节缓存行获得最佳性能
    using HighPerfQueue = SPSCQueueSoftArray<uint64_t, 128>;
    auto* queue = HighPerfQueue::create(4096);
    
    const int TEST_COUNT = 1000000;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::thread producer([queue, TEST_COUNT]() {
        for (int i = 0; i < TEST_COUNT; ++i) {
            queue->push(static_cast<uint64_t>(i));
        }
    });
    
    std::thread consumer([queue, TEST_COUNT]() {
        int consumed = 0;
        while (consumed < TEST_COUNT) {
            auto* item = queue->front();
            if (item) {
                queue->pop();
                consumed++;
            }
        }
    });
    
    producer.join();
    consumer.join();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    double ops_per_second = (double)TEST_COUNT * 1000000.0 / duration.count();
    std::cout << "高性能测试完成:" << std::endl;
    std::cout << "  吞吐量: " << static_cast<int>(ops_per_second) << " ops/sec" << std::endl;
    std::cout << "  执行时间: " << duration.count() << " 微秒" << std::endl;
    
    HighPerfQueue::destroy(queue);
}

// 示例3: 不同缓存行大小对比
void cacheline_comparison_example() {
    std::cout << "\n=== 缓存行大小对比示例 ===" << std::endl;
    
    const int TEST_SIZE = 100000;
    
    // 测试不同缓存行大小
    std::vector<std::pair<std::string, std::function<double()>>> tests = {
        {"32字节", []() {
            using Queue32 = SPSCQueueSoftArray<int, 32>;
            auto* queue = Queue32::create(1024);
            
            auto start = std::chrono::high_resolution_clock::now();
            
            std::thread producer([queue]() {
                for (int i = 0; i < TEST_SIZE; ++i) {
                    queue->push(i);
                }
            });
            
            std::thread consumer([queue]() {
                int consumed = 0;
                while (consumed < TEST_SIZE) {
                    auto* item = queue->front();
                    if (item) {
                        queue->pop();
                        consumed++;
                    }
                }
            });
            
            producer.join();
            consumer.join();
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            Queue32::destroy(queue);
            return (double)TEST_SIZE * 1000000.0 / duration.count();
        }},
        
        {"64字节", []() {
            using Queue64 = SPSCQueueSoftArray<int, 64>;
            auto* queue = Queue64::create(1024);
            
            auto start = std::chrono::high_resolution_clock::now();
            
            std::thread producer([queue]() {
                for (int i = 0; i < TEST_SIZE; ++i) {
                    queue->push(i);
                }
            });
            
            std::thread consumer([queue]() {
                int consumed = 0;
                while (consumed < TEST_SIZE) {
                    auto* item = queue->front();
                    if (item) {
                        queue->pop();
                        consumed++;
                    }
                }
            });
            
            producer.join();
            consumer.join();
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            Queue64::destroy(queue);
            return (double)TEST_SIZE * 1000000.0 / duration.count();
        }},
        
        {"128字节", []() {
            using Queue128 = SPSCQueueSoftArray<int, 128>;
            auto* queue = Queue128::create(1024);
            
            auto start = std::chrono::high_resolution_clock::now();
            
            std::thread producer([queue]() {
                for (int i = 0; i < TEST_SIZE; ++i) {
                    queue->push(i);
                }
            });
            
            std::thread consumer([queue]() {
                int consumed = 0;
                while (consumed < TEST_SIZE) {
                    auto* item = queue->front();
                    if (item) {
                        queue->pop();
                        consumed++;
                    }
                }
            });
            
            producer.join();
            consumer.join();
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            Queue128::destroy(queue);
            return (double)TEST_SIZE * 1000000.0 / duration.count();
        }}
    };
    
    std::cout << "缓存行大小  |  吞吐量(ops/sec)" << std::endl;
    std::cout << "-----------|----------------" << std::endl;
    
    for (const auto& test : tests) {
        double throughput = test.second();
        std::cout << std::setw(10) << test.first << " | " 
                  << std::setw(13) << static_cast<int>(throughput) << std::endl;
    }
}

// 示例4: 错误处理和最佳实践
void best_practices_example() {
    std::cout << "\n=== 最佳实践示例 ===" << std::endl;
    
    using SafeQueue = SPSCQueueSoftArray<std::string, 64>;
    
    // 1. 检查队列创建是否成功
    auto* queue = SafeQueue::create(512);
    if (!queue) {
        std::cerr << "错误: 队列创建失败!" << std::endl;
        return;
    }
    
    // 2. 使用RAII确保资源释放
    struct QueueGuard {
        SafeQueue* queue;
        explicit QueueGuard(SafeQueue* q) : queue(q) {}
        ~QueueGuard() { if (queue) SafeQueue::destroy(queue); }
    } guard(queue);
    
    // 3. 生产者：处理复杂对象
    std::thread producer([queue]() {
        std::vector<std::string> messages = {
            "Hello, World!",
            "SPSC Queue",
            "高性能消息传递",
            "缓存行优化",
            "完成"
        };
        
        for (const auto& msg : messages) {
            queue->push(msg);  // 使用拷贝构造
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    // 4. 消费者：安全的数据处理
    std::thread consumer([queue]() {
        std::string last_message;
        while (true) {
            auto* message = queue->front();
            if (message) {
                std::cout << "收到消息: " << *message << std::endl;
                last_message = *message;
                queue->pop();
                
                if (last_message == "完成") {
                    break;
                }
            } else {
                // 避免空转，适当休眠
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        }
    });
    
    producer.join();
    consumer.join();
    
    std::cout << "最终队列大小: " << queue->size() << std::endl;
    // guard析构时会自动调用destroy
}

int main() {
    std::cout << "SPSC队列使用示例集合" << std::endl;
    std::cout << "===================" << std::endl;
    
    try {
        basic_usage_example();
        high_performance_example();
        cacheline_comparison_example();
        best_practices_example();
        
        std::cout << "\n所有示例运行完成!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "运行时错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}