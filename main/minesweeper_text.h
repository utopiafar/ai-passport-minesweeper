#pragma once

/* Fixed UI inventory. The font coverage check scans this and all UI literals. */
#define MS_TITLE "扫雷"
#define MS_HINT_MOVE "上/下移格 长按换行"
#define MS_HINT_ACTION "确定翻格 双击旗 长按菜单"
#define MS_MENU_TITLE "游戏暂停"
#define MS_MENU_HINT "上/下选择 确定进入"
#define MS_BACK_HINT "长按确定返回"
#define MS_HELP_TITLE "玩法与按键"
#define MS_CONFIRM_TITLE "开始新局？"
#define MS_CONFIRM_BODY "当前进度将丢失"
#define MS_CANCEL "取消"
#define MS_CONFIRM "开始新局"

static const char *const ms_level_names[] = { "初级", "中级", "高级" };
static const char *const ms_level_rows[] = {
    "初级   9x9 / 10雷", "中级 16x16 / 40雷", "高级 30x16 / 99雷"
};
static const char *const ms_menu_rows[] = { "继续 / 查看棋盘", "笑脸重开", "选择难度", "玩法与按键" };
static const char *const ms_help_rows[] = {
    "上/下短按：前/后格",
    "上/下长按：上/下行",
    "确定短按：翻开格子",
    "确定双击：插旗/取消",
    "确定长按：暂停菜单",
    "数字 = 周围八格雷数",
    "旗数匹配时按数字",
    "快速翻开周围格子",
    "错旗会触雷，请谨慎",
    "首击及周围八格无雷",
    "翻开全部安全格获胜"
};
