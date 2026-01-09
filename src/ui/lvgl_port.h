/**
 * @file lvgl_port.h
 *
 */

#ifndef LVGL_PORT_H
#define LVGL_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 * INCLUDES
 *********************/
// 关键修复 1: 引入 stdint.h 以识别 uint32_t
#include <stdint.h> 

// 关键修复 2: 引入 lvgl.h 以识别 lv_disp_drv_t, lv_color_t 等类型
#include "lvgl.h"

/**********************
 * GLOBAL PROTOTYPES
 **********************/

// 初始化函数（主程序调用这个）
void lvgl_port_init(void);

// 驱动相关函数声明
// (为了让 .c 文件能正确实现这些函数，头文件必须先声明用到参数类型)
void fbdev_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p);
void fbdev_get_sizes(uint32_t *width, uint32_t *height, uint32_t *dpi);
void fbdev_set_offset(uint32_t xoffset, uint32_t yoffset);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LVGL_PORT_H*/