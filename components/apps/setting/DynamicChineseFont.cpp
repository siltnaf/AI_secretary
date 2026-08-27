#include "DynamicChineseFont.hpp"

#include <cstdio>
#include <cstring>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace {

constexpr char FONT_PATH[] = "/sdcard/fonts/font_puhui_common_20_4.bin";
constexpr uint8_t MAX_CMAPS = 32;
constexpr uint8_t CACHE_SIZE = 32;
constexpr size_t MAX_BITMAP_BYTES = 256;
constexpr char TAG[] = "DynamicChineseFont";

struct Cmap {
    uint32_t rangeStart;
    uint16_t rangeLength;
    uint16_t glyphIdStart;
    uint32_t unicodeListOffset;
    uint32_t glyphOffsetListOffset;
    uint16_t listLength;
    uint8_t type;
};

struct Glyph {
    uint16_t advance;
    uint16_t width;
    uint16_t height;
    int16_t offsetX;
    int16_t offsetY;
    uint16_t bitmapBytes;
    uint8_t bitmap[MAX_BITMAP_BYTES];
};

struct CacheEntry {
    bool valid;
    uint32_t codepoint;
    Glyph glyph;
};

FILE *fontFile = nullptr;
SemaphoreHandle_t fileMutex = nullptr;
bool available = false;
uint32_t glyphBitmapBase = 0;
uint32_t glyphDescriptorBase = 0;
uint32_t cmapsBase = 0;
uint8_t bitsPerPixel = 0;
uint8_t cmapCount = 0;
Cmap cmaps[MAX_CMAPS] = {};
CacheEntry cache[CACHE_SIZE] = {};
uint8_t nextCacheSlot = 0;
lv_font_t dynamicFont = {};

bool readAt(uint32_t offset, void *destination, size_t length)
{
    if (fontFile == nullptr || destination == nullptr || fileMutex == nullptr ||
            xSemaphoreTake(fileMutex, pdMS_TO_TICKS(250)) != pdTRUE) return false;
    const bool ok = std::fseek(fontFile, static_cast<long>(offset), SEEK_SET) == 0 &&
                    std::fread(destination, 1, length, fontFile) == length;
    xSemaphoreGive(fileMutex);
    return ok;
}

bool readU8(uint32_t offset, uint8_t &value) { return readAt(offset, &value, 1); }

bool readU16(uint32_t offset, uint16_t &value)
{
    uint8_t bytes[2];
    if (!readAt(offset, bytes, sizeof(bytes))) return false;
    value = static_cast<uint16_t>(bytes[0]) | static_cast<uint16_t>(bytes[1]) << 8;
    return true;
}

bool readU32(uint32_t offset, uint32_t &value)
{
    uint8_t bytes[4];
    if (!readAt(offset, bytes, sizeof(bytes))) return false;
    value = static_cast<uint32_t>(bytes[0]) | static_cast<uint32_t>(bytes[1]) << 8 |
            static_cast<uint32_t>(bytes[2]) << 16 | static_cast<uint32_t>(bytes[3]) << 24;
    return true;
}

int32_t sparseIndex(const Cmap &cmap, uint16_t relative)
{
    int32_t low = 0;
    int32_t high = static_cast<int32_t>(cmap.listLength) - 1;
    while (low <= high) {
        const int32_t middle = low + (high - low) / 2;
        uint16_t value = 0;
        if (!readU16(cmapsBase + cmap.unicodeListOffset + middle * 2U, value)) return -1;
        if (value == relative) return middle;
        if (value < relative) low = middle + 1;
        else high = middle - 1;
    }
    return -1;
}

uint32_t glyphIdFor(uint32_t codepoint)
{
    for (uint8_t index = 0; index < cmapCount; ++index) {
        const Cmap &cmap = cmaps[index];
        if (codepoint < cmap.rangeStart || codepoint >= cmap.rangeStart + cmap.rangeLength) continue;
        const uint16_t relative = static_cast<uint16_t>(codepoint - cmap.rangeStart);
        if (cmap.type == 2) return cmap.glyphIdStart + relative;
        if (cmap.type == 0) {
            uint8_t offset = 0;
            return readU8(cmapsBase + cmap.glyphOffsetListOffset + relative, offset)
                ? cmap.glyphIdStart + offset : 0;
        }
        const int32_t sparse = sparseIndex(cmap, relative);
        if (sparse < 0) continue;
        if (cmap.type == 3) return cmap.glyphIdStart + sparse;
        if (cmap.type == 1) {
            uint16_t offset = 0;
            return readU16(cmapsBase + cmap.glyphOffsetListOffset + sparse * 2U, offset)
                ? cmap.glyphIdStart + offset : 0;
        }
    }
    return 0;
}

bool loadGlyph(uint32_t codepoint, Glyph &glyph)
{
    const uint32_t id = glyphIdFor(codepoint);
    if (id == 0) return false;
    const uint32_t descriptor = glyphDescriptorBase + id * 16U;
    uint32_t bitmapIndex = 0;
    uint32_t advance16 = 0;
    uint16_t offsetX = 0;
    uint16_t offsetY = 0;
    if (!readU32(descriptor, bitmapIndex) || !readU32(descriptor + 4, advance16) ||
        !readU16(descriptor + 8, glyph.width) || !readU16(descriptor + 10, glyph.height) ||
        !readU16(descriptor + 12, offsetX) || !readU16(descriptor + 14, offsetY)) return false;
    glyph.offsetX = static_cast<int16_t>(offsetX);
    glyph.offsetY = static_cast<int16_t>(offsetY);
    glyph.advance = static_cast<uint16_t>((advance16 + 8U) / 16U);
    glyph.bitmapBytes = static_cast<uint16_t>((glyph.width * glyph.height * bitsPerPixel + 7U) / 8U);
    return glyph.bitmapBytes <= MAX_BITMAP_BYTES &&
           (glyph.bitmapBytes == 0 || readAt(glyphBitmapBase + bitmapIndex, glyph.bitmap, glyph.bitmapBytes));
}

const Glyph *glyphFor(uint32_t codepoint)
{
    for (CacheEntry &entry : cache) {
        if (entry.valid && entry.codepoint == codepoint) return &entry.glyph;
    }
    CacheEntry &entry = cache[nextCacheSlot++ % CACHE_SIZE];
    entry.valid = loadGlyph(codepoint, entry.glyph);
    entry.codepoint = codepoint;
    return entry.valid ? &entry.glyph : nullptr;
}

bool getGlyphDescriptor(const lv_font_t *, lv_font_glyph_dsc_t *descriptor, uint32_t codepoint, uint32_t)
{
    const Glyph *glyph = glyphFor(codepoint);
    if (glyph == nullptr) return false;
    descriptor->adv_w = glyph->advance;
    descriptor->box_w = glyph->width;
    descriptor->box_h = glyph->height;
    descriptor->ofs_x = glyph->offsetX;
    descriptor->ofs_y = glyph->offsetY;
    descriptor->bpp = bitsPerPixel;
    descriptor->is_placeholder = 0;
    return true;
}

const uint8_t *getGlyphBitmap(const lv_font_t *, uint32_t codepoint)
{
    const Glyph *glyph = glyphFor(codepoint);
    return glyph == nullptr ? nullptr : glyph->bitmap;
}

bool openFont(const lv_font_t *fallback)
{
    fontFile = std::fopen(FONT_PATH, "rb");
    if (fontFile == nullptr) return false;
    uint32_t descriptorRelative = 0;
    if (!readU32(24, descriptorRelative)) return false;
    const uint32_t descriptorBase = descriptorRelative;
    uint32_t bitmapRelative = 0;
    uint32_t glyphRelative = 0;
    uint32_t cmapRelative = 0;
    uint16_t flags = 0;
    if (!readU32(descriptorBase, bitmapRelative) || !readU32(descriptorBase + 4, glyphRelative) ||
        !readU32(descriptorBase + 8, cmapRelative) || !readU16(descriptorBase + 18, flags)) return false;
    glyphBitmapBase = descriptorBase + bitmapRelative;
    glyphDescriptorBase = descriptorBase + glyphRelative;
    cmapsBase = descriptorBase + cmapRelative;
    cmapCount = static_cast<uint8_t>(flags & 0x01FFU);
    bitsPerPixel = static_cast<uint8_t>((flags >> 9) & 0x0FU);
    if (cmapCount == 0 || cmapCount > MAX_CMAPS || bitsPerPixel != 4) return false;
    for (uint8_t index = 0; index < cmapCount; ++index) {
        const uint32_t offset = cmapsBase + index * 20U;
        Cmap &cmap = cmaps[index];
        if (!readU32(offset, cmap.rangeStart) || !readU16(offset + 4, cmap.rangeLength) ||
            !readU16(offset + 6, cmap.glyphIdStart) || !readU32(offset + 8, cmap.unicodeListOffset) ||
            !readU32(offset + 12, cmap.glyphOffsetListOffset) || !readU16(offset + 16, cmap.listLength) ||
            !readU8(offset + 18, cmap.type)) return false;
    }
    dynamicFont.get_glyph_dsc = getGlyphDescriptor;
    dynamicFont.get_glyph_bitmap = getGlyphBitmap;
    dynamicFont.line_height = 24;
    dynamicFont.base_line = 4;
    dynamicFont.subpx = LV_FONT_SUBPX_NONE;
    dynamicFont.underline_position = -2;
    dynamicFont.underline_thickness = 1;
    dynamicFont.fallback = fallback;
    std::memset(cache, 0, sizeof(cache));
    available = true;
    ESP_LOGI(TAG, "Loaded full-range PuHui font from SD (%u cmaps)", cmapCount);
    return true;
}

}

namespace DynamicChineseFont {

bool begin(const lv_font_t *fallback)
{
    if (available) return true;
    if (fileMutex == nullptr) fileMutex = xSemaphoreCreateMutex();
    return fileMutex != nullptr && openFont(fallback);
}

bool isAvailable(void) { return available; }
const lv_font_t *get(void) { return available ? &dynamicFont : nullptr; }

}