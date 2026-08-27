#pragma once
#include <cstdio>
#define ESP_LOGI(tag, format, ...) std::fprintf(stdout, "I (%s) " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, format, ...) std::fprintf(stdout, "W (%s) " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGE(tag, format, ...) std::fprintf(stderr, "E (%s) " format "\n", tag, ##__VA_ARGS__)