CXX = g++
CXXFLAGS = -std=c++11 -O3 -Wall -Wextra -pthread
TARGET = spsc_test
COMPARE_TARGET = compare_performance
MEMORY_TARGET = memory_layout_test
SOURCES = main.cc
COMPARE_SOURCES = compare_performance.cc
MEMORY_SOURCES = memory_layout_test.cc
HEADERS = chan.h chan_soft_array.h

# Default target
all: $(TARGET) $(COMPARE_TARGET) $(MEMORY_TARGET)

# Build the executable
$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

# Build the comparison test
$(COMPARE_TARGET): $(COMPARE_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(COMPARE_TARGET) $(COMPARE_SOURCES)

# Build the memory layout test
$(MEMORY_TARGET): $(MEMORY_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(MEMORY_TARGET) $(MEMORY_SOURCES)

# Clean target
clean:
	rm -f $(TARGET) $(COMPARE_TARGET) $(MEMORY_TARGET)

# Run the original test
run: $(TARGET)
	./$(TARGET)

# Run the comparison test
compare: $(COMPARE_TARGET)
	./$(COMPARE_TARGET)

# Run the memory layout test
memory: $(MEMORY_TARGET)
	./$(MEMORY_TARGET)

# Debug build
debug: CXXFLAGS = -std=c++11 -g -Wall -Wextra -pthread -DDEBUG
debug: $(TARGET) $(COMPARE_TARGET) $(MEMORY_TARGET)

# Release build with maximum optimization
release: CXXFLAGS = -std=c++11 -O3 -Wall -Wextra -pthread -DNDEBUG -march=native
release: $(TARGET) $(COMPARE_TARGET) $(MEMORY_TARGET)

# Help target
help:
	@echo "Available targets:"
	@echo "  all     - Build all executables (default)"
	@echo "  clean   - Remove built files"
	@echo "  run     - Build and run the original test"
	@echo "  compare - Build and run the performance comparison"
	@echo "  memory  - Build and run the memory layout test"
	@echo "  debug   - Build with debug symbols"
	@echo "  release - Build with maximum optimization"
	@echo "  help    - Show this help message"

.PHONY: all clean run compare memory debug release help