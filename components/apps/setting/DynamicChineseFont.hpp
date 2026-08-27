#pragma once

#include "lvgl.h"

namespace DynamicChineseFont {

bool begin(const lv_font_t *fallback);
bool isAvailable(void);
const lv_font_t *get(void);

}