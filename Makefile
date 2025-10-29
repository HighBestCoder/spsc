CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -pthread
TARGET = spsc_test
COMPARE_TARGET = compare_performance
MEMORY_TARGET = memory_layout_test
CACHELINE_TARGET = cacheline_performance_test
BENCHMARK_TARGET = benchmark_cacheline
EXAMPLES_TARGET = usage_examples
FENCE_TEST_TARGET = fence_vs_atomic_test
SOURCES = main.cc
COMPARE_SOURCES = compare_performance.cc
MEMORY_SOURCES = memory_layout_test.cc
CACHELINE_SOURCES = cacheline_performance_test.cc
BENCHMARK_SOURCES = benchmark_cacheline.cc
EXAMPLES_SOURCES = usage_examples.cc
FENCE_TEST_SOURCES = fence_vs_atomic_test.cc
HEADERS = chan.h chan_soft_array.h chan_fence.h

# Default target
all: $(TARGET) $(COMPARE_TARGET) $(MEMORY_TARGET) $(CACHELINE_TARGET) $(BENCHMARK_TARGET) $(EXAMPLES_TARGET) $(FENCE_TEST_TARGET)

# Build the executable
$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

# Build the comparison test
$(COMPARE_TARGET): $(COMPARE_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(COMPARE_TARGET) $(COMPARE_SOURCES)

# Build the memory layout test
$(MEMORY_TARGET): $(MEMORY_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(MEMORY_TARGET) $(MEMORY_SOURCES)

# Build the cacheline performance test
$(CACHELINE_TARGET): $(CACHELINE_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(CACHELINE_TARGET) $(CACHELINE_SOURCES)

# Build the benchmark cacheline test
$(BENCHMARK_TARGET): $(BENCHMARK_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(BENCHMARK_TARGET) $(BENCHMARK_SOURCES)

# Build the usage examples
$(EXAMPLES_TARGET): $(EXAMPLES_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(EXAMPLES_TARGET) $(EXAMPLES_SOURCES)

# Build the fence vs atomic test
$(FENCE_TEST_TARGET): $(FENCE_TEST_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(FENCE_TEST_TARGET) $(FENCE_TEST_SOURCES)

# Clean target
clean:
	rm -f $(TARGET) $(COMPARE_TARGET) $(MEMORY_TARGET) $(CACHELINE_TARGET) $(BENCHMARK_TARGET) $(EXAMPLES_TARGET) $(FENCE_TEST_TARGET)

# Run the original test
run: $(TARGET)
	./$(TARGET)

# Run the comparison test
compare: $(COMPARE_TARGET)
	./$(COMPARE_TARGET)

# Run the memory layout test
memory: $(MEMORY_TARGET)
	./$(MEMORY_TARGET)

# Run cacheline performance test
cacheline: $(CACHELINE_TARGET)
	./$(CACHELINE_TARGET)

# Run benchmark test
benchmark: $(BENCHMARK_TARGET)
	./$(BENCHMARK_TARGET)

# Run usage examples
examples: $(EXAMPLES_TARGET)
	./$(EXAMPLES_TARGET)

# Run fence vs atomic test
fence-test: $(FENCE_TEST_TARGET)
	./$(FENCE_TEST_TARGET)

# Run performance report
report: $(BENCHMARK_TARGET)
	chmod +x performance_report.sh && ./performance_report.sh

# Run all tests
test-all: run compare memory cacheline benchmark examples fence-test

# Debug build
debug: CXXFLAGS = -std=c++17 -g -Wall -Wextra -pthread -DDEBUG
debug: all

# Release build with maximum optimization
release: CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -pthread -DNDEBUG -march=native -mtune=native -flto
release: all

# Help target
help:
	@echo "=== SPSC Queue 构建和测试选项 ==="
	@echo ""
	@echo "构建目标:"
	@echo "  all                - 构建所有可执行文件 (默认)"
	@echo "  spsc_test          - 构建原始实现测试"
	@echo "  compare_performance - 构建性能对比测试"
	@echo "  memory_layout_test - 构建内存布局测试"
	@echo "  cacheline_performance_test - 构建缓存行性能测试"
	@echo "  benchmark_cacheline - 构建基准测试"
	@echo "  usage_examples     - 构建使用示例"
	@echo "  fence_vs_atomic_test - 构建Fence vs Atomic对比测试"
	@echo ""
	@echo "运行测试:"
	@echo "  run         - 运行原始实现测试"
	@echo "  compare     - 运行性能对比测试"
	@echo "  memory      - 运行内存布局分析"
	@echo "  cacheline   - 运行缓存行性能测试"
	@echo "  benchmark   - 运行详细基准测试"
	@echo "  examples    - 运行使用示例"
	@echo "  fence-test  - 运行Fence vs Atomic对比测试"
	@echo "  report      - 生成性能报告"
	@echo "  test-all    - 运行所有测试"
	@echo ""
	@echo "构建选项:"
	@echo "  debug       - 调试版本构建"
	@echo "  release     - 发布版本构建 (最大优化)"
	@echo "  clean       - 清理所有生成文件"
	@echo "  help        - 显示此帮助信息"
	@echo ""
	@echo "使用示例:"
	@echo "  make all && make test-all    # 构建并运行所有测试"
	@echo "  make release && make report  # 发布版本构建并生成报告"
	@echo "  make benchmark              # 运行详细性能基准测试"
	@echo "  clean   - Remove built files"
	@echo "  run     - Build and run the original test"
	@echo "  compare - Build and run the performance comparison"
	@echo "  memory  - Build and run the memory layout test"
	@echo "  debug   - Build with debug symbols"
	@echo "  release - Build with maximum optimization"
	@echo "  help    - Show this help message"

.PHONY: all clean run compare memory debug release help