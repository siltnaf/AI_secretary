#pragma once

#include "lvgl.h"

namespace AppLocale {

void begin(void);
void setChinese(bool enabled);
bool isChinese(void);
void applyNow(void);

}

#ifdef __cplusplus
extern "C" {
#endif

bool AppLocale_IsChinese(void);
void AppLocale_Apply(lv_obj_t *root);

#ifdef __cplusplus
}
#endif