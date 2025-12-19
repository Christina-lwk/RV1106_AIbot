#include <unistd.h>

#include "lvgl.h"
#include "app/app_manager.h"
#include "app/home_app.h"
#include "ui/lvgl_port.h"

int main(void)
{
    /* 1. 初始化 LVGL + framebuffer */
    lvgl_port_init();

    /* 2. 初始化 App 系统 */
    app_manager_init();

    /* 3. 启动 Home App */
    app_manager_start(&home_app);

    /* 4. 主循环 */
    while (1) {
        lv_timer_handler();
        app_manager_loop();
        usleep(5000);
    }

    return 0;
}
