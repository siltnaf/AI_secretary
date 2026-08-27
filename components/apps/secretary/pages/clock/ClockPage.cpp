#include <ctime>

#include "ClockPage.hpp"

namespace SecretaryClockPage {

static void getLocalTime(time_t now, struct tm *result)
{
#ifdef _WIN32
    localtime_s(result, &now);
#else
    localtime_r(&now, result);
#endif
}

lv_obj_t *create(lv_obj_t *parent)
{
    lv_obj_t *clock = lv_label_create(parent);
    lv_obj_set_style_text_font(clock, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(clock, lv_color_hex(0x16697A), 0);
    lv_obj_center(clock);
    update(clock);
    return clock;
}

void update(lv_obj_t *clock)
{
    if (clock == nullptr) {
        return;
    }
    time_t now = time(nullptr);
    struct tm local_time {};
    getLocalTime(now, &local_time);
    char value[48];
    std::strftime(value, sizeof(value), "%H:%M:%S\n%a, %d %b", &local_time);
    lv_label_set_text(clock, value);
    lv_obj_set_style_text_align(clock, LV_TEXT_ALIGN_CENTER, 0);
}

}