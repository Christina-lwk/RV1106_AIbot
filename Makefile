-include toolchain.mk
CC  ?= gcc
CXX ?= g++

# 1. 目录定义
SRC_DIR   = src
LIB_DIR   = third_party
LIBS_DIR  = libs
BUILD_DIR = product/build
BIN_DIR   = product/bin

# 目标文件
TARGET = $(BIN_DIR)/echo_mate_app

# 2. 库文件搜索路径 (Sysroot)
SYSROOT_LIB_DIR := /home/ubuntu/project/Echo-Mate/SDK/rv1106-sdk/sysdrv/source/buildroot/buildroot-2023.02.6/output/host/arm-buildroot-linux-uclibcgnueabihf/sysroot/usr/lib

# 3. 头文件包含路径 (INCLUDES)
INCLUDES := -I$(SRC_DIR)/include
INCLUDES += -I$(SRC_DIR)
INCLUDES += -I$(SRC_DIR)/services/network
INCLUDES += -I$(SRC_DIR)/app/AI_chat
INCLUDES += -I$(SRC_DIR)/app/AI_chat/states

# 第三方库头文件
INCLUDES += -I$(LIB_DIR)/tinyalsa/include
INCLUDES += -I$(LIB_DIR)/cjson/include
INCLUDES += -I$(LIBS_DIR)/curl/include
INCLUDES += -I$(LIB_DIR)/lvgl
INCLUDES += -I$(LIB_DIR)/lvgl/src
INCLUDES += -I$(LIB_DIR)/snowboy/include

# 4. 编译参数 (FLAGS)
COMMON_FLAGS := -O2 -g -Wall -Wshadow -Wundef $(INCLUDES)

CFLAGS  := $(COMMON_FLAGS) \
           -DUSE_EVDEV=1 \
           -DLV_LVGL_H_INCLUDE_SIMPLE

CXXFLAGS := $(COMMON_FLAGS) -std=c++11 -D_GLIBCXX_USE_CXX11_ABI=0# 链接参数

LDFLAGS  := -L$(SYSROOT_LIB_DIR) \
            -L$(LIB_DIR)/snowboy/lib \
            -lm -lpthread -lcurl -lcjson -lssl -lcrypto -lz \
            -lsnowboy-detect -lopenblas

# 5. 源文件扫描
APP_SRCS_C    := $(shell find $(SRC_DIR) -name '*.c' -not -path "$(SRC_DIR)/utils/*")
TINYALSA_SRCS := $(shell find $(LIB_DIR)/tinyalsa/src -name '*.c')
CJSON_SRCS    := $(shell find $(LIB_DIR)/cjson -name '*.c')
LVGL_SRCS     := $(shell find $(LIB_DIR)/lvgl/src -name '*.c')
LVGL_SRCS     += $(shell find $(LIB_DIR)/lvgl/demos -name '*.c')

APP_SRCS_CPP  := $(shell find $(SRC_DIR) -name '*.cpp')
APP_SRCS_CC   := $(shell find $(SRC_DIR) -name '*.cc')

# 6. 目标文件生成路径
OBJS := $(APP_SRCS_C:%.c=$(BUILD_DIR)/%.o) \
        $(TINYALSA_SRCS:%.c=$(BUILD_DIR)/%.o) \
        $(CJSON_SRCS:%.c=$(BUILD_DIR)/%.o) \
        $(LVGL_SRCS:%.c=$(BUILD_DIR)/%.o) \
        $(APP_SRCS_CPP:%.cpp=$(BUILD_DIR)/%.o) \
        $(APP_SRCS_CC:%.cc=$(BUILD_DIR)/%.o)

# 7. 编译规则
.PHONY: all clean check

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	@echo "LINKING: $@"
	@$(CXX) $(OBJS) -o $@ $(LDFLAGS)
	@echo "✅ BUILD SUCCESS! Output: $@"

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "CC   $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "CXX  $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.cc
	@mkdir -p $(dir $@)
	@echo "CXX  $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

check:
	@echo ">>> STARTING HEADER & SYNTAX CHECK <<<"
	@for file in $(APP_SRCS_C); do \
		echo "[CHECK C]   $$file"; \
		$(CC) $(CFLAGS) -fsyntax-only $$file || exit 1; \
	done
	@echo ">>> ✅ CHECK PASSED <<<"

clean:
	@echo "CLEANING..."
	@rm -rf product/build product/bin