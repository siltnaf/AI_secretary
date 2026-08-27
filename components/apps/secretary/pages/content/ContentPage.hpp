#pragma once

#include "lvgl.h"

namespace SecretaryContentPage {

lv_obj_t *create(lv_obj_t *parent, const char *title, const char *description, const char *action_label);

}