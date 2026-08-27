#pragma once
#include <cstdint>
using TaskHandle_t = void *;
using BaseType_t = int;
using UBaseType_t = unsigned;
#define pdPASS 1
#define pdMS_TO_TICKS(ms) (ms)