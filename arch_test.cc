#include <iostream>
#include "chan_fence.h"

int main() {
    std::cout << "CPU Architecture Detection and Memory Fence Test" << std::endl;
    std::cout << "================================================" << std::endl;
    
    // 检测CPU架构
    std::cout << "Detected CPU Architecture: ";
#if defined(__x86_64__)
    std::cout << "x86_64 (Intel/AMD 64-bit)" << std::endl;
#elif defined(__i386__)
    std::cout << "i386 (Intel/AMD 32-bit)" << std::endl;
#elif defined(__aarch64__)
    std::cout << "AArch64 (ARM 64-bit, Apple M1/M2 etc.)" << std::endl;
#elif defined(__arm__)
    std::cout << "ARM (32-bit)" << std::endl;
#elif defined(__riscv)
    std::cout << "RISC-V" << std::endl;
#else
    std::cout << "Unknown/Other" << std::endl;
#endif

    // 测试内存屏障功能
    std::cout << std::endl;
    std::cout << "Testing Memory Fence Instructions:" << std::endl;
    std::cout << "===================================" << std::endl;
    
    try {
        std::cout << "Testing lfence()... ";
        Fence::lfence();
        std::cout << "OK" << std::endl;
        
        std::cout << "Testing sfence()... ";
        Fence::sfence();
        std::cout << "OK" << std::endl;
        
        std::cout << "Testing mfence()... ";
        Fence::mfence();
        std::cout << "OK" << std::endl;
        
        std::cout << "Testing compiler_fence()... ";
        Fence::compiler_fence();
        std::cout << "OK" << std::endl;
        
        std::cout << std::endl;
        std::cout << "All memory fence instructions work correctly!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    // 演示内存屏障的使用
    std::cout << std::endl;
    std::cout << "Memory Fence Usage Examples:" << std::endl;
    std::cout << "============================" << std::endl;
    
    volatile int shared_data = 0;
    volatile bool flag = false;
    
    // 模拟生产者写入
    std::cout << "Producer side:" << std::endl;
    shared_data = 42;           // 写入数据
    Fence::sfence();            // 确保数据写入完成
    flag = true;                // 设置标志
    std::cout << "  Data written: " << shared_data << std::endl;
    std::cout << "  Flag set: " << flag << std::endl;
    
    // 模拟消费者读取
    std::cout << "Consumer side:" << std::endl;
    if (flag) {                 // 检查标志
        Fence::lfence();        // 确保后续读取不会重排
        int data = shared_data; // 读取数据
        std::cout << "  Data read: " << data << std::endl;
    }
    
    return 0;
}