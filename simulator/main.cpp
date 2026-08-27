#include <cstdint>
#include <cstring>

#include "lvgl.h"
#include "sdl/sdl.h"

#include "secretary/SecretaryApp.hpp"

extern "C" {
LV_IMG_DECLARE(img_app_calculator);
LV_IMG_DECLARE(img_app_setting);
LV_IMG_DECLARE(img_app_img_display);
LV_IMG_DECLARE(img_app_video_player);
LV_IMG_DECLARE(img_app_2048);
LV_IMG_DECLARE(img_app_music_player);
LV_IMG_DECLARE(ui_img_sls_logo_png);
LV_IMG_DECLARE(esp_brookesia_image_small_app_launcher_default_98_98);
LV_IMG_DECLARE(ui_img_wifi_png);
LV_IMG_DECLARE(ui_img_bluetooth_png);
LV_IMG_DECLARE(ui_img_sound_png);
LV_IMG_DECLARE(ui_img_light_png);
LV_IMG_DECLARE(ui_img_about_png);
LV_IMG_DECLARE(ui_img_return_png);
LV_IMG_DECLARE(ui_img_arrow_png);
}

namespace {

SecretaryApp secretary;

enum class LauncherApp : uintptr_t {
    Calculator,
    Settings,
    Image,
    Video,
    Game2048,
    Music,
    Squareline,
    Secretary,
};

struct LauncherItem {
    LauncherApp app;
    const lv_img_dsc_t *icon;
    const char *name;
};

const LauncherItem ITEMS[] = {
    {LauncherApp::Calculator, &img_app_calculator, "Calculator"},
    {LauncherApp::Settings, &img_app_setting, "Settings"},
    {LauncherApp::Image, &img_app_img_display, "Image"},
    {LauncherApp::Video, &img_app_video_player, "Video Player"},
    {LauncherApp::Game2048, &img_app_2048, "2048 Game"},
    {LauncherApp::Music, &img_app_music_player, "Music Player"},
    {LauncherApp::Squareline, &ui_img_sls_logo_png, "Squareline"},
    {LauncherApp::Secretary, &esp_brookesia_image_small_app_launcher_default_98_98, "Secretary"},
};

void showLauncher(void * = nullptr);
void showSettings(void * = nullptr);

void showSettingsSection(lv_event_t *event)
{
    const char *section = static_cast<const char *>(lv_event_get_user_data(event));
    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xE5F3FF), 0);

    lv_obj_t *header = lv_obj_create(lv_scr_act());
    lv_obj_set_size(header, lv_pct(100), 72);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x202A33), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_t *back = lv_btn_create(header);
    lv_obj_set_size(back, 46, 46);
    lv_obj_align(back, LV_ALIGN_LEFT_MID, 14, 0);
    lv_obj_add_event_cb(back, [](lv_event_t *) { lv_async_call(showSettings, nullptr); }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *back_label = lv_label_create(back);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, section);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 76, 0);

    lv_obj_t *content = lv_obj_create(lv_scr_act());
    lv_obj_set_size(content, 430, 620);
    lv_obj_align(content, LV_ALIGN_BOTTOM_MID, 0, -24);
    lv_obj_set_style_bg_color(content, lv_color_hex(0xF3F4F0), 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 20, 0);

    if (std::strcmp(section, "Audio") == 0) {
        lv_obj_t *volume_label = lv_label_create(content);
        lv_label_set_text(volume_label, "Volume");
        lv_obj_set_style_text_font(volume_label, &lv_font_montserrat_20, 0);
        lv_obj_align(volume_label, LV_ALIGN_TOP_LEFT, 0, 12);
        lv_obj_t *volume = lv_slider_create(content);
        lv_obj_set_width(volume, 270);
        lv_obj_align(volume, LV_ALIGN_TOP_RIGHT, 0, 12);
        lv_slider_set_value(volume, 60, LV_ANIM_OFF);

        lv_obj_t *voice = lv_dropdown_create(content);
        lv_dropdown_set_options(voice, "Bella\nJasper\nLuna\nBruno\nRosie\nHugo\nKiki\nLeo");
        lv_obj_set_width(voice, lv_pct(100));
        lv_obj_align(voice, LV_ALIGN_TOP_LEFT, 0, 90);
        lv_dropdown_set_selected(voice, 1);
        lv_obj_t *voice_title = lv_label_create(content);
        lv_label_set_text(voice_title, "TTS Voice");
        lv_obj_set_style_text_font(voice_title, &lv_font_montserrat_18, 0);
        lv_obj_align_to(voice_title, voice, LV_ALIGN_OUT_TOP_LEFT, 0, -8);

        lv_obj_t *test = lv_btn_create(content);
        lv_obj_set_size(test, 180, 52);
        lv_obj_align(test, LV_ALIGN_TOP_LEFT, 0, 190);
        lv_obj_set_style_bg_color(test, lv_color_hex(0x16697A), 0);
        lv_obj_set_style_radius(test, 4, 0);
        lv_obj_t *test_label = lv_label_create(test);
        lv_label_set_text(test_label, "Audio Test");
        lv_obj_center(test_label);
    } else if (std::strcmp(section, "Display") == 0) {
        lv_obj_t *brightness_label = lv_label_create(content);
        lv_label_set_text(brightness_label, "Brightness");
        lv_obj_set_style_text_font(brightness_label, &lv_font_montserrat_20, 0);
        lv_obj_align(brightness_label, LV_ALIGN_TOP_LEFT, 0, 12);
        lv_obj_t *brightness = lv_slider_create(content);
        lv_obj_set_width(brightness, 270);
        lv_obj_align(brightness, LV_ALIGN_TOP_RIGHT, 0, 12);
        lv_slider_set_value(brightness, 80, LV_ANIM_OFF);
        lv_obj_t *language = lv_dropdown_create(content);
        lv_dropdown_set_options(language, "English\nChinese");
        lv_obj_set_width(language, lv_pct(100));
        lv_obj_align(language, LV_ALIGN_TOP_LEFT, 0, 100);
        lv_obj_t *language_title = lv_label_create(content);
        lv_label_set_text(language_title, "Language");
        lv_obj_set_style_text_font(language_title, &lv_font_montserrat_18, 0);
        lv_obj_align_to(language_title, language, LV_ALIGN_OUT_TOP_LEFT, 0, -8);
    } else {
        lv_obj_t *message = lv_label_create(content);
        lv_label_set_text_fmt(message, "%s\n\nThis simulator page uses the same settings hierarchy as the P4 application.\n\n4G is reserved for the future modem module.", section);
        lv_label_set_long_mode(message, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(message, lv_pct(100));
        lv_obj_set_style_text_font(message, &lv_font_montserrat_20, 0);
        lv_obj_align(message, LV_ALIGN_TOP_LEFT, 0, 20);
    }
}

void openPlaceholder(lv_event_t *event)
{
    LauncherApp app = static_cast<LauncherApp>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(event)));
    if (app == LauncherApp::Settings) {
        lv_async_call(showSettings, nullptr);
        return;
    }
    if (app == LauncherApp::Secretary) {
        lv_async_call([](void *) { secretary.run(); }, nullptr);
        return;
    }

    const LauncherItem *selected = nullptr;
    for (const LauncherItem &item : ITEMS) {
        if (item.app == app) selected = &item;
    }
    if (selected == nullptr) return;

    lv_async_call([](void *data) {
        const LauncherItem *item = static_cast<const LauncherItem *>(data);
        lv_obj_clean(lv_scr_act());
        lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xE5F3FF), 0);
        lv_obj_t *bar = lv_obj_create(lv_scr_act());
        lv_obj_set_size(bar, lv_pct(100), 72);
        lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x202A33), 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_radius(bar, 0, 0);
        lv_obj_t *back = lv_btn_create(bar);
        lv_obj_set_size(back, 46, 46);
        lv_obj_align(back, LV_ALIGN_LEFT_MID, 14, 0);
        lv_obj_add_event_cb(back, [](lv_event_t *) { lv_async_call(showLauncher, nullptr); }, LV_EVENT_CLICKED, nullptr);
        lv_obj_t *back_label = lv_label_create(back);
        lv_label_set_text(back_label, LV_SYMBOL_LEFT);
        lv_obj_center(back_label);
        lv_obj_t *title = lv_label_create(bar);
        lv_label_set_text(title, item->name);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(title, lv_color_white(), 0);
        lv_obj_align(title, LV_ALIGN_LEFT_MID, 76, 0);
        lv_obj_t *icon = lv_img_create(lv_scr_act());
        lv_img_set_src(icon, item->icon);
        lv_img_set_zoom(icon, item->icon->header.w > 112 ? 192 : 256);
        lv_obj_align(icon, LV_ALIGN_CENTER, 0, -50);
        lv_obj_t *message = lv_label_create(lv_scr_act());
        lv_label_set_text(message, "This tile represents the corresponding P4 application.\nThe Secretary tile runs the shared firmware UI source.");
        lv_label_set_long_mode(message, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(message, 390);
        lv_obj_set_style_text_align(message, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(message, &lv_font_montserrat_18, 0);
        lv_obj_align(message, LV_ALIGN_CENTER, 0, 70);
    }, const_cast<LauncherItem *>(selected));
}

void showLauncher(void *)
{
    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xE5F3FF), 0);
    lv_obj_t *header = lv_obj_create(lv_scr_act());
    lv_obj_set_size(header, lv_pct(100), 84);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x202A33), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "ESP-Brookesia Apps");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_center(title);

    lv_obj_t *grid = lv_obj_create(lv_scr_act());
    lv_obj_set_size(grid, 440, 650);
    lv_obj_align(grid, LV_ALIGN_BOTTOM_MID, 0, -25);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 6, 0);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(grid, 18, 0);

    for (const LauncherItem &item : ITEMS) {
        lv_obj_t *button = lv_btn_create(grid);
        lv_obj_set_size(button, 200, 135);
        lv_obj_set_style_bg_color(button, lv_color_white(), 0);
        lv_obj_set_style_border_color(button, lv_color_hex(0xB7C6CC), 0);
        lv_obj_set_style_border_width(button, 1, 0);
        lv_obj_set_style_radius(button, 5, 0);
        lv_obj_add_event_cb(button, openPlaceholder, LV_EVENT_CLICKED, reinterpret_cast<void *>(item.app));
        lv_obj_t *symbol = lv_img_create(button);
        lv_img_set_src(symbol, item.icon);
        lv_img_set_zoom(symbol, item.icon->header.w > 112 ? 154 : 174);
        lv_obj_align(symbol, LV_ALIGN_TOP_MID, 0, 8);
        lv_obj_t *name = lv_label_create(button);
        lv_label_set_text(name, item.name);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_18, 0);
        lv_obj_align(name, LV_ALIGN_BOTTOM_MID, 0, -17);
    }
}

void showSettings(void *)
{
    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xE5F3FF), 0);
    lv_obj_t *header = lv_obj_create(lv_scr_act());
    lv_obj_set_size(header, lv_pct(100), 84);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x202A33), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_t *back = lv_btn_create(header);
    lv_obj_set_size(back, 46, 46);
    lv_obj_align(back, LV_ALIGN_LEFT_MID, 14, 0);
    lv_obj_add_event_cb(back, [](lv_event_t *) { lv_async_call(showLauncher, nullptr); }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *back_label = lv_label_create(back);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Settings");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 76, 0);

    lv_obj_t *list = lv_obj_create(lv_scr_act());
    lv_obj_set_size(list, 430, 650);
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(list, lv_color_hex(0xF3F4F0), 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 12, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list, 8, 0);

    const char *items[] = {"Content URL", "Wi-Fi", "Audio", "Display", "4G Transport", "SD Card", "Playlist Cache", "About Device"};
    for (const char *item : items) {
        lv_obj_t *row = lv_btn_create(list);
        lv_obj_set_width(row, lv_pct(100));
        lv_obj_set_height(row, 60);
        lv_obj_set_style_bg_color(row, lv_color_white(), 0);
        lv_obj_set_style_border_color(row, lv_color_hex(0xCCD4D7), 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_radius(row, 4, 0);
        if (std::strcmp(item, "Audio") == 0 || std::strcmp(item, "Display") == 0) {
            lv_obj_add_event_cb(row, showSettingsSection, LV_EVENT_CLICKED, const_cast<char *>(item));
        }
        const lv_img_dsc_t *icon_source = nullptr;
        if (std::strcmp(item, "Wi-Fi") == 0) icon_source = &ui_img_wifi_png;
        else if (std::strcmp(item, "Audio") == 0) icon_source = &ui_img_sound_png;
        else if (std::strcmp(item, "Display") == 0) icon_source = &ui_img_light_png;
        else if (std::strcmp(item, "About Device") == 0) icon_source = &ui_img_about_png;
        else if (std::strcmp(item, "Content URL") == 0) icon_source = &ui_img_arrow_png;
        else if (std::strcmp(item, "SD Card") == 0) icon_source = &ui_img_arrow_png;
        if (icon_source != nullptr) {
            lv_obj_t *icon = lv_img_create(row);
            lv_img_set_src(icon, icon_source);
            lv_obj_set_size(icon, 42, 42);
            lv_img_set_zoom(icon, icon_source->header.w > 42 ? 128 : 256);
            lv_obj_align(icon, LV_ALIGN_LEFT_MID, 12, 0);
        }
        lv_obj_t *label = lv_label_create(row);
        lv_label_set_text(label, item);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 66, 0);
        lv_obj_t *value = lv_label_create(row);
        lv_label_set_text(value, std::strcmp(item, "4G Transport") == 0 ? "Not installed" : LV_SYMBOL_RIGHT);
        lv_obj_set_style_text_font(value, &lv_font_montserrat_16, 0);
        lv_obj_align(value, LV_ALIGN_RIGHT_MID, -16, 0);
    }
}

}

int main()
{
    lv_init();
    sdl_init();
    static lv_color_t buffer[480 * 40];
    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, buffer, nullptr, sizeof(buffer) / sizeof(buffer[0]));
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 480;
    disp_drv.ver_res = 800;
    disp_drv.flush_cb = sdl_display_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = sdl_mouse_read;
    lv_indev_drv_register(&indev_drv);
    ESP_Brookesia_PhoneApp::setCloseHandler([]() {
        lv_async_call(showLauncher, nullptr);
    });
    secretary.init();
    showLauncher();
    while (true) {
        lv_tick_inc(5);
        lv_timer_handler();
        SDL_Delay(5);
    }
    return 0;
}