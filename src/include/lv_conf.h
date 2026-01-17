/**
 * @file lv_conf.h
 * RV1106 精简版配置 (适配 Widget Demo)
 */

/* clang-format off */
#if 1 /* 必须为 1 以启用此文件 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   🎨 颜色设置
 *====================*/

/* 颜色深度: 16位 (RGB565) - 对应你的屏幕硬件 */
#define LV_COLOR_DEPTH 16

/* 字节交换: 0
 * 因为我们在 fbdev.c 里手动处理了颜色转换，这里保持 0 即可 */
#define LV_COLOR_16_SWAP 0

/* 透明色: 纯绿 (通常用于色键扣像，一般用不到) */
#define LV_COLOR_CHROMA_KEY lv_color_hex(0x00ff00)

/*=========================
   🧠 内存设置
 *=========================*/

/* 1: 使用自定义 malloc, 0: 使用 LVGL 内置内存池 */
#define LV_MEM_CUSTOM 0

#if LV_MEM_CUSTOM == 0
    /* 分配给 LVGL 的堆内存大小
     * 128KB (128 * 1024) 足够运行复杂的 Widget Demo */
    #define LV_MEM_SIZE (128U * 1024U)

    /* 内存池地址 (0 = 自动分配) */
    #define LV_MEM_ADR 0
#else
    #define LV_MEM_CUSTOM_INCLUDE <stdlib.h>
    #define LV_MEM_CUSTOM_ALLOC   malloc
    #define LV_MEM_CUSTOM_FREE    free
    #define LV_MEM_CUSTOM_REALLOC realloc
#endif

/* 渲染缓冲区数量 (建议 16) */
#define LV_MEM_BUF_MAX_NUM 16

/*====================
   ⚙️ 基础硬件抽象层 (HAL)
 *====================*/

/* 屏幕默认刷新周期 (ms) */
#define LV_DISP_DEF_REFR_PERIOD 30

/* 输入设备读取周期 (ms) */
#define LV_INDEV_DEF_READ_PERIOD 30

/* 自定义心跳: 0
 * 已经在 main.c 中使用 pthread 线程调用 lv_tick_inc() */
#define LV_TICK_CUSTOM 0
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())
#endif

/* 屏幕 DPI (每英寸像素点)，影响默认控件大小 */
#define LV_DPI_DEF 130

/*=======================
   功能配置
 *=======================*/

/* 启用高级绘图引擎 (阴影、圆角、渐变等必须开启) */
#define LV_DRAW_COMPLEX 1

#if LV_DRAW_COMPLEX != 0
    /* 阴影缓存大小 (0 = 禁用) */
    #define LV_SHADOW_CACHE_SIZE 0
    /* 圆角抗锯齿缓存大小 */
    #define LV_CIRCLE_CACHE_SIZE 4
#endif

/* 图片缓存数量 (0 = 禁用) */
#define LV_IMG_CACHE_DEF_SIZE 0

/*==================
   字体设置
 *===================*/

/* 启用 Montserrat 14号字体 (作为默认字体) */
#define LV_FONT_MONTSERRAT_14 1

/* 设置默认字体 */
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/*==================
    组件开关
 *================*/

/* 启用基本控件 (按钮、标签、滑动条等) */
#define LV_USE_ARC        1
#define LV_USE_BAR        1
#define LV_USE_BTN        1
#define LV_USE_BTNMATRIX  1
#define LV_USE_CANVAS     1
#define LV_USE_CHECKBOX   1
#define LV_USE_DROPDOWN   1
#define LV_USE_IMG        1
#define LV_USE_LABEL      1
#define LV_USE_LINE       1
#define LV_USE_ROLLER     1
#define LV_USE_SLIDER     1
#define LV_USE_SWITCH     1
#define LV_USE_TEXTAREA   1
#define LV_USE_TABLE      1

/* 额外组件 */
#define LV_USE_ANIMIMG    1
#define LV_USE_CALENDAR   1
#define LV_USE_CHART      1
#define LV_USE_COLORWHEEL 1
#define LV_USE_IMGBTN     1
#define LV_USE_KEYBOARD   1
#define LV_USE_LED        1
#define LV_USE_LIST       1
#define LV_USE_MENU       1
#define LV_USE_METER      1
#define LV_USE_MSGBOX     1
#define LV_USE_SPAN       1
#define LV_USE_SPINBOX    1
#define LV_USE_SPINNER    1
#define LV_USE_TABVIEW    1
#define LV_USE_TILEVIEW   1
#define LV_USE_WIN        1

/*===================
   Demo 演示
 ====================*/

/* 启用 Widget Demo (必须为 1) */
#define LV_USE_DEMO_WIDGETS 1
#if LV_USE_DEMO_WIDGETS
    #define LV_DEMO_WIDGETS_SLIDESHOW 0
#endif

/* 启用示例构建 */
#define LV_BUILD_EXAMPLES 1

/*=================
日志调试
 *=================*/

/* 启用日志 (调试阶段建议开启) */
#define LV_USE_LOG 1
#if LV_USE_LOG
    /* 日志级别: WARN (警告) */
    #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
    #define LV_LOG_PRINTF 0 /* 0: 需要自定义回调 (我们在Linux通常不需要) */
#endif

/*=====================
   💻 编译器兼容性设置
 *====================*/
/* 以下宏用于防止编译器警告，保持默认即可 */
#define LV_ATTRIBUTE_TICK_INC
#define LV_ATTRIBUTE_TIMER_HANDLER
#define LV_ATTRIBUTE_FLUSH_READY
#define LV_ATTRIBUTE_MEM_ALIGN_SIZE 1
#define LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_LARGE_CONST
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY
#define LV_ATTRIBUTE_FAST_MEM
#define LV_ATTRIBUTE_DMA
#define LV_EXPORT_CONST_INT(int_value) struct _silence_gcc_warning
#define LV_USE_LARGE_COORD 0

#endif /*LV_CONF_H*/

#endif /*End of "Content enable"*/