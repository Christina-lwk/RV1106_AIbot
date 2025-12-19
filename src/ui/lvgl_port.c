/**
 * @file fbdev.c
 * 合并版：
 * - fbdev 驱动
 * - LVGL init
 * - LVGL tick 线程
 */

#include "lvgl_port.h"
#include "lv_drv_conf.h"
#include "evdev.h"

#if USE_FBDEV || USE_BSD_FBDEV

#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <string.h>

#ifndef FBDEV_PATH
#define FBDEV_PATH  "/dev/fb0"
#endif

#ifndef DIV_ROUND_UP
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#endif

/* =========================
 * Framebuffer globals
 * ========================= */
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
static char *fbp = NULL;
static long int screensize = 0;
static int fbfd = -1;

/* =========================
 * LVGL tick thread
 * ========================= */
static void *lv_tick_thread(void *data)
{
    (void)data;
    while (1) {
        lv_tick_inc(5);
        usleep(5000);
    }
    return NULL;
}

/* =========================
 * FBDEV init
 * ========================= */
void fbdev_init(void)
{
    fbfd = open(FBDEV_PATH, O_RDWR);
    if (fbfd < 0) {
        perror("Error: cannot open framebuffer device");
        return;
    }

    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        perror("Error reading fixed information");
        return;
    }

    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        perror("Error reading variable information");
        return;
    }

    printf("FBDEV: %dx%d %dbpp line_len=%u\n",
           vinfo.xres, vinfo.yres,
           vinfo.bits_per_pixel,
           finfo.line_length);

    screensize = finfo.line_length * vinfo.yres;

    fbp = (char *)mmap(0, screensize,
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED, fbfd, 0);

    if (fbp == MAP_FAILED) {
        perror("Error: mmap framebuffer failed");
        fbp = NULL;
        return;
    }

    printf("FBDEV Init OK: mapped %ld bytes\n", screensize);
}

/* =========================
 * FBDEV exit
 * ========================= */
void fbdev_exit(void)
{
    if (fbp && fbp != MAP_FAILED) {
        munmap(fbp, screensize);
    }
    if (fbfd >= 0) {
        close(fbfd);
    }
}

/* =========================
 * FBDEV flush
 * ========================= */
void fbdev_flush(lv_disp_drv_t *drv,
                 const lv_area_t *area,
                 lv_color_t *color_p)
{
    if (!fbp ||
        area->x2 < 0 || area->y2 < 0 ||
        area->x1 >= (int32_t)vinfo.xres ||
        area->y1 >= (int32_t)vinfo.yres) {
        lv_disp_flush_ready(drv);
        return;
    }

    int32_t x1 = area->x1 < 0 ? 0 : area->x1;
    int32_t y1 = area->y1 < 0 ? 0 : area->y1;
    int32_t x2 = area->x2 >= (int32_t)vinfo.xres ? vinfo.xres - 1 : area->x2;
    int32_t y2 = area->y2 >= (int32_t)vinfo.yres ? vinfo.yres - 1 : area->y2;

    int w = x2 - x1 + 1;

    if (vinfo.bits_per_pixel == 32) {
        uint32_t *fb32 = (uint32_t *)fbp;

        for (int y = y1; y <= y2; y++) {
            long offset = (x1 + vinfo.xoffset)
                        + (y + vinfo.yoffset) * (finfo.line_length / 4);

#if LV_COLOR_DEPTH == 32
            memcpy(&fb32[offset], color_p, w * 4);
#else
            for (int i = 0; i < w; i++) {
                lv_color_t c = color_p[i];
                uint8_t r = (c.ch.red   * 255) / 31;
                uint8_t g = (c.ch.green * 255) / 63;
                uint8_t b = (c.ch.blue  * 255) / 31;
                fb32[offset + i] = 0xFF000000 | (r << 16) | (g << 8) | b;
            }
#endif
            color_p += w;
        }
    }
    else if (vinfo.bits_per_pixel == 16) {
        uint16_t *fb16 = (uint16_t *)fbp;

        for (int y = y1; y <= y2; y++) {
            long offset = (x1 + vinfo.xoffset)
                        + (y + vinfo.yoffset) * (finfo.line_length / 2);
            memcpy(&fb16[offset], color_p, w * 2);
            color_p += w;
        }
    }

    lv_disp_flush_ready(drv);
}

/* =========================
 * LVGL PORT INIT（重点）
 * ========================= */
void lvgl_port_init(void)
{
    /* 1. init lvgl core */
    lv_init();

    /* 2. init framebuffer */
    fbdev_init();

    /* 3. draw buffer */
    static lv_color_t buf1[1280 * 20];
    static lv_color_t buf2[1280 * 20];
    static lv_disp_draw_buf_t draw_buf;

    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, 1280 * 20);

    /* 4. display driver */
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);

    disp_drv.draw_buf = &draw_buf;
    disp_drv.flush_cb = fbdev_flush;
    disp_drv.hor_res  = vinfo.xres;
    disp_drv.ver_res  = vinfo.yres;

    lv_disp_drv_register(&disp_drv);

    /* 5. tick thread */
    pthread_t tid;
    pthread_create(&tid, NULL, lv_tick_thread, NULL);

    printf("LVGL port init done\n");
}

void fbdev_get_sizes(uint32_t *width, uint32_t *height, uint32_t *dpi)
{
    if (width)  *width  = vinfo.xres;
    if (height) *height = vinfo.yres;
    if (dpi && vinfo.width)
        *dpi = DIV_ROUND_UP(vinfo.xres * 254, vinfo.width * 10);
}

void fbdev_set_offset(uint32_t xoffset, uint32_t yoffset)
{
    vinfo.xoffset = xoffset;
    vinfo.yoffset = yoffset;
}

#endif /* USE_FBDEV */
