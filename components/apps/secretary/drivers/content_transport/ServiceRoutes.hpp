#pragma once

#include <cstddef>

#include "secretary/pages/PageCatalog.hpp"

namespace SecretaryServiceRoutes {

bool build(SecretaryPage page, const char *content_url, char *buffer, size_t buffer_size);
bool isRemoteService(SecretaryPage page);

}