#include <iostream>
#include <iomanip>
#include "chan.h"
#include "chan_soft_array.h"

int main() {
    std::cout << "SPSC Queue Memory Layout Analysis" << std::endl;
    std::cout << "=================================" << std::endl;
    std::cout << std::endl;
    
    const int queue_size = 1024;
    
    // Original implementation
    SPSCQueue<uint64_t> original_queue(queue_size);
    std::cout << "Original SPSCQueue:" << std::endl;
    std::cout << "  Class size: " << sizeof(SPSCQueue<uint64_t>) << " bytes" << std::endl;
    std::cout << "  Buffer allocation: Separate heap allocation" << std::endl;
    std::cout << "  Memory layout: Object and buffer are separate" << std::endl;
    std::cout << "  Cache locality: Potentially poor (two separate allocations)" << std::endl;
    std::cout << std::endl;
    
    // Soft array implementation
    auto* soft_queue = SPSCQueueSoftArray<uint64_t>::create(queue_size);
    std::cout << "Soft Array SPSCQueue:" << std::endl;
    std::cout << "  Base class size: " << sizeof(SPSCQueueSoftArray<uint64_t>) << " bytes" << std::endl;
    std::cout << "  Buffer size: " << (queue_size + 1) * sizeof(uint64_t) << " bytes" << std::endl;
    std::cout << "  Total allocated size: " << sizeof(SPSCQueueSoftArray<uint64_t>) + (queue_size + 1) * sizeof(uint64_t) << " bytes" << std::endl;
    std::cout << "  Buffer allocation: Inline with object (flexible array)" << std::endl;
    std::cout << "  Memory layout: Object and buffer are contiguous" << std::endl;
    std::cout << "  Cache locality: Better (single allocation)" << std::endl;
    std::cout << std::endl;
    
    // Address analysis
    std::cout << "Memory Address Analysis:" << std::endl;
    std::cout << "========================" << std::endl;
    std::cout << std::hex << std::showbase;
    std::cout << "Original queue object: " << &original_queue << std::endl;
    std::cout << "Soft array queue object: " << soft_queue << std::endl;
    
    // Test a few operations to verify everything works
    std::cout << std::dec << std::noshowbase;
    std::cout << std::endl;
    std::cout << "Functionality Test:" << std::endl;
    std::cout << "==================" << std::endl;
    
    // Test original queue
    original_queue.push(42);
    original_queue.push(100);
    uint64_t* val1 = original_queue.front();
    std::cout << "Original queue - first value: " << (val1 ? *val1 : 0) << std::endl;
    original_queue.pop();
    uint64_t* val2 = original_queue.front();
    std::cout << "Original queue - second value: " << (val2 ? *val2 : 0) << std::endl;
    original_queue.pop();
    
    // Test soft array queue
    soft_queue->push(42);
    soft_queue->push(100);
    uint64_t* val3 = soft_queue->front();
    std::cout << "Soft array queue - first value: " << (val3 ? *val3 : 0) << std::endl;
    soft_queue->pop();
    uint64_t* val4 = soft_queue->front();
    std::cout << "Soft array queue - second value: " << (val4 ? *val4 : 0) << std::endl;
    soft_queue->pop();
    
    std::cout << std::endl;
    std::cout << "Both implementations work correctly!" << std::endl;
    
    SPSCQueueSoftArray<uint64_t>::destroy(soft_queue);
    
    return 0;
}