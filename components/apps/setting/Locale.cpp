#include "Locale.hpp"

#include <cstdio>
#include <cstring>

#include "ChineseFont.hpp"
#include "esp_log.h"
#include "nvs.h"

namespace {

constexpr char NVS_NAMESPACE[] = "storage";
constexpr char NVS_LANGUAGE_KEY[] = "language";
constexpr uint32_t REFRESH_PERIOD_MS = 400;

struct Translation {
    const char *english;
    const char *chinese;
};

const Translation TRANSLATIONS[] = {
    {"AI Secretary", "AI秘书"},
    {"Secretary", "秘书"},
    {"Personal workspace", "个人工作空间"},
    {"Reader", "阅读器"},
    {"Content Reader", "内容阅读器"},
    {"Library", "图书馆"},
    {"Clock", "时钟"},
    {"Calendar", "日历"},
    {"Calculator", "计算器"},
    {"Music Player", "音乐播放器"},
    {"Video Player", "视频播放器"},
    {"Image", "图片"},
    {"Squareline", "智能设备"},
    {"2048", "2048"},
    {"Music", "音乐"},
    {"Chat", "聊天"},
    {"Game", "游戏"},
    {"Voice", "语音"},
    {"Recording", "录音"},
    {"Poems", "诗词"},
    {"Words", "单词"},
    {"Cartoons", "动画"},
    {"Radio", "收音机"},
    {"Find Home", "找回家"},
    {"Settings", "设置"},
    {"Networking", "网络"},
    {"Bluetooth", "蓝牙"},
    {"Audio", "音频"},
    {"Volume", "音量"},
    {"Display", "显示"},
    {"Brightness", "亮度"},
    {"SD Card", "SD卡"},
    {"Playlist Cache", "播放列表缓存"},
    {"About Device", "关于设备"},
    {"Content URL", "内容网址"},
    {"Cellular / 4G", "蜂窝网络 / 4G"},
    {"TTS Voice", "语音合成声音"},
    {"Audio Test", "音频测试"},
    {"Language", "语言"},
    {"Chinese", "中文"},
    {"English", "英文"},
    {"Mounted", "已挂载"},
    {"Not inserted", "未插入"},
    {"Test", "测试"},
    {"Codec ready", "音频设备正常"},
    {"Codec error", "音频设备错误"},
    {"Device Name", "设备名称"},
    {"Manufacturer", "制造商"},
    {"UI Framework", "界面框架"},
    {"Software Version", "软件版本"},
    {"UI Framework Version", "界面框架版本"},
    {"Password...", "密码..."},
    {"SSID:", "网络名称："},
    {"Checking SD card...", "正在检查SD卡..."},
    {"SD card is empty", "SD卡为空"},
    {"Format SD Card", "格式化SD卡"},
    {"Format SD Card?", "格式化SD卡？"},
    {"All files on the SD card will be deleted.", "SD卡中的所有文件都将被删除。"},
    {"Formatting SD card...", "正在格式化SD卡..."},
    {"Format completed", "格式化完成"},
    {"The SD card was formatted successfully.", "SD卡已成功格式化。"},
    {"SD format failed", "SD卡格式化失败"},
    {"SD card formatted and mounted", "SD卡已格式化并挂载"},
    {"SD card not mounted", "SD卡未挂载"},
    {"Unable to open SD directory", "无法打开SD卡目录"},
    {"Insert and mount an SD card first", "请先插入并挂载SD卡"},
    {"Save", "保存"},
    {"Cancel", "取消"},
    {"Format", "格式化"},
    {"OK", "确定"},
    {"E-paper Content Services", "电子纸内容服务"},
    {"Used by Book, Voice Story, Music, Poem, Learn Word, Cartoon, Radio, Map, Chat, and Games.",
     "供图书、故事、音乐、诗词、单词、动画、收音机、地图、聊天和游戏使用。"},
    {"Invalid URL", "网址无效"},
    {"Use http:// or https://", "请使用 http:// 或 https://"},
    {"Use Wi-Fi now. 4G can be added as another transport later.", "当前使用Wi-Fi，稍后可添加4G网络。"},
    {"Save URL", "保存网址"},
    {"Fetch", "获取"},
    {"Fetching content...", "正在获取内容..."},
    {"Enter a content URL and fetch it over Wi-Fi.", "输入内容网址并通过Wi-Fi获取。"},
    {"No content loaded.", "尚未加载内容。"},
    {"Content URL saved.", "内容网址已保存。"},
    {"Unable to save the URL.", "无法保存网址。"},
    {"Unable to save content URL.", "无法保存内容网址。"},
    {"Fetching content over Wi-Fi...", "正在通过Wi-Fi获取内容..."},
    {"Use a URL beginning with http:// or https://", "网址必须以 http:// 或 https:// 开头"},
    {"Fetch failed. Connect Wi-Fi in Settings and verify the URL.", "获取失败，请在设置中连接Wi-Fi并检查网址。"},
    {"Saved URL is shared by Reader, Library, Chat, Music, Game, and Find Home.",
     "保存的网址由阅读器、图书馆、聊天、音乐、游戏和找回家功能共用。"},
    {"Load Service", "加载服务"},
    {"Open Reader", "打开阅读器"},
    {"Prev", "上一页"},
    {"Next", "下一页"},
    {"Camera", "相机"},
    {"Photo", "照片"},
    {"SCORE", "分数"},
    {"BEST", "最高分"},
    {"New Game", "新游戏"},
    {"Welcome to play!", "欢迎游戏！"},
    {"Welcome to paly!", "欢迎游戏！"},
    {"Sorry, game over!", "抱歉，游戏结束！"},
    {"Demo", "演示"},
    {"Smart Gadget", "智能设备"},
    {"Incoming Call", "来电"},
    {"Yesterday 07:25 PM", "昨天 晚上07:25"},
    {"Let's get some dinner, how about pizza?", "一起吃晚饭吧，披萨怎么样？"},
    {"Sounds good! What about James?", "好啊！James呢？"},
    {"Delivered", "已送达"},
    {"He likes it too! Where do we meet?", "他也喜欢！我们在哪里见？"},
    {"Partly cloudy", "局部多云"},
    {"Clear", "晴"},
    {"Cloudy", "多云"},
    {"Rain", "雨"},
    {"New York", "纽约"},
    {"Location unavailable", "位置不可用"},
    {"Weather unavailable", "天气不可用"},
    {"Humidity", "湿度"},
    {"Wind", "风速"},
    {"Set alarm", "设置闹钟"},
    {"Breakfast", "早餐"},
    {"Yoga", "瑜伽"},
    {"Sleep", "睡觉"},
};

bool chineseEnabled = false;
lv_timer_t *refreshTimer = nullptr;
constexpr uint16_t CHINESE_FONT_SIZES[] = {16, 20, 24, 30, 38};
constexpr size_t CHINESE_FONT_COUNT = sizeof(CHINESE_FONT_SIZES) / sizeof(CHINESE_FONT_SIZES[0]);
lv_style_t chineseStyles[CHINESE_FONT_COUNT];
bool styleInitialized = false;
const char *TAG = "AppLocale";

struct FontOverride {
    lv_obj_t *object;
    const lv_font_t *original;
};

constexpr size_t MAX_FONT_OVERRIDES = 256;
FontOverride fontOverrides[MAX_FONT_OVERRIDES] = {};

void forgetFontOverride(lv_event_t *event)
{
    lv_obj_t *object = lv_event_get_target(event);
    for (FontOverride &entry : fontOverrides) {
        if (entry.object == object) entry = {};
    }
}

FontOverride *findFontOverride(lv_obj_t *object, bool create)
{
    FontOverride *empty = nullptr;
    for (FontOverride &entry : fontOverrides) {
        if (entry.object == object) return &entry;
        if (entry.object == nullptr && empty == nullptr) empty = &entry;
    }
    if (!create || empty == nullptr) return nullptr;
    empty->object = object;
    empty->original = lv_obj_get_style_text_font(object, LV_PART_MAIN);
    lv_obj_add_event_cb(object, forgetFontOverride, LV_EVENT_DELETE, nullptr);
    return empty;
}

const char *lookup(const char *text, bool chinese)
{
    if (text == nullptr) return nullptr;
    for (const Translation &entry : TRANSLATIONS) {
        const char *source = chinese ? entry.english : entry.chinese;
        const char *target = chinese ? entry.chinese : entry.english;
        if (std::strcmp(text, source) == 0 || std::strcmp(text, target) == 0) return target;
    }
    return nullptr;
}

const char *translateDate(const char *text, bool chinese, char *buffer, size_t size)
{
    static const char *ENGLISH_DAYS[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char *CHINESE_DAYS[] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};
    static const char *ENGLISH_MONTHS[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    if (chinese) {
        char day[4] = {};
        char month[4] = {};
        int date = 0;
        if (std::sscanf(text, "%3s %d %3s", day, &date, month) != 3) return nullptr;
        int dayIndex = -1;
        int monthIndex = -1;
        for (int index = 0; index < 7; ++index) if (std::strcmp(day, ENGLISH_DAYS[index]) == 0) dayIndex = index;
        for (int index = 0; index < 12; ++index) if (std::strcmp(month, ENGLISH_MONTHS[index]) == 0) monthIndex = index;
        if (dayIndex < 0 || monthIndex < 0) return nullptr;
        std::snprintf(buffer, size, "%s %d月%d日", CHINESE_DAYS[dayIndex], monthIndex + 1, date);
        return buffer;
    }

    char chineseDay[8] = {};
    int month = 0;
    int date = 0;
    if (std::sscanf(text, "%7s %d月%d日", chineseDay, &month, &date) != 3 || month < 1 || month > 12) return nullptr;
    int dayIndex = -1;
    for (int index = 0; index < 7; ++index) if (std::strcmp(chineseDay, CHINESE_DAYS[index]) == 0) dayIndex = index;
    if (dayIndex < 0) return nullptr;
    std::snprintf(buffer, size, "%s %d %s", ENGLISH_DAYS[dayIndex], date, ENGLISH_MONTHS[month - 1]);
    return buffer;
}

const char *translateDynamic(const char *text, bool chinese, char *buffer, size_t size)
{
    int value = 0;
    if (chinese) {
        if (std::sscanf(text, "Content page %d", &value) == 1) {
            std::snprintf(buffer, size, "内容第%d页", value);
            return buffer;
        }
        if (std::sscanf(text, "Score[%d]: Weak...", &value) == 1) {
            std::snprintf(buffer, size, "分数[%d]：较弱...", value);
            return buffer;
        }
        if (std::sscanf(text, "Score[%d]: Normal.", &value) == 1) {
            std::snprintf(buffer, size, "分数[%d]：普通。", value);
            return buffer;
        }
        if (std::sscanf(text, "Score[%d]: Good!", &value) == 1) {
            std::snprintf(buffer, size, "分数[%d]：很好！", value);
            return buffer;
        }
        if (std::sscanf(text, "Score[%d]: Excellent!", &value) == 1) {
            std::snprintf(buffer, size, "分数[%d]：优秀！", value);
            return buffer;
        }
    } else {
        if (std::sscanf(text, "内容第%d页", &value) == 1) {
            std::snprintf(buffer, size, "Content page %d", value);
            return buffer;
        }
        const char *suffix = nullptr;
        if (std::sscanf(text, "分数[%d]", &value) == 1) {
            suffix = std::strstr(text, "较弱") ? "Weak..." :
                     std::strstr(text, "普通") ? "Normal." :
                     std::strstr(text, "优秀") ? "Excellent!" : "Good!";
            std::snprintf(buffer, size, "Score[%d]: %s", value, suffix);
            return buffer;
        }
    }
    return nullptr;
}

void applyFont(lv_obj_t *object, bool translated)
{
    if (!styleInitialized) return;
    if (chineseEnabled && translated && ChineseFont::isAvailable()) {
        FontOverride *override = findFontOverride(object, true);
        if (override == nullptr) {
            ESP_LOGE(TAG, "Font override table is full");
            return;
        }
        const lv_font_t *originalFont = override->original;
        const uint16_t originalSize = originalFont == nullptr ? 20 : originalFont->line_height;
        size_t bestIndex = 0;
        uint16_t bestDistance = originalSize > CHINESE_FONT_SIZES[0]
                                ? originalSize - CHINESE_FONT_SIZES[0]
                                : CHINESE_FONT_SIZES[0] - originalSize;
        for (size_t index = 1; index < CHINESE_FONT_COUNT; ++index) {
            const uint16_t distance = originalSize > CHINESE_FONT_SIZES[index]
                                      ? originalSize - CHINESE_FONT_SIZES[index]
                                      : CHINESE_FONT_SIZES[index] - originalSize;
            if (distance < bestDistance) {
                bestIndex = index;
                bestDistance = distance;
            }
        }
        lv_obj_set_style_text_font(object, ChineseFont::get(CHINESE_FONT_SIZES[bestIndex]),
                                   LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        FontOverride *override = findFontOverride(object, false);
        if (override != nullptr) {
            lv_obj_set_style_text_font(object, override->original, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}

void translateTextObject(lv_obj_t *object)
{
    const bool isLabel = lv_obj_check_type(object, &lv_label_class);
    const bool isTextarea = lv_obj_check_type(object, &lv_textarea_class);
    if (!isLabel && !isTextarea) return;

    const char *text = isLabel ? lv_label_get_text(object) : lv_textarea_get_text(object);
    char dynamicText[160] = {};
    const char *translated = lookup(text, chineseEnabled);
    if (translated == nullptr) translated = translateDynamic(text, chineseEnabled, dynamicText, sizeof(dynamicText));
    if (translated == nullptr) translated = translateDate(text, chineseEnabled, dynamicText, sizeof(dynamicText));
    if (translated != nullptr && std::strcmp(text, translated) != 0) {
        if (isLabel) lv_label_set_text(object, translated);
        else lv_textarea_set_text(object, translated);
    }

    bool usesChinese = translated != nullptr;
    if (isTextarea) {
        const char *placeholder = lv_textarea_get_placeholder_text(object);
        const char *translatedPlaceholder = lookup(placeholder, chineseEnabled);
        if (translatedPlaceholder != nullptr && std::strcmp(placeholder, translatedPlaceholder) != 0) {
            lv_textarea_set_placeholder_text(object, translatedPlaceholder);
            usesChinese = true;
        }
    }
    applyFont(object, usesChinese);
}

void walk(lv_obj_t *object)
{
    if (object == nullptr) return;
    translateTextObject(object);
    const uint32_t count = lv_obj_get_child_cnt(object);
    for (uint32_t index = 0; index < count; ++index) {
        walk(lv_obj_get_child(object, index));
    }
}

bool loadLanguage(void)
{
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) return false;
    int32_t language = 0;
    const esp_err_t result = nvs_get_i32(handle, NVS_LANGUAGE_KEY, &language);
    nvs_close(handle);
    if (result == ESP_OK) chineseEnabled = language != 0;
    return result == ESP_OK;
}

void refresh(lv_timer_t *)
{
    const bool previous = chineseEnabled;
    loadLanguage();
    if (previous != chineseEnabled) {
        ESP_LOGI(TAG, "Language changed to %s", chineseEnabled ? "Chinese" : "English");
    }
    AppLocale::applyNow();
}

}

namespace AppLocale {

void begin(void)
{
    if (refreshTimer != nullptr) return;
    loadLanguage();
    for (lv_style_t &style : chineseStyles) lv_style_init(&style);
    styleInitialized = true;
    refreshTimer = lv_timer_create(refresh, REFRESH_PERIOD_MS, nullptr);
    applyNow();
}

void setChinese(bool enabled)
{
    chineseEnabled = enabled;
    applyNow();
}

bool isChinese(void) { return chineseEnabled; }

void applyNow(void)
{
    if (!styleInitialized) return;
    if (ChineseFont::isAvailable()) {
        for (size_t index = 0; index < CHINESE_FONT_COUNT; ++index) {
            lv_style_set_text_font(&chineseStyles[index], ChineseFont::get(CHINESE_FONT_SIZES[index]));
        }
    }
    walk(lv_scr_act());
    walk(lv_layer_top());
    walk(lv_layer_sys());
}

}

extern "C" bool AppLocale_IsChinese(void) { return AppLocale::isChinese(); }
extern "C" void AppLocale_Apply(lv_obj_t *root) { walk(root); }