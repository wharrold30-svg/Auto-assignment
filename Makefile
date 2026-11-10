# =====================================================================
# COMPILER AND CONFIGURATION FLAGS
# =====================================================================
CXX        := g++
# Added -MMD -MP to automatically track header (.h) dependency modifications
BASE_FLAGS := -std=c++23 -Wall -Wextra -pthread -Iinclude -Ilibs -Iexternal/include -MMD -MP
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

# =====================================================================
# FILE DISCOVERY & AUTOMATION
# =====================================================================

# 1. Automatically find EVERY single .cpp file in src/ and libs/
APP_SRCS      := $(wildcard $(SRC_DIR)/*.cpp)
LIB_SRCS      := $(wildcard $(LIB_DIR)/*.cpp)

# 2. Map source files into distinct subdirectories inside build/obj/
APP_OBJS      := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/src/%.o, $(APP_SRCS))
LIB_OBJS      := $(patsubst $(LIB_DIR)/%.cpp, $(OBJ_DIR)/libs/%.o, $(LIB_SRCS))

# 3. Combine them all into a single object list for the main application
ALL_APP_OBJS  := $(APP_OBJS) $(LIB_OBJS)
TARGET        := $(BUILD_DIR)/app

# 4. GOOGLE TEST SAFEGUARD:
# Once created, src/main.cpp will become build/obj/src/main.o.
# TEST_SAFE_OBJS strips it out so it never collides with gtest_main.
APP_MAIN_OBJ   := $(OBJ_DIR)/src/main.o
TEST_SAFE_OBJS := $(filter-out $(APP_MAIN_OBJ), $(ALL_APP_OBJS))

# 5. Automatically discover all test files
TEST_SRCS        := $(wildcard $(TEST_DIR)/*_test.cpp)
TEST_EXECUTABLES := $(patsubst $(TEST_DIR)/%_test.cpp, $(BUILD_DIR)/%_test, $(TEST_SRCS))

# 6. Gather all .d tracking files (Application objects + Test objects)
ALL_DEPS         := $(ALL_APP_OBJS:.o=.d) $(TEST_SRCS:$(TEST_DIR)/%.cpp=$(OBJ_DIR)/tests/%.d)

# =====================================================================
# APPLICATION BUILD RULES
# =====================================================================

# Default target: build the main app
all: print_mode $(TARGET)

print_mode:
	@echo $(BUILD_MSG)

# Build and execute the main application instantly
run: all
	@echo "--- Running Main Application ---"
	./$(TARGET)

# Links the application (only looks at pre-compiled object files)
$(TARGET): $(ALL_APP_OBJS) | $(BUILD_DIR)
	@echo "Linking $(TARGET)..."
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

# Pattern rule: Compile src/ files incrementally
$(OBJ_DIR)/src/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Pattern rule: Compile libs/ files incrementally
$(OBJ_DIR)/libs/%.o: $(LIB_DIR)/%.cpp | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# =====================================================================
# GOOGLE TEST IMPLEMENTATION BLOCK
# =====================================================================

# Pattern rule: Compile individual test .cpp files into object files first
$(OBJ_DIR)/tests/%.o: $(TEST_DIR)/%.cpp | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Links an individual test suite binary without the app's main.o
$(BUILD_DIR)/%_test: $(OBJ_DIR)/tests/%_test.o $(TEST_SAFE_OBJS) | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo "Linking Test Suite: $*..."
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

$(BUILD_DIR) $(OBJ_DIR):
	@mkdir -p $@

# =====================================================================
# AUTOMATIC HEADER TRACKING INTEGRATION
# =====================================================================
# Reads the generated compiler .d files. If any header file listed 
# inside them changes, Make automatically rebuilds the dependent object.
-include $(ALL_DEPS)

# =====================================================================
# CLEAN TARGETS
# =====================================================================

# NEW: Clean a single specific test suite (e.g., 'make clean-Connection')
clean-%:
	@echo "Cleaning specific test suite: $*..."
	@rm -f $(BUILD_DIR)/$*_test
	@rm -f $(OBJ_DIR)/tests/$*_test.o
	@rm -f $(OBJ_DIR)/tests/$*_test.d

# Clean ONLY main application build artifacts (keeps test binaries)
clean_src:
	@echo "Cleaning main application object files and binary..."
	@rm -rf $(OBJ_DIR)/src $(TARGET)

# Clean ONLY compiled test binaries and test object files
clean_test:
	@echo "Cleaning compiled test binaries and object files..."
	@rm -rf $(OBJ_DIR)/tests $(TEST_EXECUTABLES)

# Clean everything completely
clean:
	@echo "Cleaning all build artifacts..."
	rm -rf $(BUILD_DIR)

.PHONY: all run clean clean_src clean_test run_all print_mode
