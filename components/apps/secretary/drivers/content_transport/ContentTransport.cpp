#include <cstdlib>
#include <cstring>

#ifdef SECRETARY_SIMULATOR
#include "ContentTransport.hpp"

namespace SecretaryContentTransport {
Result get(const char *) { return {}; }
void release(Result *result) { if (result) { free(result->data); *result = {}; } }
}
#else
#include "esp_http_client.h"
#include "esp_heap_caps.h"
#include "esp_wifi.h"

#include "ContentTransport.hpp"

namespace {

struct DownloadContext {
    char *data;
    size_t size;
    bool overflow;
};

esp_err_t onHttpEvent(esp_http_client_event_t *event)
{
    if (event->event_id != HTTP_EVENT_ON_DATA || event->user_data == nullptr) {
        return ESP_OK;
    }

    DownloadContext *context = static_cast<DownloadContext *>(event->user_data);
    if (context->size + event->data_len >= SecretaryContentTransport::MAX_CONTENT_BYTES) {
        context->overflow = true;
        return ESP_FAIL;
    }

    std::memcpy(context->data + context->size, event->data, event->data_len);
    context->size += event->data_len;
    return ESP_OK;
}

}

namespace SecretaryContentTransport {

Result get(const char *url)
{
    Result result{};
    if (url == nullptr || (std::strncmp(url, "http://", 7) != 0 && std::strncmp(url, "https://", 8) != 0)) {
        return result;
    }

    wifi_ap_record_t access_point{};
    if (esp_wifi_sta_get_ap_info(&access_point) != ESP_OK) {
        return result;
    }

    DownloadContext context{};
    context.data = static_cast<char *>(heap_caps_malloc(MAX_CONTENT_BYTES + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (context.data == nullptr) {
        context.data = static_cast<char *>(malloc(MAX_CONTENT_BYTES + 1));
    }
    if (context.data == nullptr) {
        return result;
    }

    esp_http_client_config_t config{};
    config.url = url;
    config.timeout_ms = 15000;
    config.event_handler = onHttpEvent;
    config.user_data = &context;
    config.buffer_size = 2048;
    config.buffer_size_tx = 1024;
    config.disable_auto_redirect = false;
    config.keep_alive_enable = false;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == nullptr) {
        free(context.data);
        return result;
    }

    esp_err_t err = esp_http_client_perform(client);
    int status_code = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    if (err != ESP_OK || context.overflow || status_code < 200 || status_code >= 300) {
        free(context.data);
        return result;
    }

    context.data[context.size] = '\0';
    result.data = context.data;
    result.size = context.size;
    result.status_code = status_code;
    result.ok = true;
    return result;
}

void release(Result *result)
{
    if (result == nullptr) {
        return;
    }
    free(result->data);
    *result = {};
}

}
#endif