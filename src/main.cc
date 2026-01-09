#include <unistd.h>
#include <stdio.h>
#include <string>
#include <iostream>

extern "C" {
    #include "lvgl.h"
    #include "app/app_manager.h"
    #include "app/home_app.h"
    #include "ui/lvgl_port.h"
}

#include "services/network/NetworkClient.h"
#include "app/AI_chat/chat_app.h" // 确保路径正确

int main(void)
{
    printf("========== Echo-Mate System Booting ==========\n");

    /* 1. 初始化 LVGL */
    lvgl_port_init();

    /* 2. 初始化 App 系统 */
    app_manager_init();

    /* 4. 启动 UI 主页 */
    app_manager_start(&home_app);

    /* 5. 初始化 AI 核心 (它内部会自动创建 AudioProcess) */
    printf(">>> [Main] Initializing ChatApp Core...\n");
    ChatApp robot; 
    robot.Init(); 

    printf(">>> [Main] Entering System Loop...\n");

    /* 6. 主循环 */
    while (1) {
        lv_timer_handler();      // UI
        app_manager_loop();      // 页面逻辑
        robot.RunOnce();         // AI 状态机
        usleep(5000);            // 5ms 休眠
    }

    return 0;
}