# toolchain.mk
# 定义交叉编译工具链

CC = arm-rockchip830-linux-uclibcgnueabihf-gcc
CXX = arm-rockchip830-linux-uclibcgnueabihf-g++
LD = arm-rockchip830-linux-uclibcgnueabihf-ld
AR = arm-rockchip830-linux-uclibcgnueabihf-ar
STRIP = arm-rockchip830-linux-uclibcgnueabihf-strip

# 可以在这里添加特定于编译器的全局标志
# TARGET_ARCH = -mcpu=cortex-a7 -mfloat-abi=hard
