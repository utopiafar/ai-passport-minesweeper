#include "minesweeper_ui.h"
#include "minesweeper_text.h"
#include "../assets/fonts/minesweeper_codepoints.h"
#include <stdio.h>

LV_FONT_DECLARE(ms_font_12);
LV_FONT_DECLARE(ms_font_14);

#define PAPER 0xc0c0c0
#define WHITE 0xffffff
#define SHADOW 0x808080
#define INK 0x161616
#define NAVY 0x000080
#define BLUE 0x005dcc
#define RED 0xf12d2d
#define BOARD_X 21
#define BOARD_Y 84
#define CELL 22

static const ms_app_t *s_app;
static int s_battery = -1;
static bool s_input_ok = true;

static void box(lv_layer_t *layer, int x, int y, int w, int h, uint32_t color) {
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_color = lv_color_hex(color);
    d.bg_opa = LV_OPA_COVER;
    lv_area_t r = {x, y, x + w - 1, y + h - 1};
    lv_draw_rect(layer, &d, &r);
}

static void outline(lv_layer_t *layer, int x, int y, int w, int h, uint32_t color) {
    box(layer, x, y, w, 1, color);
    box(layer, x, y + h - 1, w, 1, color);
    box(layer, x, y, 1, h, color);
    box(layer, x + w - 1, y, 1, h, color);
}

static void bevel(lv_layer_t *layer, int x, int y, int w, int h, bool raised) {
    uint32_t top = raised ? WHITE : SHADOW, bottom = raised ? SHADOW : WHITE;
    box(layer, x, y, w, h, PAPER);
    box(layer, x, y, w, 2, top);
    box(layer, x, y, 2, h, top);
    box(layer, x, y + h - 2, w, 2, bottom);
    box(layer, x + w - 2, y, 2, h, bottom);
}

static void text(lv_layer_t *layer, int x, int y, int w, const char *value,
                 const lv_font_t *font, uint32_t color, lv_text_align_t align) {
    lv_draw_label_dsc_t d;
    lv_draw_label_dsc_init(&d);
    d.text = value;
    d.font = font;
    d.color = lv_color_hex(color);
    d.align = align;
    d.text_local = 1; // LVGL draw tasks may outlive the stack buffer below.
    lv_area_t r = {x, y, x + w - 1, y + font->line_height - 1};
    lv_draw_label(layer, &d, &r);
}

static void counter(lv_layer_t *layer, int x, int value) {
    static const uint8_t segments[10] = {0x3f, 0x06, 0x5b, 0x4f, 0x66,
                                       0x6d, 0x7d, 0x07, 0x7f, 0x6f};
    static const int geom[7][4] = {
        {2,0,9,2}, {11,2,2,8}, {11,12,2,8}, {2,20,9,2},
        {0,12,2,8}, {0,2,2,8}, {2,10,9,2}
    };
    bevel(layer, x, 35, 56, 31, false);
    box(layer, x + 3, 38, 50, 25, 0x210000);
    bool negative = value < 0;
    if (negative) value = -value;
    if (negative && value > 99) value = 99; // Sign consumes the first digit.
    if (value > 999) value = 999;
    for (int i = 0; i < 3; ++i) {
        int digit = i == 0 ? value / 100 : i == 1 ? (value / 10) % 10 : value % 10;
        uint8_t mask = negative && i == 0 ? 0x40 : segments[digit];
        for (int bit = 0; bit < 7; ++bit) {
            box(layer, x + 5 + i * 16 + geom[bit][0], 39 + geom[bit][1],
                geom[bit][2], geom[bit][3], mask & (1 << bit) ? RED : 0x480505);
        }
    }
}

static void face(lv_layer_t *layer, ms_state_t state) {
    bevel(layer, 102, 35, 36, 31, true);
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.radius = LV_RADIUS_CIRCLE;
    d.bg_color = lv_color_hex(0xffd923);
    d.border_color = lv_color_hex(INK);
    d.border_width = 1;
    lv_area_t r = {108, 38, 131, 61};
    lv_draw_rect(layer, &d, &r);
    if (state == MS_LOST) {
        for (int i = 0; i < 3; ++i) {
            box(layer, 113 + i, 44 + i, 1, 1, INK);
            box(layer, 115 - i, 44 + i, 1, 1, INK);
            box(layer, 123 + i, 44 + i, 1, 1, INK);
            box(layer, 125 - i, 44 + i, 1, 1, INK);
        }
        box(layer, 115, 54, 10, 2, INK);
    } else {
        box(layer, 114, 44, 2, 3, INK);
        box(layer, 124, 44, 2, 3, INK);
        if (state == MS_WON) box(layer, 112, 44, 16, 2, INK);
        box(layer, 114, 52, 2, 2, INK);
        box(layer, 124, 52, 2, 2, INK);
        box(layer, 116, 54, 8, 2, INK);
    }
}

static void flag(lv_layer_t *layer, int x, int y) {
    box(layer, x + 11, y + 5, 2, 12, INK);
    box(layer, x + 5, y + 5, 7, 3, RED);
    box(layer, x + 7, y + 8, 5, 2, RED);
    box(layer, x + 7, y + 17, 10, 2, INK);
}

static void mine(lv_layer_t *layer, int x, int y) {
    box(layer, x + 6, y + 7, 10, 9, INK);
    box(layer, x + 10, y + 3, 2, 17, INK);
    box(layer, x + 3, y + 10, 17, 2, INK);
    box(layer, x + 5, y + 5, 2, 2, INK);
    box(layer, x + 16, y + 16, 2, 2, INK);
    box(layer, x + 7, y + 8, 3, 3, WHITE);
}

static void draw_board(lv_layer_t *layer) {
    static const uint32_t colors[9] = {INK, 0x0000ff, 0x008000, 0xff0000,
                                      0x000080, 0x800000, 0x008080, INK, SHADOW};
    const ms_game_t *g = &s_app->game;
    bevel(layer, BOARD_X - 3, BOARD_Y - 3, 204, 204, false);
    for (int y = 0; y < MS_VIEW_SIZE; ++y) {
        for (int x = 0; x < MS_VIEW_SIZE; ++x) {
            int cx = s_app->view_x + x, cy = s_app->view_y + y;
            int px = BOARD_X + x * CELL, py = BOARD_Y + y * CELL;
            uint8_t c = ms_cell(g, cx, cy);
            bool show_mine = (c & MS_MINE) && g->state == MS_LOST;
            bool wrong_flag = (c & MS_FLAG) && !(c & MS_MINE) && g->state == MS_LOST;
            if ((c & MS_OPEN) || show_mine || wrong_flag) {
                box(layer, px, py, CELL, CELL, g->exploded == cy * g->width + cx ? RED : PAPER);
                outline(layer, px, py, CELL, CELL, 0x999999);
            } else bevel(layer, px, py, CELL, CELL, true);
            if (show_mine) mine(layer, px, py);
            else if (c & MS_FLAG) flag(layer, px, py);
            else if ((c & MS_OPEN) && (c & MS_NUMBER)) {
                char digit[] = {(char)('0' + (c & MS_NUMBER)), '\0'};
                text(layer, px, py + 4, CELL, digit, &lv_font_montserrat_14,
                     colors[c & MS_NUMBER], LV_TEXT_ALIGN_CENTER);
            }
            if (wrong_flag) {
                for (int i = 3; i < 19; ++i) {
                    box(layer, px + i, py + i, 2, 1, RED);
                    box(layer, px + i, py + 21 - i, 2, 1, RED);
                }
            }
            if (cx == s_app->x && cy == s_app->y) {
                outline(layer, px + 2, py + 2, CELL - 4, CELL - 4, BLUE);
                outline(layer, px + 3, py + 3, CELL - 6, CELL - 6, BLUE);
            }
        }
    }
    // Small scroll indicators avoid shrinking the classic cells on large boards.
    if (s_app->view_x > 0) box(layer, 14, 175, 3, 14, NAVY);
    if (s_app->view_x + MS_VIEW_SIZE < g->width) box(layer, 223, 175, 3, 14, NAVY);
    if (s_app->view_y > 0) box(layer, 113, 81, 14, 2, NAVY);
    if (s_app->view_y + MS_VIEW_SIZE < g->height) box(layer, 113, 285, 14, 2, NAVY);
}

static void menu_row(lv_layer_t *layer, int y, const char *value, bool selected) {
    bevel(layer, 27, y, 186, 32, !selected);
    if (selected) box(layer, 30, y + 3, 180, 26, NAVY);
    text(layer, 33, y + 6, 174, value, &ms_font_14,
         selected ? WHITE : INK, LV_TEXT_ALIGN_CENTER);
}

static void draw(lv_event_t *event) {
    lv_layer_t *layer = lv_event_get_layer(event);
    const ms_app_t *a = s_app;
    char buf[80];
    box(layer, 0, 0, 240, 29, NAVY);
    text(layer, 23, 6, 120, MS_TITLE, &ms_font_14, WHITE, LV_TEXT_ALIGN_LEFT);
    snprintf(buf, sizeof(buf), s_battery >= 0 ? "%d%%" : "--%%", s_battery);
    text(layer, 165, 7, 48, buf, &ms_font_12, WHITE, LV_TEXT_ALIGN_RIGHT);
    if (!s_input_ok) {
        text(layer, 25, 115, 190, "按键初始化失败", &ms_font_14, INK, LV_TEXT_ALIGN_CENTER);
        text(layer, 25, 150, 190, "请重启并检查设备", &ms_font_14, INK, LV_TEXT_ALIGN_CENTER);
        return;
    }
    if (a->page == MS_GAME_PAGE) {
        counter(layer, 20, (int)a->game.mines - a->game.flags);
        counter(layer, 164, ms_app_seconds(a));
        face(layer, a->game.state);
        if (a->game.state == MS_WON) snprintf(buf, sizeof(buf), "获胜！长按确定可重开");
        else if (a->game.state == MS_LOST) snprintf(buf, sizeof(buf), "触雷！长按确定可重开");
        else if (a->game.state == MS_READY) snprintf(buf, sizeof(buf), "%s / 首击安全", ms_level_names[a->game.level]);
        else snprintf(buf, sizeof(buf), "%s %02d/%02d行 %02d/%02d列", ms_level_names[a->game.level],
                      a->y + 1, a->game.height, a->x + 1, a->game.width);
        text(layer, 20, 67, 200, buf, &ms_font_12, INK, LV_TEXT_ALIGN_CENTER);
        draw_board(layer);
        text(layer, 20, 287, 200, MS_HINT_MOVE, &ms_font_12, INK, LV_TEXT_ALIGN_CENTER);
        text(layer, 17, 303, 206, MS_HINT_ACTION, &ms_font_12, INK, LV_TEXT_ALIGN_CENTER);
    } else if (a->page == MS_MENU_PAGE) {
        text(layer, 25, 49, 190, a->idle ? "闲置暂停" : MS_MENU_TITLE, &ms_font_14, NAVY, LV_TEXT_ALIGN_CENTER);
        snprintf(buf, sizeof(buf), "%s / %03u秒", ms_level_names[a->game.level], ms_app_seconds(a));
        text(layer, 25, 78, 190, buf, &ms_font_12, INK, LV_TEXT_ALIGN_CENTER);
        for (int i = 0; i < 4; ++i) menu_row(layer, 111 + i * 38, ms_menu_rows[i], a->selection == i);
        text(layer, 20, 279, 200, MS_MENU_HINT, &ms_font_12, INK, LV_TEXT_ALIGN_CENTER);
        text(layer, 20, 299, 200, MS_BACK_HINT, &ms_font_12, INK, LV_TEXT_ALIGN_CENTER);
    } else if (a->page == MS_LEVEL_PAGE) {
        text(layer, 25, 54, 190, "选择难度", &ms_font_14, NAVY, LV_TEXT_ALIGN_CENTER);
        for (int i = 0; i < 3; ++i) menu_row(layer, 104 + i * 43, ms_level_rows[i], a->selection == i);
        text(layer, 20, 257, 200, "大棋盘随光标滚动", &ms_font_12, INK, LV_TEXT_ALIGN_CENTER);
        text(layer, 20, 294, 200, MS_BACK_HINT, &ms_font_12, INK, LV_TEXT_ALIGN_CENTER);
    } else if (a->page == MS_HELP_PAGE) {
        text(layer, 25, 35, 190, MS_HELP_TITLE, &ms_font_14, NAVY, LV_TEXT_ALIGN_CENTER);
        for (unsigned i = 0; i < sizeof(ms_help_rows) / sizeof(ms_help_rows[0]); ++i)
            text(layer, 28, 65 + (int)i * 20, 190, ms_help_rows[i], &ms_font_12, INK, LV_TEXT_ALIGN_LEFT);
        text(layer, 20, 297, 200, "确定返回", &ms_font_12, NAVY, LV_TEXT_ALIGN_CENTER);
    } else {
        text(layer, 25, 76, 190, MS_CONFIRM_TITLE, &ms_font_14, NAVY, LV_TEXT_ALIGN_CENTER);
        text(layer, 25, 112, 190, MS_CONFIRM_BODY, &ms_font_12, INK, LV_TEXT_ALIGN_CENTER);
        menu_row(layer, 163, MS_CANCEL, a->selection == 0);
        menu_row(layer, 206, MS_CONFIRM, a->selection == 1);
        text(layer, 20, 279, 200, "上/下选择 确定确认", &ms_font_12, INK, LV_TEXT_ALIGN_CENTER);
    }
}

bool ms_ui_check_fonts(void) {
    const lv_font_t *fonts[] = { &ms_font_12, &ms_font_14 };
    for (unsigned f = 0; f < 2; ++f) {
        for (unsigned i = 0; i < sizeof(ms_codepoints) / sizeof(ms_codepoints[0]); ++i) {
            lv_font_glyph_dsc_t glyph = {0};
            if (!lv_font_get_glyph_dsc(fonts[f], &glyph, ms_codepoints[i], 0) || glyph.is_placeholder) return false;
        }
        lv_font_glyph_dsc_t missing = {0};
        if (lv_font_get_glyph_dsc(fonts[f], &missing, 0x9f98, 0) && !missing.is_placeholder) return false;
    }
    return true;
}

lv_obj_t *ms_ui_create(const ms_app_t *app) {
    s_app = app;
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(PAPER), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_size(screen, 240, 320);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(screen, draw, LV_EVENT_DRAW_MAIN, NULL);
    return screen;
}

void ms_ui_refresh(lv_obj_t *screen, int battery_soc, bool input_ok) {
    s_battery = battery_soc;
    s_input_ok = input_ok;
    lv_obj_invalidate(screen);
}
