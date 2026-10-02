#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "minesweeper.h"
#include "minesweeper_ui.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static const char *TAG = "minesweeper";
static ms_app_t s_app;
static lv_obj_t *s_screen;
static QueueHandle_t s_inputs;
static bool s_input_ok;

typedef struct { bsp_btn_t button; bsp_btn_ev_t event; } input_t;

/* Shared button timer callbacks never touch UI, ADC, storage or blocking I/O. */
static void on_key(bsp_btn_t button, bsp_btn_ev_t event, void *user) {
    (void)user;
    const input_t input = {button, event};
    if (s_inputs) (void)xQueueSend(s_inputs, &input, 0);
}

static bool translate(const input_t *input, ms_action_t *action) {
    if (input->event == BSP_BTN_PRESS) *action = MS_WAKE;
    else if (input->button == BSP_BTN_OK) {
        if (input->event == BSP_BTN_CLICK) *action = MS_REVEAL;
        else if (input->event == BSP_BTN_DOUBLE) *action = MS_TOGGLE_FLAG;
        else if (input->event == BSP_BTN_LONG) *action = MS_MENU;
        else return false;
    } else if (input->event == BSP_BTN_CLICK || input->event == BSP_BTN_DOUBLE) {
        // A rapid directional double click still moves; OK double is flag only.
        *action = input->button == BSP_BTN_UP ? MS_PREV : MS_NEXT;
    } else if (input->event == BSP_BTN_LONG) {
        *action = input->button == BSP_BTN_UP ? MS_ROW_UP : MS_ROW_DOWN;
    } else return false;
    return true;
}

static void game_task(void *arg) {
    (void)arg;
    int battery = -1;
    uint64_t battery_at = 0;
    uint8_t brightness = 255;
    uint16_t shown_seconds = 0;
    ms_page_t shown_page = MS_GAME_PAGE;
    bool shown_idle = false;
    bool dirty = true;
    for (;;) {
        input_t input;
        bool received = xQueueReceive(s_inputs, &input, pdMS_TO_TICKS(100)) == pdTRUE;
        uint64_t now = (uint64_t)esp_timer_get_time() / 1000;
        // I2C polling belongs to this worker, outside the LVGL lock.
        if (now >= battery_at) {
            int next = bsp_battery_soc();
            dirty |= next != battery;
            battery = next;
            battery_at = now + 30000;
        }
        if (!bsp_lvgl_lock(1000)) continue;
        ms_app_tick(&s_app, now);
        ms_action_t action;
        if (received && translate(&input, &action)) {
            s_app.next_seed = esp_random() ^ (uint32_t)esp_timer_get_time();
            ms_app_action(&s_app, action, now);
            dirty |= action != MS_WAKE;
        }
        uint16_t seconds = ms_app_seconds(&s_app);
        dirty |= seconds != shown_seconds || shown_page != s_app.page || shown_idle != s_app.idle;
        if (dirty) {
            ms_ui_refresh(s_screen, battery, s_input_ok);
            shown_seconds = seconds;
            shown_page = s_app.page;
            shown_idle = s_app.idle;
            dirty = false;
        }
        uint8_t next_brightness = ms_app_brightness(&s_app, now);
        bsp_lvgl_unlock();
        if (brightness != next_brightness) {
            bsp_display_backlight(next_brightness);
            brightness = next_brightness;
        }
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Passport Minesweeper 1.0: offline classic / three buttons");
    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(TAG, "Display initialization failed");
        return;
    }
    if (bsp_battery_init() != ESP_OK) ESP_LOGW(TAG, "Battery reading unavailable");
    s_inputs = xQueueCreate(16, sizeof(input_t));
    if (!s_inputs) { ESP_LOGE(TAG, "Input queue allocation failed"); return; }
    s_input_ok = bsp_button_init(on_key, NULL) == ESP_OK;
    if (!s_input_ok) ESP_LOGE(TAG, "Button initialization failed");
    ms_app_init(&s_app, esp_random(), (uint64_t)esp_timer_get_time() / 1000);
    if (!bsp_lvgl_lock(1000)) { ESP_LOGE(TAG, "LVGL lock failed"); return; }
    if (!ms_ui_check_fonts()) {
        ESP_LOGE(TAG, "Font coverage check failed");
        bsp_lvgl_unlock();
        return;
    }
    s_screen = ms_ui_create(&s_app);
    lv_screen_load(s_screen);
    ms_ui_refresh(s_screen, -1, s_input_ok);
    bsp_lvgl_unlock();
    bsp_display_backlight(80);
    if (xTaskCreate(game_task, "minesweeper", 6144, NULL, 4, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Game worker allocation failed");
        if (bsp_lvgl_lock(1000)) {
            ms_ui_refresh(s_screen, -1, false);
            bsp_lvgl_unlock();
        }
        return;
    }
    ESP_LOGI(TAG, "Ready; fonts=PASS; free heap=%u; largest block=%u",
             (unsigned)esp_get_free_heap_size(),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}
