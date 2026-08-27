#pragma once

#include "lvgl.h"

#ifdef __cplusplus
namespace ChineseFont {

void begin(void);
bool isAvailable(void);
const lv_font_t *get(uint16_t size = 20);
const lv_font_t *getDynamic(void);
bool hasGlyph(uint32_t codepoint);

}
#endif

#ifdef __cplusplus
extern "C" {
#endif

const lv_font_t *ChineseFont_Get(void);
bool ChineseFont_IsAvailable(void);

#ifdef __cplusplus
}
#endif