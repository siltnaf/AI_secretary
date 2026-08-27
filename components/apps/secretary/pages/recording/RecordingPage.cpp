#include "RecordingPage.hpp"

#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>

#include "bsp/esp-bsp.h"
#include "bsp_board_extra.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

constexpr char DIRECTORY[] = BSP_SD_MOUNT_POINT "/recordings";
constexpr uint32_t SAMPLE_RATE = 44100;
constexpr uint16_t CHANNELS = 2;
constexpr uint16_t BITS = 16;
const char *TAG = "SecretaryRecorder";

struct State {
    lv_obj_t *status;
    lv_obj_t *buttonLabel;
    lv_obj_t *files;
    volatile bool recording;
    volatile bool stop;
    TaskHandle_t task;
};

void writeU16(FILE *file, uint16_t value) { std::fwrite(&value, sizeof(value), 1, file); }
void writeU32(FILE *file, uint32_t value) { std::fwrite(&value, sizeof(value), 1, file); }

void writeHeader(FILE *file, uint32_t bytes)
{
    std::fseek(file, 0, SEEK_SET);
    std::fwrite("RIFF", 1, 4, file); writeU32(file, bytes + 36);
    std::fwrite("WAVEfmt ", 1, 8, file); writeU32(file, 16); writeU16(file, 1);
    writeU16(file, CHANNELS); writeU32(file, SAMPLE_RATE);
    writeU32(file, SAMPLE_RATE * CHANNELS * BITS / 8); writeU16(file, CHANNELS * BITS / 8);
    writeU16(file, BITS); std::fwrite("data", 1, 4, file); writeU32(file, bytes);
}

void refreshFiles(State *state)
{
    if (state == nullptr || state->files == nullptr) return;
    char listing[512] = {};
    DIR *directory = opendir(DIRECTORY);
    if (directory != nullptr) {
        struct dirent *entry;
        while ((entry = readdir(directory)) != nullptr) {
            if (entry->d_name[0] == '.') continue;
            if (std::strlen(listing) + std::strlen(entry->d_name) + 3 >= sizeof(listing)) break;
            std::strcat(listing, entry->d_name);
            std::strcat(listing, "\n");
        }
        closedir(directory);
    }
    lv_label_set_text(state->files, listing[0] ? listing : "No recordings");
}

void complete(void *argument)
{
    State *state = static_cast<State *>(argument);
    if (state == nullptr) return;
    state->recording = false;
    state->task = nullptr;
    if (state->status != nullptr) lv_label_set_text(state->status, "Recording saved to SD card");
    if (state->buttonLabel != nullptr) lv_label_set_text(state->buttonLabel, LV_SYMBOL_AUDIO " Record");
    refreshFiles(state);
}

void recordTask(void *argument)
{
    State *state = static_cast<State *>(argument);
    mkdir(DIRECTORY, 0775);
    char path[128] = {};
    std::snprintf(path, sizeof(path), "%s/recording-%lu.wav", DIRECTORY,
                  static_cast<unsigned long>(xTaskGetTickCount()));
    FILE *file = std::fopen(path, "wb");
    uint32_t bytes = 0;
    if (file != nullptr) {
        uint8_t header[44] = {};
        std::fwrite(header, 1, sizeof(header), file);
        uint8_t buffer[2048];
        while (!state->stop) {
            size_t read = 0;
            if (bsp_extra_i2s_read(buffer, sizeof(buffer), &read, 100) == ESP_OK && read > 0) {
                bytes += std::fwrite(buffer, 1, read, file);
            }
        }
        writeHeader(file, bytes);
        std::fclose(file);
        ESP_LOGI(TAG, "Saved %s (%lu bytes)", path, static_cast<unsigned long>(bytes));
    }
    lv_async_call(complete, state);
    vTaskDelete(nullptr);
}

void toggle(lv_event_t *event)
{
    State *state = static_cast<State *>(lv_event_get_user_data(event));
    if (state == nullptr) return;
    if (state->recording) {
        state->stop = true;
        lv_label_set_text(state->status, "Saving recording...");
        return;
    }
    state->stop = false;
    state->recording = true;
    lv_label_set_text(state->status, "Recording...");
    lv_label_set_text(state->buttonLabel, LV_SYMBOL_STOP " Stop");
    if (xTaskCreate(recordTask, "secretary-rec", 4096, state, 3, &state->task) != pdPASS) {
        state->recording = false;
        lv_label_set_text(state->status, "Unable to start recording");
    }
}

void destroy(lv_event_t *event)
{
    State *state = static_cast<State *>(lv_event_get_user_data(event));
    if (state == nullptr) return;
    state->status = nullptr;
    state->buttonLabel = nullptr;
    state->files = nullptr;
    if (state->recording) state->stop = true;
    else delete state;
}

}

namespace SecretaryRecordingPage {

lv_obj_t *create(lv_obj_t *parent)
{
    State *state = new State{};
    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_set_size(container, lv_pct(100), lv_pct(100));
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(container, destroy, LV_EVENT_DELETE, state);

    state->status = lv_label_create(container);
    lv_label_set_text(state->status, "Ready");
    lv_obj_set_style_text_font(state->status, &lv_font_montserrat_24, 0);
    lv_obj_align(state->status, LV_ALIGN_TOP_MID, 0, 24);

    lv_obj_t *button = lv_btn_create(container);
    lv_obj_set_size(button, 220, 64);
    lv_obj_align(button, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_add_event_cb(button, toggle, LV_EVENT_CLICKED, state);
    state->buttonLabel = lv_label_create(button);
    lv_label_set_text(state->buttonLabel, LV_SYMBOL_AUDIO " Record");
    lv_obj_center(state->buttonLabel);

    state->files = lv_label_create(container);
    lv_obj_set_width(state->files, lv_pct(90));
    lv_label_set_long_mode(state->files, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(state->files, &lv_font_montserrat_18, 0);
    lv_obj_align(state->files, LV_ALIGN_TOP_LEFT, 20, 180);
    refreshFiles(state);
    return container;
}

}