#include "home_app.h"
#include "lvgl.h"

static lv_obj_t *label = NULL;

static void home_init(void)
{
    label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello Echo-Mate");
    lv_obj_center(label);
}

static void home_enter(void)
{
    lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
}

static void home_exit(void)
{
    lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
}

static void home_loop(void)
{
    /* v0.1 什么都不做 */
}

app_t home_app = {
    .name  = "home",
    .init  = home_init,
    .enter = home_enter,
    .exit  = home_exit,
    .loop  = home_loop,
};
