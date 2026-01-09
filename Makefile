# ==========================================
# Echo-Mate 完整工程 Makefile
# ==========================================
-include toolchain.mk
CC  ?= gcc
CXX ?= g++

# 1. 目录定义
SRC_DIR   = src
INC_DIR   = include
LIB_DIR   = third_party
BUILD_DIR = product/build
BIN_DIR   = product/bin

TARGET = $(BIN_DIR)/echo_mate_app

# 2. 库文件搜索路径 (使用您提供的绝对路径)
SYSROOT_LIB_DIR := /home/ubuntu/project/Echo-Mate/SDK/rv1106-sdk/sysdrv/source/buildroot/buildroot-2023.02.6/output/host/arm-buildroot-linux-uclibcgnueabihf/sysroot/usr/lib

# 3. 头文件包含路径 (INCLUDES)
# [核心原则] 将 src 添加为根，这样可以用 #include "app/ai_chat/xxx.h"
INCLUDES := -I$(INC_DIR)
INCLUDES += -I$(INC_DIR)/common
INCLUDES += -I$(INC_DIR)/cjson
INCLUDES += -I$(SRC_DIR)
INCLUDES += -I$(SRC_DIR)/services/network

# AI Chat 模块路径
INCLUDES += -I$(SRC_DIR)/app/AI_chat
INCLUDES += -I$(SRC_DIR)/app/AI_chat/states

# [第三方库]
INCLUDES += -I$(LIB_DIR)/lvgl
INCLUDES += -I$(LIB_DIR)/lvgl/src
INCLUDES += -I$(LIB_DIR)/lvgl/demos
INCLUDES += -I$(LIB_DIR)/tinyalsa/include

# 4. 编译参数 (FLAGS)
# 通用参数：包含优化等级、警告和头文件路径
COMMON_FLAGS := -O2 -g -Wall -Wshadow -Wundef $(INCLUDES)

# C 编译参数
CFLAGS  := $(COMMON_FLAGS) \
           -DUSE_EVDEV=1 \
           -DLV_LVGL_H_INCLUDE_SIMPLE

# C++ 编译参数 (使用 C++11 标准)
CXXFLAGS := $(COMMON_FLAGS) -std=c++11

# 链接参数：指定库路径和需要链接的库
# 注意：这里按需链接了 curl, cjson, ssl, crypto, z, pthread
LDFLAGS  := -L$(SYSROOT_LIB_DIR) -lm -lpthread -lcurl -lcjson -lssl -lcrypto -lz

# 5. 源文件扫描 (Source Scanning)
# [妙招] 使用 find 命令递归扫描，自动发现新添加的文件！
# C 文件：排除 utils 目录下的工具源码(防止 main 函数冲突)
APP_SRCS_C    := $(shell find $(SRC_DIR) -name '*.c' -not -path "$(SRC_DIR)/utils/*")

# 第三方库源码
TINYALSA_SRCS := $(shell find $(LIB_DIR)/tinyalsa/src -name '*.c')
LVGL_SRCS     := $(shell find $(LIB_DIR)/lvgl/src -name '*.c')
LVGL_SRCS     += $(shell find $(LIB_DIR)/lvgl/demos -name '*.c')

# C++ 文件：自动扫描 src 下所有 .cpp 和 .cc
# 这里会自动抓取你的 src/app/ai_chat/chat_app.cc
APP_SRCS_CPP  := $(shell find $(SRC_DIR) -name '*.cpp')
APP_SRCS_CC   := $(shell find $(SRC_DIR) -name '*.cc')

# 6. 目标文件生成路径 (Object Generation)
# 将源文件的 .c/.cpp/.cc 映射为 product/build/xxx.o
OBJS := $(APP_SRCS_C:%.c=$(BUILD_DIR)/%.o) \
        $(TINYALSA_SRCS:%.c=$(BUILD_DIR)/%.o) \
        $(LVGL_SRCS:%.c=$(BUILD_DIR)/%.o) \
        $(APP_SRCS_CPP:%.cpp=$(BUILD_DIR)/%.o) \
        $(APP_SRCS_CC:%.cc=$(BUILD_DIR)/%.o)

# 7. 编译规则 (Rules)
.PHONY: all clean check info

all: $(TARGET)

# 链接步骤
$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	@echo "LINKING: $@"
	@$(CXX) $(OBJS) -o $@ $(LDFLAGS)
	@echo "✅ BUILD SUCCESS! Output: $@"

# C 文件编译规则
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "CC   $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# C++ (cpp) 文件编译规则
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "CXX  $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# C++ (cc) 文件编译规则
$(BUILD_DIR)/%.o: %.cc
	@mkdir -p $(dir $@)
	@echo "CXX  $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# 代码检查规则 (Check)
check:
	@echo ">>> STARTING HEADER & SYNTAX CHECK <<<"
	@for file in $(APP_SRCS_C); do \
		echo "[CHECK C]   $$file"; \
		$(CC) $(CFLAGS) -fsyntax-only $$file || exit 1; \
	done
	@for file in $(APP_SRCS_CPP); do \
		echo "[CHECK CPP] $$file"; \
		$(CXX) $(CXXFLAGS) -fsyntax-only $$file || exit 1; \
	done
	@for file in $(APP_SRCS_CC); do \
		echo "[CHECK CC]  $$file"; \
		$(CXX) $(CXXFLAGS) -fsyntax-only $$file || exit 1; \
	done
	@echo ">>> ✅ CHECK PASSED: ALL INCLUDES OK! <<<"

clean:
	@echo "CLEANING..."
	@rm -rf product/build product/bin