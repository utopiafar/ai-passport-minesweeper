#pragma once
#include "lvgl.h"
#include "minesweeper.h"

/* Call only from the LVGL task or while holding the BSP LVGL lock. The app
 * object must outlive this screen. All screen content uses explicit fonts. */
lv_obj_t *ms_ui_create(const ms_app_t *app);
void ms_ui_refresh(lv_obj_t *screen, int battery_soc, bool input_ok);
bool ms_ui_check_fonts(void);
