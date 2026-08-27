#pragma once

#include <cstddef>

namespace SecretaryContentTransport {

constexpr size_t MAX_CONTENT_BYTES = 48 * 1024;

struct Result {
    char *data;
    size_t size;
    int status_code;
    bool ok;
};

Result get(const char *url);
void release(Result *result);

}