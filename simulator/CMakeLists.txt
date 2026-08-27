cmake_minimum_required(VERSION 3.20)
project(ai_secretary_simulator LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_path(SDL2_INCLUDE_DIR SDL2/SDL.h PATHS C:/msys64/mingw64/include)
find_library(SDL2_LIBRARY NAMES SDL2 SDL2.dll PATHS C:/msys64/mingw64/lib C:/msys64/mingw64/bin REQUIRED)

add_library(lvgl STATIC)
file(GLOB_RECURSE LVGL_SOURCES CONFIGURE_DEPENDS
    vendor/lvgl/src/*.c
)
target_sources(lvgl PRIVATE ${LVGL_SOURCES})
target_include_directories(lvgl PUBLIC . vendor/lvgl vendor/lvgl/src)
target_compile_definitions(lvgl PUBLIC LV_CONF_INCLUDE_SIMPLE)

add_library(lv_drivers STATIC vendor/lv_drivers/sdl/sdl.c vendor/lv_drivers/sdl/sdl_common.c)
target_include_directories(lv_drivers PUBLIC vendor vendor/lv_drivers vendor/lvgl vendor/lvgl/src)
target_include_directories(lv_drivers PUBLIC ${SDL2_INCLUDE_DIR})
target_link_libraries(lv_drivers PUBLIC lvgl ${SDL2_LIBRARY})

add_executable(ai_secretary_simulator main.cpp)
target_compile_definitions(ai_secretary_simulator PRIVATE SDL_MAIN_HANDLED)
file(GLOB_RECURSE SECRETARY_SOURCES CONFIGURE_DEPENDS
    ../components/apps/secretary/*.cpp
)
target_sources(ai_secretary_simulator PRIVATE ${SECRETARY_SOURCES})
target_sources(ai_secretary_simulator PRIVATE
    ../components/apps/calculator/assets/img_app_calculator.c
    ../components/apps/setting/assets/img_app_setting.c
    ../components/apps/image_display/assets/img_app_img_display.c
    ../components/apps/video_player/assets/img_app_video_player.c
    ../components/apps/game_2048/assets/img_app_2048.c
    ../components/apps/music_player/assets/img_app_music_player.c
    ../local_components/esp-brookesia/src/app_examples/phone/squareline/src/ui/images/ui_img_sls_logo_png.c
    ../local_components/esp-brookesia/src/assets/images/small/esp_brookesia_image_small_app_launcher_default_98_98.c
    assets/setting/images/ui_img_wifi_png.c
    assets/setting/images/ui_img_bluetooth_png.c
    assets/setting/images/ui_img_sound_png.c
    assets/setting/images/ui_img_light_png.c
    assets/setting/images/ui_img_about_png.c
    assets/setting/images/ui_img_return_png.c
    assets/setting/images/ui_img_arrow_png.c
)
target_compile_definitions(ai_secretary_simulator PRIVATE SECRETARY_SIMULATOR)
target_include_directories(ai_secretary_simulator PRIVATE
    . shims ../components/apps vendor vendor/lvgl vendor/lvgl/src vendor/lv_drivers)
target_include_directories(ai_secretary_simulator PRIVATE ${SDL2_INCLUDE_DIR})
target_link_libraries(ai_secretary_simulator PRIVATE lv_drivers lvgl ${SDL2_LIBRARY})

if (MINGW)
    add_custom_command(TARGET ai_secretary_simulator POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "C:/msys64/mingw64/bin/SDL2.dll"
            "$<TARGET_FILE_DIR:ai_secretary_simulator>/SDL2.dll")
endif()