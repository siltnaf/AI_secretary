#pragma once

#include <cstddef>

namespace SecretaryContentStore {

bool loadUrl(char *buffer, size_t buffer_size);
bool saveUrl(const char *url);

}