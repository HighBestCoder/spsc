# SPSC Queue 实现比较

## 概述

本项目实现了两种单生产者单消费者(SPSC)队列：
1. **原始实现** (`chan.h`) - 使用分离的内存分配
2. **柔性数组实现** (`chan_soft_array.h`) - 使用柔性数组技术

## 文件结构

```
spsc/
├── chan.h                    # 原始SPSC队列实现
├── chan_soft_array.h         # 柔性数组SPSC队列实现
├── main.cc                   # 原始实现性能测试
├── compare_performance.cc    # 性能对比测试
├── memory_layout_test.cc     # 内存布局分析
├── Makefile                  # 构建配置
└── README.md                 # 本文档
```

## 实现特点

### 原始实现 (chan.h)
- 队列对象和缓冲区分别分配内存
- 使用 `operator new[]` 分配缓冲区
- 类大小：256字节
- 潜在的缓存局部性问题

### 柔性数组实现 (chan_soft_array.h)
- 队列对象和缓冲区在单次分配中连续存储
- 使用柔性数组成员 `T buf_[0]`
- 基础类大小：192字节 + 缓冲区大小
- 更好的缓存局部性
- 使用placement new和自定义内存管理

## 关键技术点

### 柔性数组技术
```cpp
template <typename T>
class SPSCQueueSoftArray {
private:
  alignas(64) int cap_ = 0;
  alignas(64) std::atomic<int> head_{0};
  alignas(64) std::atomic<int> tail_{0};
  alignas(64) T buf_[0];  // 柔性数组成员
};
```

### 内存管理
```cpp
// 创建：计算总内存大小并使用placement new
static SPSCQueueSoftArray* create(const int cap) noexcept {
    size_t total_size = sizeof(SPSCQueueSoftArray) + sizeof(T) * actual_cap;
    void* raw_memory = operator new(total_size, std::align_val_t(64));
    return new(raw_memory) SPSCQueueSoftArray(actual_cap);
}

// 销毁：先调用析构函数再释放内存
static void destroy(SPSCQueueSoftArray* queue) noexcept {
    queue->~SPSCQueueSoftArray();
    operator delete(queue, std::align_val_t(64));
}
```

## 性能测试结果

### 测试配置
- 元素数量：1,048,576 (1M)
- 队列容量：1024
- 元素类型：uint64_t (8字节)
- 总数据量：8MB
- 测试轮数：5轮取平均值

### 结果对比
| 实现方式 | 带宽(MB/s) | 吞吐量(elements/s) | 性能提升 |
|---------|-----------|------------------|---------|
| 原始实现 | 322.98    | 42,333,547       | 基准     |
| 柔性数组 | 372.83    | 48,866,974       | +15.43% |

### 优化编译结果
使用 `-march=native` 等优化选项时，两种实现的性能差距可能会有所变化。

## 构建和运行

### 构建所有目标
```bash
make all
```

### 运行测试
```bash
# 原始实现测试
make run

# 性能对比测试
make compare

# 内存布局分析
make memory
```

### 其他构建选项
```bash
# 调试版本
make debug

# 最大优化版本
make release

# 清理
make clean

# 帮助信息
make help
```

## 内存布局优势

### 原始实现
- 队列对象和缓冲区分别分配
- 可能存在缓存不命中
- 内存碎片化

### 柔性数组实现
- 单次分配，内存连续
- 更好的缓存局部性
- 减少内存碎片

## 设计考虑

### 优势
1. **性能提升**：在测试中显示15.43%的性能提升
2. **内存效率**：单次分配，减少内存碎片
3. **缓存友好**：连续内存布局提高缓存命中率

### 注意事项
1. **复杂性**：需要自定义内存管理
2. **类型安全**：需要仔细处理构造和析构
3. **可移植性**：柔性数组是C99/C++标准的扩展

## 结论

柔性数组实现在保持相同功能的前提下，通过改善内存布局获得了显著的性能提升。这种技术特别适用于：

1. 性能敏感的应用
2. 高频率的数据传输
3. 对内存使用效率有要求的场景

两种实现都提供了完整的SPSC队列功能，可以根据具体需求选择合适的版本。