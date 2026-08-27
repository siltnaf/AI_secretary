#pragma once
using nvs_handle_t = int;
using esp_err_t = int;
#define ESP_OK 0
#define NVS_READONLY 0
#define NVS_READWRITE 1
inline esp_err_t nvs_open(const char *, int, nvs_handle_t *) { return -1; }