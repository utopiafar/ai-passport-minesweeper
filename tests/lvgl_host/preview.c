#include "minesweeper_ui.h"
#include "bsp_display_rounding.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static uint16_t framebuffer[240 * 320];
static uint16_t draw_buffer[240 * 32];
static ms_app_t app;
static lv_obj_t *screen;

static void flush(lv_display_t *display, const lv_area_t *area, uint8_t *pixels) {
    uint16_t *src = (uint16_t *)pixels;
    for (int y = area->y1; y <= area->y2; ++y) {
        for (int x = area->x1; x <= area->x2; ++x) {
            uint16_t pixel = *src++;
            framebuffer[y * 240 + x] = bsp_display_pixel_outside_rounded_rect(x, y, 240, 320, 30) ? 0 : pixel;
        }
    }
    lv_display_flush_ready(display);
}

static void snapshot(const char *dir, const char *name) {
    ms_ui_refresh(screen, 78, true);
    lv_refr_now(NULL);
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s.ppm", dir, name);
    FILE *file = fopen(path, "wb");
    assert(file);
    fprintf(file, "P6\n240 320\n255\n");
    for (unsigned i = 0; i < 240 * 320; ++i) {
        uint16_t p = framebuffer[i];
        unsigned char rgb[] = {(unsigned char)(((p >> 11) & 31) * 255 / 31),
                               (unsigned char)(((p >> 5) & 63) * 255 / 63),
                               (unsigned char)((p & 31) * 255 / 31)};
        fwrite(rgb, 1, 3, file);
    }
    fclose(file);
    lv_mem_monitor_t monitor;
    lv_mem_monitor(&monitor);
    printf("%s: LVGL pool free=%zu / %zu bytes, peak=%zu\n", name,
           monitor.free_size, monitor.total_size, monitor.max_used);
    assert(monitor.free_size > 2048);
}

int main(int argc, char **argv) {
    assert(argc == 2);
    lv_init();
    lv_display_t *display = lv_display_create(240, 320);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, draw_buffer, NULL, sizeof(draw_buffer), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    assert(ms_ui_check_fonts());
    puts("LVGL glyph descriptors: PASS (both fonts, negative probe)");
    ms_app_init(&app, 13, 0);
    screen = ms_ui_create(&app);
    lv_screen_load(screen);
    snapshot(argv[1], "ready");
    ms_app_action(&app, MS_REVEAL, 1000);
    ms_app_tick(&app, 13000);
    snapshot(argv[1], "playing");
    app.page = MS_MENU_PAGE;
    snapshot(argv[1], "menu");
    app.page = MS_LEVEL_PAGE;
    snapshot(argv[1], "difficulty");
    app.page = MS_HELP_PAGE;
    snapshot(argv[1], "help");
    app.page = MS_CONFIRM_PAGE;
    snapshot(argv[1], "confirm");
    app.page = MS_GAME_PAGE;
    for (int y = 0; y < app.game.height; ++y) {
        for (int x = 0; x < app.game.width; ++x) {
            if (!(ms_cell(&app.game, x, y) & (MS_OPEN | MS_MINE))) ms_reveal(&app.game, x, y);
        }
    }
    snapshot(argv[1], "won");
    ms_app_init(&app, 13, 0);
    ms_reveal(&app.game, 0, 0);
    for (int i = 0; i < app.game.width * app.game.height; ++i) {
        if (!(app.game.cells[i] & (MS_MINE | MS_OPEN))) {
            ms_toggle_flag(&app.game, i % app.game.width, i / app.game.width);
            break;
        }
    }
    for (int i = 0; i < app.game.width * app.game.height; ++i) {
        if (app.game.cells[i] & MS_MINE) {
            ms_reveal(&app.game, i % app.game.width, i / app.game.width);
            app.x = i % app.game.width; app.y = i / app.game.width;
            break;
        }
    }
    snapshot(argv[1], "lost");
    ms_init(&app.game, MS_EXPERT, 41);
    app.x = 29; app.y = 15; app.view_x = 21; app.view_y = 7;
    ms_reveal(&app.game, app.x, app.y);
    snapshot(argv[1], "expert");
    // Maximum wrong-flag drawing load in a visible viewport, and <-99 counter.
    for (int i = 0; i < app.game.width * app.game.height; ++i)
        if (!(app.game.cells[i] & MS_OPEN)) ms_toggle_flag(&app.game, i % app.game.width, i / app.game.width);
    for (int i = 0; i < app.game.width * app.game.height; ++i) {
        if (app.game.cells[i] & MS_MINE) {
            ms_toggle_flag(&app.game, i % app.game.width, i / app.game.width);
            ms_reveal(&app.game, i % app.game.width, i / app.game.width);
            break;
        }
    }
    app.view_x = app.view_y = app.x = app.y = 0;
    snapshot(argv[1], "overflagged-lost");
    // Stress screen redraws and page changes; fixed pool must not leak.
    lv_mem_monitor_t before, after;
    lv_mem_monitor(&before);
    for (int i = 0; i < 300; ++i) {
        app.page = (ms_page_t)(i % 5);
        ms_ui_refresh(screen, -1, true);
        lv_refr_now(NULL);
    }
    lv_mem_monitor(&after);
    assert(after.free_size == before.free_size);
    ms_ui_refresh(screen, -1, false);
    lv_refr_now(NULL);
    puts("LVGL rendering/page stress: PASS (300 redraws, no pool leak)");
    lv_display_delete(display);
    lv_deinit();
    return 0;
}
