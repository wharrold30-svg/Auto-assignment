# Compiler and Base Flags
CXX        := g++
BASE_FLAGS := -std=c++23 -Wall -Wextra -pthread -Iinclude -Ilibs
LDFLAGS    := -lcurl -lcryptopp -lgtest -lgtest_main -pthread

# Debug Toggle (Default: Off/Release mode)
DEBUG ?= 0

ifeq ($(DEBUG),1)
    CXXFLAGS := $(BASE_FLAGS) -g -O0 -DDEBUG_MODE
    BUILD_MSG := "--- BUILDING IN DEBUG MODE ---"
else
    CXXFLAGS := $(BASE_FLAGS) -O3
    BUILD_MSG := "--- BUILDING IN RELEASE MODE ---"
endif

# Directories
SRC_DIR   := src
INC_DIR   := include
LIB_DIR   := libs
TEST_DIR  := tests
BUILD_DIR := build
OBJ_DIR   := $(BUILD_DIR)/obj

# Application Files
SRCS      := $(wildcard $(SRC_DIR)/*.cpp)
OBJS      := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))
TARGET    := $(BUILD_DIR)/app

# Test Files & Infrastructure
COMMON_SRCS      := Connection.cpp Date.cpp Email.cpp
COMMON_SRC_PATHS := $(addprefix $(LIB_DIR)/, $(COMMON_SRCS))

TEST_SRCS        := $(wildcard $(TEST_DIR)/*_test.cpp)
TEST_EXECUTABLES := $(patsubst $(TEST_DIR)/%_test.cpp, $(BUILD_DIR)/%_test, $(TEST_SRCS))

# Default target: build the main app
all: print_mode $(TARGET)

print_mode:
	@echo $(BUILD_MSG)

# Build and execute the main application instantly
run: all
	@echo "--- Running Main Application ---"
	./$(TARGET)

# Link main executable
$(TARGET): $(OBJS) $(COMMON_SRC_PATHS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

# Compile main source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# =====================================================================
# TEST IMPLEMENTATION BLOCK
# =====================================================================

# Pattern rule to build individual test binaries
$(BUILD_DIR)/%_test: $(TEST_DIR)/%_test.cpp $(COMMON_SRC_PATHS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

# Target to run a single specific test by name (e.g., 'make run-Connection')
run-%: $(BUILD_DIR)/%_test
	@echo "--- Running Test Suite: $* ---"
	./$<

# Target to run every single test suite found in the tests/ directory
run_all: $(TEST_EXECUTABLES)
	@echo "========================================"
	@echo "Running all test suites..."
	@echo "========================================"
	@for exec in $(TEST_EXECUTABLES); do \
		echo "[RUNNING] Test Suite: $$(basename $$exec)"; \
		./$$exec || exit 1; \
	done
	@echo "SUCCESS: Passed all tests!"

# Create build directories
$(BUILD_DIR) $(OBJ_DIR):
	@mkdir -p $@

# =====================================================================
# CLEAN TARGETS
# =====================================================================

# Clean ONLY main application build artifacts (keeps test binaries)
clean_src:
	@echo "Cleaning main application object files and binary..."
	@rm -rf $(OBJ_DIR) $(TARGET)

# Clean ONLY compiled test binaries (keeps main app object files)
clean_test:
	@echo "Cleaning compiled test binaries..."
	@rm -f $(TEST_EXECUTABLES)

# Clean everything completely
clean:
	@echo "Cleaning all build artifacts..."
	@rm -rf $(BUILD_DIR)

.PHONY: all run clean clean_src clean_test run_all print_mode

