#include "ChineseFont.hpp"
#include "DynamicChineseFont.hpp"

#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>

#include "bsp/esp32_p4_function_ev_board.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

constexpr char FONT_DIRECTORY[] = "/sdcard/fonts";
constexpr size_t FONT_COUNT = 5;

extern const uint8_t app_locale_16_start[] asm("_binary_app_locale_16_fnt_start");
extern const uint8_t app_locale_16_end[] asm("_binary_app_locale_16_fnt_end");
extern const uint8_t app_locale_20_start[] asm("_binary_app_locale_20_fnt_start");
extern const uint8_t app_locale_20_end[] asm("_binary_app_locale_20_fnt_end");
extern const uint8_t app_locale_24_start[] asm("_binary_app_locale_24_fnt_start");
extern const uint8_t app_locale_24_end[] asm("_binary_app_locale_24_fnt_end");
extern const uint8_t app_locale_30_start[] asm("_binary_app_locale_30_fnt_start");
extern const uint8_t app_locale_30_end[] asm("_binary_app_locale_30_fnt_end");
extern const uint8_t app_locale_38_start[] asm("_binary_app_locale_38_fnt_start");
extern const uint8_t app_locale_38_end[] asm("_binary_app_locale_38_fnt_end");
extern const uint8_t font_puhui_common_20_4_start[] asm("_binary_font_puhui_common_20_4_bin_start");
extern const uint8_t font_puhui_common_20_4_end[] asm("_binary_font_puhui_common_20_4_bin_end");

struct FontAsset {
    uint16_t size;
    const uint8_t *start;
    const uint8_t *end;
    lv_font_t *font;
};

FontAsset fonts[] = {
    {16, app_locale_16_start, app_locale_16_end, nullptr},
    {20, app_locale_20_start, app_locale_20_end, nullptr},
    {24, app_locale_24_start, app_locale_24_end, nullptr},
    {30, app_locale_30_start, app_locale_30_end, nullptr},
    {38, app_locale_38_start, app_locale_38_end, nullptr},
};

volatile bool workerStarted = false;
const char *TAG = "ChineseFont";

void makePaths(uint16_t size, char *systemPath, size_t systemSize, char *lvglPath, size_t lvglSize)
{
    std::snprintf(systemPath, systemSize, "%s/app_locale_v4_%u.fnt", FONT_DIRECTORY, size);
    std::snprintf(lvglPath, lvglSize, "F:fonts/app_locale_v4_%u.fnt", size);
}

bool fileMatches(const char *path, size_t expectedSize)
{
    struct stat info = {};
    return stat(path, &info) == 0 && static_cast<size_t>(info.st_size) == expectedSize;
}

bool ensureDirectory(void)
{
    return access(FONT_DIRECTORY, F_OK) == 0 || mkdir(FONT_DIRECTORY, 0775) == 0;
}

bool seedDynamicFont(void)
{
    constexpr char path[] = "/sdcard/fonts/font_puhui_common_20_4.bin";
    const size_t size = static_cast<size_t>(font_puhui_common_20_4_end - font_puhui_common_20_4_start);
    if (fileMatches(path, size)) return true;
    FILE *output = std::fopen(path, "wb");
    if (output == nullptr) return false;
    const bool ok = std::fwrite(font_puhui_common_20_4_start, 1, size, output) == size;
    std::fclose(output);
    return ok;
}

bool seedAsset(const FontAsset &asset)
{
    char path[96] = {};
    char lvglPath[64] = {};
    makePaths(asset.size, path, sizeof(path), lvglPath, sizeof(lvglPath));
    const size_t assetSize = static_cast<size_t>(asset.end - asset.start);
    if (fileMatches(path, assetSize)) return true;

    char temporaryPath[104] = {};
    std::snprintf(temporaryPath, sizeof(temporaryPath), "%s.part", path);
    remove(temporaryPath);
    FILE *output = std::fopen(temporaryPath, "wb");
    if (output == nullptr) return false;
    const bool written = std::fwrite(asset.start, 1, assetSize, output) == assetSize;
    std::fflush(output);
    std::fclose(output);
    if (!written) {
        remove(temporaryPath);
        return false;
    }
    remove(path);
    return rename(temporaryPath, path) == 0;
}

void loadFonts(void *)
{
    size_t loaded = 0;
    for (FontAsset &asset : fonts) {
        char path[96] = {};
        char lvglPath[64] = {};
        makePaths(asset.size, path, sizeof(path), lvglPath, sizeof(lvglPath));
        if (asset.font == nullptr) asset.font = lv_font_load(lvglPath);
        lv_font_glyph_dsc_t descriptor = {};
        const bool valid = asset.font != nullptr &&
                           lv_font_get_glyph_dsc(asset.font, &descriptor, 0x79D8, 0) &&
                           descriptor.box_w > 0 && descriptor.box_h > 0;
        if (valid) ++loaded;
        else {
            if (asset.font != nullptr) lv_font_free(asset.font);
            asset.font = nullptr;
            ESP_LOGE(TAG, "Font %u failed Chinese glyph validation", asset.size);
        }
    }
    if (!DynamicChineseFont::begin(fonts[1].font)) {
        ESP_LOGE(TAG, "Full-range Chinese font failed to load");
    }
    ESP_LOGI(TAG, "Loaded %u/%u project Chinese fonts from SD", static_cast<unsigned>(loaded),
             static_cast<unsigned>(FONT_COUNT));
}

void provisionTask(void *)
{
    bool ready = ensureDirectory();
    for (const FontAsset &asset : fonts) ready = ready && seedAsset(asset);
    ready = ready && seedDynamicFont();
    if (ready) lv_async_call(loadFonts, nullptr);
    else ESP_LOGE(TAG, "Could not seed project Chinese fonts to SD");
    workerStarted = false;
    vTaskDelete(nullptr);
}

}

namespace ChineseFont {

void begin(void)
{
    if (fonts[0].font != nullptr || workerStarted || bsp_sdcard == nullptr) return;
    workerStarted = true;
    if (xTaskCreate(provisionTask, "font-provision", 8192, nullptr, 1, nullptr) != pdPASS) {
        workerStarted = false;
        ESP_LOGE(TAG, "Could not start Chinese font provisioner");
    }
}

bool isAvailable(void) { return fonts[0].font != nullptr; }

const lv_font_t *get(uint16_t size)
{
    FontAsset *best = &fonts[0];
    uint16_t bestDistance = size > best->size ? size - best->size : best->size - size;
    for (FontAsset &asset : fonts) {
        const uint16_t distance = size > asset.size ? size - asset.size : asset.size - size;
        if (asset.font != nullptr && (best->font == nullptr || distance < bestDistance)) {
            best = &asset;
            bestDistance = distance;
        }
    }
    return best->font;
}

const lv_font_t *getDynamic(void)
{
    const lv_font_t *font = DynamicChineseFont::get();
    return font != nullptr ? font : get(20);
}

bool hasGlyph(uint32_t codepoint)
{
    const lv_font_t *font = get(20);
    if (font == nullptr) return false;
    lv_font_glyph_dsc_t descriptor = {};
    return lv_font_get_glyph_dsc(font, &descriptor, codepoint, 0);
}

}

extern "C" const lv_font_t *ChineseFont_Get(void) { return ChineseFont::get(20); }
extern "C" bool ChineseFont_IsAvailable(void) { return ChineseFont::isAvailable(); }