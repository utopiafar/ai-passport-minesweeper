idf_component_register(
    SRCS "main.c" "minesweeper.c" "minesweeper_ui.c"
         "../assets/fonts/ms_font_12.c" "../assets/fonts/ms_font_14.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_hw_support esp_timer
)
