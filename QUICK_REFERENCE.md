# SPSC Queue 快速参考

## 📚 文件说明

### 核心实现
| 文件 | 说明 | 特点 |
|------|------|------|
| `chan.h` | 原始SPSC队列实现 | 分离内存分配，教学参考 |
| `chan_soft_array.h` | 柔性数组实现 | 高性能，可配置缓存行大小 |

### 测试程序
| 文件 | 功能 | 运行命令 |
|------|------|----------|
| `main.cc` | 原始实现性能测试 | `make run` |
| `compare_performance.cc` | 对比测试 | `make compare` |
| `memory_layout_test.cc` | 内存布局分析 | `make memory` |
| `cacheline_performance_test.cc` | 缓存行性能测试 | `make cacheline` |
| `benchmark_cacheline.cc` | 详细基准测试 | `make benchmark` |
| `usage_examples.cc` | 使用示例 | `make examples` |

### 工具脚本
| 文件 | 功能 | 使用方法 |
|------|------|----------|
| `performance_report.sh` | 性能报告生成 | `make report` |
| `PERFORMANCE_SUMMARY.txt` | 性能总结文档 | 查看性能分析结论 |

## 🚀 快速开始

```bash
# 1. 构建所有程序
make all

# 2. 运行所有测试
make test-all

# 3. 生成性能报告
make report

# 4. 查看帮助
make help
```

## 💻 代码使用

### 推荐配置 (64字节缓存行)
```cpp
using Queue = SPSCQueueSoftArray<int, 1024, 64>;
auto* queue = Queue::create();
// ... 使用队列
Queue::destroy(queue);
```

### 高性能配置 (128字节缓存行)
```cpp
using Queue = SPSCQueueSoftArray<int, 1024, 128>;
auto* queue = Queue::create();
```
// ... 使用队列  
Queue::destroy(queue);
```

## 📊 性能数据

| 配置 | 吞吐量 | 适用场景 |
|------|--------|----------|
| 128字节 | ~170M ops/sec | 极致性能要求 |
| 64字节 | ~144M ops/sec | 生产环境推荐 |
| 32字节 | ~109M ops/sec | 不推荐 |
| 256字节 | ~122M ops/sec | 内存浪费 |

## 🎯 选择指南

- **生产环境**: 使用64字节缓存行
- **极致性能**: 使用128字节缓存行  
- **避免使用**: 32字节(false sharing)和256字节(内存浪费)

## 🔧 构建选项

```bash
make all      # 标准构建
make debug    # 调试版本
make release  # 最大优化
make clean    # 清理文件
```