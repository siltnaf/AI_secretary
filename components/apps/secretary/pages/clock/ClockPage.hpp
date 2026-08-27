#pragma once

#include "lvgl.h"

namespace SecretaryClockPage {

lv_obj_t *create(lv_obj_t *parent);
void update(lv_obj_t *clock);

}