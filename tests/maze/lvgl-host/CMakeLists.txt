cmake_minimum_required(VERSION 3.12)
project(maze_ui_refresh_test C)
# SDK作为现有只读依赖传入，不下载或复制到仓库。
if(NOT DEFINED LVGL_DIR)
    message(FATAL_ERROR "Pass -DLVGL_DIR=<existing SDK external/lvgl_v9>")
endif()
get_filename_component(REPO "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
file(GLOB_RECURSE LVGL_SOURCES "${LVGL_DIR}/src/*.c")
add_library(lvgl_host STATIC ${LVGL_SOURCES})
target_include_directories(lvgl_host PUBLIC "${LVGL_DIR}")
target_compile_definitions(lvgl_host PUBLIC LV_CONF_SKIP LV_KCONFIG_IGNORE
    LV_FONT_MONTSERRAT_16=1 "LV_FONT_DEFAULT=&lv_font_montserrat_16")
add_executable(ui-refresh "${REPO}/tests/maze/ui_refresh_harness.c"
    "${REPO}/src/maze_ui.c" "${REPO}/src/maze_app.c"
    "${REPO}/src/maze_model.c" "${REPO}/src/maze_level.c" "${REPO}/src/maze_font_zh_16.c")
target_include_directories(ui-refresh PRIVATE "${REPO}/tests/maze" "${REPO}/src"
    "${LVGL_DIR}/../../middleware/lvgl")
target_compile_options(ui-refresh PRIVATE -std=c99 -Wall -Wextra -Werror)
target_link_libraries(ui-refresh PRIVATE lvgl_host m)
