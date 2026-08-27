#pragma once
#include "FreeRTOS.h"
using TaskFunction_t = void (*)(void *);
inline BaseType_t xTaskCreate(TaskFunction_t, const char *, unsigned, void *, UBaseType_t, TaskHandle_t *) { return pdPASS; }
inline void vTaskDelete(TaskHandle_t) {}
inline void vTaskDelay(unsigned) {}