// 全局应用管理器 (负责切换 Home <-> 各种 App)

#include <stddef.h>
#include "app_manager.h"

static app_t *current_app = NULL;

void app_manager_init(void)
{
    /* 预留：将来做 app 注册 */
}

void app_manager_start(app_t *app)
{
    if(current_app && current_app->exit)
        current_app->exit();

    current_app = app;

    if(current_app->init)
        current_app->init();

    if(current_app->enter)
        current_app->enter();
}

void app_manager_loop(void)
{
    if(current_app && current_app->loop)
        current_app->loop();
}

