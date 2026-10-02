cmake_minimum_required(VERSION 3.16)
project(minesweeper_preview C CXX)
if(NOT LVGL_SOURCE_DIR)
    message(FATAL_ERROR "Set LVGL_SOURCE_DIR to an LVGL v9.5.0 checkout")
endif()
set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_LIST_DIR}/lv_conf.h" CACHE PATH "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory("${LVGL_SOURCE_DIR}" lvgl)
add_executable(minesweeper_preview
    preview.c ../../main/minesweeper.c ../../main/minesweeper_ui.c
    ../../assets/fonts/ms_font_12.c ../../assets/fonts/ms_font_14.c
    ../../components/bsp/src/bsp_display_rounding.c)
target_include_directories(minesweeper_preview PRIVATE ../../main ../../components/bsp/src)
target_link_libraries(minesweeper_preview PRIVATE lvgl m)
target_compile_options(minesweeper_preview PRIVATE -Wall -Wextra -Werror)
