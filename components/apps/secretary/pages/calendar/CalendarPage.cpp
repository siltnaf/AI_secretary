#include <ctime>

#include "CalendarPage.hpp"

namespace SecretaryCalendarPage {

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
    lv_obj_t *calendar = lv_calendar_create(parent);
    lv_obj_set_size(calendar, lv_pct(100), lv_pct(100));
    lv_calendar_set_today_date(calendar, 2026, 8, 25);

    time_t now = time(nullptr);
    struct tm local_time {};
    getLocalTime(now, &local_time);
    const int year = local_time.tm_year + 1900;
    const int month = local_time.tm_mon + 1;
    const int day = local_time.tm_mday;
    lv_calendar_set_today_date(calendar, year, month, day);
    lv_calendar_set_showed_date(calendar, year, month);
    lv_calendar_date_t highlighted[] = {{static_cast<uint16_t>(year), static_cast<int8_t>(month), static_cast<int8_t>(day)}};
    lv_calendar_set_highlighted_dates(calendar, highlighted, 1);
    lv_obj_set_style_text_font(calendar, &lv_font_montserrat_16, 0);
    return calendar;
}

}