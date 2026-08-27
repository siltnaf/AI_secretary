#include <cstring>

#ifdef SECRETARY_SIMULATOR
#include <fstream>
#include <string>
#else
#include "nvs.h"
#endif

#include "ContentStore.hpp"

namespace {

constexpr char NVS_NAMESPACE[] = "secretary";
constexpr char NVS_KEY_URL[] = "content_url";
constexpr char DEFAULT_URL[] = "https://example.com";

}

namespace SecretaryContentStore {

bool loadUrl(char *buffer, size_t buffer_size)
{
#ifdef SECRETARY_SIMULATOR
    std::ifstream file("secretary_content_url.txt");
    std::string value;
    if (file) {
        std::getline(file, value);
    }
    if (value.empty()) value = DEFAULT_URL;
    std::strncpy(buffer, value.c_str(), buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
    return true;
#else
    if (buffer == nullptr || buffer_size == 0) {
        return false;
    }

    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) {
        std::strncpy(buffer, DEFAULT_URL, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        return true;
    }

    size_t required_size = buffer_size;
    esp_err_t err = nvs_get_str(handle, NVS_KEY_URL, buffer, &required_size);
    nvs_close(handle);
    if (err != ESP_OK || buffer[0] == '\0') {
        std::strncpy(buffer, DEFAULT_URL, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
    }
    return true;
#endif
}

bool saveUrl(const char *url)
{
#ifdef SECRETARY_SIMULATOR
    std::ofstream file("secretary_content_url.txt", std::ios::trunc);
    file << url;
    return static_cast<bool>(file);
#else
    if (url == nullptr || url[0] == '\0') {
        return false;
    }

    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) {
        return false;
    }

    esp_err_t err = nvs_set_str(handle, NVS_KEY_URL, url);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err == ESP_OK;
#endif
}

}