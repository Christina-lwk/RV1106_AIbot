################################################################################
# C/C++ 混合编译 Makefile
################################################################################

-include toolchain.mk
CC ?= gcc
CXX ?= g++ # C++ 编译器

################################################################################
# 目录配置
################################################################################
SRC_DIR = src
INC_DIR = include
LIB_DIR = third_party
BUILD_DIR = product/build
BIN_DIR = product/bin

TARGET_NAME = echo_mate_app
TARGET = $(BIN_DIR)/$(TARGET_NAME)

################################################################################
# 源文件收集 (C 和 C++ 分开收集)
################################################################################

# 1. 扫描 C 文件 (.c)
APP_SRCS_C := $(shell find $(SRC_DIR) -name '*.c')

# 收集 LVGL (C语言)
LVGL_DIR := $(LIB_DIR)/lvgl
LVGL_SRCS := $(shell find $(LVGL_DIR)/src -name '*.c')
LVGL_SRCS += $(shell find $(LVGL_DIR)/demos -name '*.c')

# 2. 扫描 C++ 文件 (.cpp, .cc, .cxx)
APP_SRCS_CXX := $(shell find $(SRC_DIR) -name '*.cpp')
APP_SRCS_CXX += $(shell find $(SRC_DIR) -name '*.cc')

# 3. 合并所有源文件列表 (用于 info 显示)
SRCS_ALL := $(APP_SRCS_C) $(LVGL_SRCS) $(APP_SRCS_CXX)

# 4. 生成 .o 文件路径
OBJS := $(APP_SRCS_C:%.c=$(BUILD_DIR)/%.o) \
        $(LVGL_SRCS:%.c=$(BUILD_DIR)/%.o) \
        $(APP_SRCS_CXX:%.cpp=$(BUILD_DIR)/%.o) \
        $(APP_SRCS_CXX:%.cc=$(BUILD_DIR)/%.o)

################################################################################
# 编译选项
################################################################################
# 头文件路径
INCLUDES := -I$(INC_DIR) \
            -I$(INC_DIR)/common \
            -I$(SRC_DIR) \
            -I$(LVGL_DIR) \
            -I$(LVGL_DIR)/src \
            -I$(LVGL_DIR)/demos 

# 通用 Flag
COMMON_FLAGS := -O2 -g -Wall -Wshadow -Wundef $(INCLUDES)

# C 专用 Flag
CFLAGS := $(COMMON_FLAGS) -std=c99 -DLV_CONF_INCLUDE_SIMPLE -DLV_LVGL_H_INCLUDE_SIMPLE

# C++ 专用 Flag
CXXFLAGS := $(COMMON_FLAGS) -std=c++11 

# 链接库 (C++ 项目通常需要用 g++ 链接)
LDFLAGS := -lm -lpthread

################################################################################
# 构建规则
################################################################################

.PHONY: all clean info

all: $(TARGET)

# 链接 (使用 CXX 进行链接，以支持 C++ 标准库)
$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	@echo "LINKING (Mixed C/C++): $@"
	@$(CXX) $(OBJS) -o $@ $(LDFLAGS)
	@echo "SUCCESS! Target: $@"

# 编译 C 文件
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# 编译 C++ 文件 (.cpp)
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "CXX $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# 编译 C++ 文件 (.cc)
$(BUILD_DIR)/%.o: %.cc
	@mkdir -p $(dir $@)
	@echo "CXX $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@rm -rf product/build product/bin

info:
	@echo "C files: $(words $(APP_SRCS_C) $(LVGL_SRCS))"
	@echo "C++ files: $(words $(APP_SRCS_CXX))"
