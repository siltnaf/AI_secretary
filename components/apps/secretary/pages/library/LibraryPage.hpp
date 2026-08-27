#pragma once

#include <cstddef>

#include "lvgl.h"

namespace SecretaryLibraryPage {

lv_obj_t *create(lv_obj_t *parent, const char *content_url);
void update(lv_obj_t *parent, const char *content_url, const char *payload, size_t payload_size);
void updateStatus(lv_obj_t *parent, const char *message);

}