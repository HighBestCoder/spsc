#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <cassert>
#include "chan.h"

const uint64_t NUM_ELEMENTS = 1 << 20; // 1MB elements
const int QUEUE_SIZE = 1024; // Queue capacity

std::atomic<bool> producer_done{false};

void producer(SPSCQueue<uint64_t>& queue) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (uint64_t i = 0; i < NUM_ELEMENTS; ++i) {
        // Push data to queue (blocking until space available)
        while (!queue.push(i)) {
            // Busy wait - the push will block internally when queue is full
            std::this_thread::yield();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    producer_done.store(true, std::memory_order_release);
    
    std::cout << "Producer finished in " << duration.count() << " microseconds" << std::endl;
    std::cout << "Producer throughput: " << (NUM_ELEMENTS * 1000000.0 / duration.count()) 
              << " elements/second" << std::endl;
}

void consumer(SPSCQueue<uint64_t>& queue) {
    auto start_time = std::chrono::high_resolution_clock::now();
    uint64_t received_count = 0;
    uint64_t expected_value = 0;
    
    while (received_count < NUM_ELEMENTS) {
        uint64_t* data = queue.front();
        if (data != nullptr) {
            // Verify data integrity
            assert(*data == expected_value);
            expected_value++;
            
            queue.pop();
            received_count++;
        } else {
            // Queue is empty, yield to avoid busy waiting
            std::this_thread::yield();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    std::cout << "Consumer finished in " << duration.count() << " microseconds" << std::endl;
    std::cout << "Consumer throughput: " << (NUM_ELEMENTS * 1000000.0 / duration.count()) 
              << " elements/second" << std::endl;
    std::cout << "Total elements received: " << received_count << std::endl;
}

int main() {
    std::cout << "SPSC Queue Performance Test" << std::endl;
    std::cout << "=========================" << std::endl;
    std::cout << "Number of elements: " << NUM_ELEMENTS << std::endl;
    std::cout << "Queue capacity: " << QUEUE_SIZE << std::endl;
    std::cout << "Element size: " << sizeof(uint64_t) << " bytes" << std::endl;
    std::cout << "Total data size: " << (NUM_ELEMENTS * sizeof(uint64_t) / 1024 / 1024) << " MB" << std::endl;
    std::cout << std::endl;
    
    // Create SPSC queue
    SPSCQueue<uint64_t> queue(QUEUE_SIZE);
    
    auto overall_start = std::chrono::high_resolution_clock::now();
    
    // Create producer and consumer threads
    std::thread producer_thread(producer, std::ref(queue));
    std::thread consumer_thread(consumer, std::ref(queue));
    
    // Wait for both threads to complete
    producer_thread.join();
    consumer_thread.join();
    
    auto overall_end = std::chrono::high_resolution_clock::now();
    auto overall_duration = std::chrono::duration_cast<std::chrono::microseconds>(overall_end - overall_start);
    
    std::cout << std::endl;
    std::cout << "Overall Performance" << std::endl;
    std::cout << "==================" << std::endl;
    std::cout << "Total time: " << overall_duration.count() << " microseconds" << std::endl;
    std::cout << "Overall throughput: " << (NUM_ELEMENTS * 1000000.0 / overall_duration.count()) 
              << " elements/second" << std::endl;
    std::cout << "Bandwidth: " << (NUM_ELEMENTS * sizeof(uint64_t) * 1000000.0 / overall_duration.count() / 1024 / 1024) 
              << " MB/s" << std::endl;
    
    return 0;
}