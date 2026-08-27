#include "LibraryPage.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "cJSON.h"
#include "secretary/drivers/content_transport/ContentTransport.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "setting/ChineseFont.hpp"

namespace {

constexpr size_t ITEMS_PER_PAGE = 10;
constexpr lv_coord_t LIST_TOP = 38;
constexpr char TAG[] = "SecretaryLibrary";

struct DetailRequest {
    int id;
    char baseUrl[256];
    lv_obj_t *modal;
};

struct DetailCompletion {
    int id;
    lv_obj_t *modal;
    SecretaryContentTransport::Result result;
};

void styleText(lv_obj_t *label, lv_coord_t size)
{
    lv_obj_set_width(label, lv_pct(100));
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(label, lv_color_hex(0x263238), 0);
    const lv_font_t *font = ChineseFont::getDynamic();
    lv_obj_set_style_text_font(label, font != nullptr
        ? font : (size >= 20 ? &lv_font_montserrat_20 : &lv_font_montserrat_16), 0);
}

lv_obj_t *makePanel(lv_obj_t *parent)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_size(panel, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_pad_all(panel, 6, 0);
    return panel;
}

void closeModal(lv_event_t *event)
{
    lv_obj_t *modal = static_cast<lv_obj_t *>(lv_event_get_user_data(event));
    if (modal != nullptr && lv_obj_is_valid(modal)) lv_obj_del(modal);
}

lv_obj_t *createModal(const char *status)
{
    lv_obj_t *modal = lv_obj_create(lv_layer_top());
    lv_obj_set_size(modal, lv_pct(94), lv_pct(90));
    lv_obj_center(modal);
    lv_obj_set_style_bg_color(modal, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(modal, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(modal, lv_color_hex(0x16697A), 0);
    lv_obj_set_style_border_width(modal, 2, 0);
    lv_obj_set_style_radius(modal, 4, 0);
    lv_obj_set_style_pad_all(modal, 14, 0);

    lv_obj_t *label = lv_label_create(modal);
    lv_label_set_text(label, status);
    styleText(label, 20);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    return modal;
}

void showDetail(void *argument)
{
    DetailCompletion *completion = static_cast<DetailCompletion *>(argument);
    lv_obj_t *modal = completion->modal;
    if (modal == nullptr || !lv_obj_is_valid(modal)) {
        SecretaryContentTransport::release(&completion->result);
        delete completion;
        return;
    }
    lv_obj_clean(modal);

    lv_obj_t *close = lv_btn_create(modal);
    lv_obj_set_size(close, 44, 44);
    lv_obj_align(close, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(close, lv_color_hex(0x344552), 0);
    lv_obj_set_style_radius(close, 4, 0);
    lv_obj_add_event_cb(close, closeModal, LV_EVENT_CLICKED, modal);
    lv_obj_t *closeLabel = lv_label_create(close);
    lv_label_set_text(closeLabel, LV_SYMBOL_CLOSE);
    lv_obj_center(closeLabel);

    if (!completion->result.ok) {
        lv_obj_t *error = lv_label_create(modal);
        lv_label_set_text(error, "Book content fetch failed.");
        styleText(error, 20);
        lv_obj_align(error, LV_ALIGN_TOP_LEFT, 0, 12);
        SecretaryContentTransport::release(&completion->result);
        delete completion;
        return;
    }

    cJSON *document = cJSON_ParseWithLength(completion->result.data, completion->result.size);
    cJSON *titleValue = document == nullptr ? nullptr : cJSON_GetObjectItemCaseSensitive(document, "title");
    cJSON *authorValue = document == nullptr ? nullptr : cJSON_GetObjectItemCaseSensitive(document, "author");
    cJSON *categoryValue = document == nullptr ? nullptr : cJSON_GetObjectItemCaseSensitive(document, "category");
    cJSON *contentValue = document == nullptr ? nullptr : cJSON_GetObjectItemCaseSensitive(document, "content");
    const char *title = cJSON_IsString(titleValue) ? titleValue->valuestring : "Untitled";
    const char *author = cJSON_IsString(authorValue) ? authorValue->valuestring : "";
    const char *category = cJSON_IsString(categoryValue) ? categoryValue->valuestring : "";
    const char *content = cJSON_IsString(contentValue) ? contentValue->valuestring : "";
    ESP_LOGI(TAG, "detail id=%d title=%s content_bytes=%u", completion->id, title,
             static_cast<unsigned>(std::strlen(content)));

    lv_obj_t *heading = lv_label_create(modal);
    lv_label_set_text(heading, title);
    styleText(heading, 20);
    lv_obj_set_width(heading, lv_pct(100) - 60);
    lv_obj_align(heading, LV_ALIGN_TOP_LEFT, 0, 6);

    char metadata[160];
    std::snprintf(metadata, sizeof(metadata), "%s%s%s", author,
                  author[0] && category[0] ? "  |  " : "", category);
    lv_obj_t *meta = lv_label_create(modal);
    lv_label_set_text(meta, metadata);
    styleText(meta, 16);
    lv_obj_set_style_text_color(meta, lv_color_hex(0x52606D), 0);
    lv_obj_align(meta, LV_ALIGN_TOP_LEFT, 0, 46);

    lv_obj_t *reader = lv_obj_create(modal);
    lv_obj_set_size(reader, lv_pct(100), lv_pct(100) - 82);
    lv_obj_align(reader, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(reader, lv_color_hex(0xF7F8F6), 0);
    lv_obj_set_style_border_width(reader, 0, 0);
    lv_obj_set_style_radius(reader, 2, 0);
    lv_obj_set_scroll_dir(reader, LV_DIR_VER);
    lv_obj_t *body = lv_label_create(reader);
    lv_label_set_text(body, content[0] ? content : "This book has no content.");
    styleText(body, 20);
    lv_obj_set_width(body, lv_pct(100));
    lv_obj_align(body, LV_ALIGN_TOP_LEFT, 0, 0);

    cJSON_Delete(document);
    SecretaryContentTransport::release(&completion->result);
    delete completion;
}

void fetchDetailTask(void *argument)
{
    DetailRequest *request = static_cast<DetailRequest *>(argument);
    char endpoint[320];
    std::snprintf(endpoint, sizeof(endpoint), "%s/api/books/%d", request->baseUrl, request->id);
    ESP_LOGI(TAG, "detail request id=%d endpoint=%s", request->id, endpoint);
    SecretaryContentTransport::Result result{};
    for (int attempt = 0; attempt < 2 && !result.ok; ++attempt) {
        result = SecretaryContentTransport::get(endpoint);
        if (!result.ok && attempt == 0) vTaskDelay(pdMS_TO_TICKS(500));
    }
    DetailCompletion *completion = new DetailCompletion{request->id, request->modal, result};
    delete request;
    lv_async_call(showDetail, completion);
    vTaskDelete(nullptr);
}

void openBook(lv_event_t *event)
{
    DetailRequest *source = static_cast<DetailRequest *>(lv_event_get_user_data(event));
    if (source == nullptr) return;
    DetailRequest *request = new DetailRequest(*source);
    request->modal = createModal("Loading book...");
    if (xTaskCreate(fetchDetailTask, "Book detail", 8192, request, 3, nullptr) != pdPASS) {
        lv_obj_t *modal = request->modal;
        delete request;
        if (modal != nullptr && lv_obj_is_valid(modal)) {
            lv_obj_clean(modal);
            lv_obj_t *error = lv_label_create(modal);
            lv_label_set_text(error, "Unable to start book request.");
            styleText(error, 20);
            lv_obj_center(error);
        }
    }
}

void releaseRow(lv_event_t *event)
{
    delete static_cast<DetailRequest *>(lv_event_get_user_data(event));
}

void addBookRow(lv_obj_t *parent, const char *contentUrl, size_t index, const char *title, int id)
{
    lv_obj_t *row = lv_btn_create(parent);
    lv_obj_set_size(row, lv_pct(100), 38);
    lv_obj_align(row, LV_ALIGN_TOP_LEFT, 0, LIST_TOP + static_cast<lv_coord_t>(index * 40));
    lv_obj_set_style_bg_color(row, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_color(row, lv_color_hex(0xCCD4D7), 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_radius(row, 2, 0);
    DetailRequest *request = new DetailRequest{};
    request->id = id;
    request->modal = nullptr;
    std::strncpy(request->baseUrl, contentUrl == nullptr ? "" : contentUrl, sizeof(request->baseUrl) - 1);
    char *api = std::strstr(request->baseUrl + (std::strncmp(request->baseUrl, "https://", 8) == 0 ? 8 : 7), "/api/");
    if (api != nullptr) *api = '\0';
    while (request->baseUrl[0] && request->baseUrl[std::strlen(request->baseUrl) - 1] == '/') {
        request->baseUrl[std::strlen(request->baseUrl) - 1] = '\0';
    }
    lv_obj_add_event_cb(row, openBook, LV_EVENT_CLICKED, request);
    lv_obj_add_event_cb(row, releaseRow, LV_EVENT_DELETE, request);
    char text[192];
    std::snprintf(text, sizeof(text), "%u. %s  (#%d)", static_cast<unsigned>(index + 1),
                  title == nullptr || title[0] == '\0' ? "Untitled" : title, id);
    lv_obj_t *label = lv_label_create(row);
    lv_label_set_text(label, text);
    styleText(label, 16);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 10, 0);
}

}

namespace SecretaryLibraryPage {

lv_obj_t *create(lv_obj_t *parent, const char *)
{
    lv_obj_t *panel = makePanel(parent);
    lv_obj_t *status = lv_label_create(panel);
    lv_label_set_text(status, "Fetching booklist...");
    styleText(status, 20);
    lv_obj_align(status, LV_ALIGN_TOP_LEFT, 0, 8);
    return panel;
}

void update(lv_obj_t *parent, const char *content_url, const char *payload, size_t payload_size)
{
    ESP_LOGI(TAG, "update parent=%p payload=%p bytes=%u", parent, payload,
             static_cast<unsigned>(payload_size));
    if (parent == nullptr) return;
    lv_obj_clean(parent);
    lv_obj_t *panel = makePanel(parent);
    if (payload == nullptr || payload_size == 0) {
        lv_obj_t *error = lv_label_create(panel);
        lv_label_set_text(error, "No booklist received.");
        styleText(error, 20);
        lv_obj_align(error, LV_ALIGN_TOP_LEFT, 0, 8);
        return;
    }

    cJSON *document = cJSON_ParseWithLength(payload, payload_size);
    cJSON *items = document == nullptr ? nullptr : cJSON_GetObjectItemCaseSensitive(document, "items");
    if (!cJSON_IsArray(items)) {
        ESP_LOGE(TAG, "response is not an items array parse=%p", document);
        lv_obj_t *error = lv_label_create(panel);
        lv_label_set_text(error, "Booklist response has no items.");
        styleText(error, 20);
        lv_obj_align(error, LV_ALIGN_TOP_LEFT, 0, 8);
        cJSON_Delete(document);
        return;
    }

    const cJSON *total = cJSON_GetObjectItemCaseSensitive(document, "total");
    char heading[64];
    std::snprintf(heading, sizeof(heading), "Books: %d", cJSON_IsNumber(total) ? total->valueint : cJSON_GetArraySize(items));
    lv_obj_t *title = lv_label_create(panel);
    lv_label_set_text(title, heading);
    styleText(title, 20);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    const size_t count = static_cast<size_t>(cJSON_GetArraySize(items));
    ESP_LOGI(TAG, "parsed total=%d items=%u", cJSON_IsNumber(total) ? total->valueint : -1,
             static_cast<unsigned>(count));
    const size_t visible = count < ITEMS_PER_PAGE ? count : ITEMS_PER_PAGE;
    for (size_t index = 0; index < visible; ++index) {
        cJSON *item = cJSON_GetArrayItem(items, static_cast<int>(index));
        cJSON *itemTitle = cJSON_IsObject(item) ? cJSON_GetObjectItemCaseSensitive(item, "title") : nullptr;
        cJSON *itemId = cJSON_IsObject(item) ? cJSON_GetObjectItemCaseSensitive(item, "id") : nullptr;
        ESP_LOGI(TAG, "item[%u] id=%d title=%s", static_cast<unsigned>(index),
                 cJSON_IsNumber(itemId) ? itemId->valueint : 0,
                 cJSON_IsString(itemTitle) ? itemTitle->valuestring : "<missing>");
        addBookRow(panel, content_url, index, cJSON_IsString(itemTitle) ? itemTitle->valuestring : "Untitled",
                   cJSON_IsNumber(itemId) ? itemId->valueint : 0);
    }
    if (count == 0) {
        lv_obj_t *empty = lv_label_create(panel);
        lv_label_set_text(empty, "No books found.");
        styleText(empty, 20);
        lv_obj_align(empty, LV_ALIGN_TOP_LEFT, 0, 48);
    }
    cJSON_Delete(document);
}

void updateStatus(lv_obj_t *parent, const char *message)
{
    if (parent == nullptr) return;
    lv_obj_clean(parent);
    lv_obj_t *panel = makePanel(parent);
    lv_obj_t *status = lv_label_create(panel);
    lv_label_set_text(status, message == nullptr ? "Booklist unavailable." : message);
    styleText(status, 20);
    lv_obj_align(status, LV_ALIGN_TOP_LEFT, 0, 8);
}

}