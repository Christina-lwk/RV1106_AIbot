#include <unistd.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <vector> // 新增

extern "C" {
    #include "lvgl.h"
    #include "app/app_manager.h"
    #include "app/home_app.h"
    #include "ui/lvgl_port.h"
}

#include "services/network/NetworkClient.h"
#include "services/audio/AudioProcess.h"      // 必须显式引用，因为 main 要获取录音数据
#include "services/wakeword/WakeWordEngine.h" // 新增：唤醒引擎
#include "app/AI_chat/chat_app.h"

// Snowboy 模型路径
#define SNOWBOY_RES   "third_party/snowboy/resources/common.res"
#define SNOWBOY_MODEL "third_party/snowboy/resources/snowboy.umdl"

int main(void)
{
    printf("========== Echo-Mate System Booting ==========\n");

    /* 1. 初始化 LVGL */
    lvgl_port_init();

    /* 2. 初始化 App 系统 */
    app_manager_init();

    /* 3. 启动 UI 主页 */
    app_manager_start(&home_app);

    /* 4. 初始化 系统级服务 (唤醒引擎) */
    printf(">>> [Main] Initializing WakeWord Engine...\n");
    WakeWordEngine wake_engine;
    wake_engine.Init(SNOWBOY_RES, SNOWBOY_MODEL);    

    /* 5. 初始化 AI App (它内部会自动创建 AudioProcess) */
    printf(">>> [Main] Initializing ChatApp Core...\n");
    ChatApp robot; 
    robot.Init(); 

    /* 6. 主循环 */
    printf(">>> [Main] Entering System Loop...\n");
    std::vector<int16_t> audio_frame; // 用于暂存从音频服务拿到的数据

    while (1) {
        /* --- UI 任务 (永远运行) --- */
        lv_timer_handler();
        app_manager_loop();

        /* --- 核心逻辑分流: 桌面模式 vs App模式 --- */
        
        // 假设 robot 提供一个 IsRunning() 方法，判断是否正在对话中
        // 如果你还没写 IsRunning，可以用一个简单的 bool 变量在 main 里控制，或者去 ChatApp 加一个
        if (robot.IsRunning()) {
            robot.RunOnce(); 
        } 
        else {
            if (AudioProcess::GetInstance().GetFrame(audio_frame)) {
                // 喂给唤醒引擎
                int ret = wake_engine.Detect(audio_frame);
                
                if (ret > 0) {
                    printf(">>> Wake Word Detected! Launching Robot... ⚡️ <<<\n");
                    // 启动机器人
                    robot.Start(); 
                    // 唤醒后清空缓冲区，防止"Snowboy"这个词的声音被录进去
                    AudioProcess::GetInstance().ClearBuff();
                }
            }
        }

        usleep(5000); // 5ms 休眠
    }

    return 0;
}