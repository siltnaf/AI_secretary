#include <ctime>
#include <cstring>
#include <algorithm>
#include <cstdlib>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "drivers/content_store/ContentStore.hpp"
#include "drivers/content_transport/ContentTransport.hpp"
#include "drivers/content_transport/ServiceRoutes.hpp"
#include "pages/calendar/CalendarPage.hpp"
#include "pages/clock/ClockPage.hpp"
#include "pages/content/ContentPage.hpp"
#include "pages/library/LibraryPage.hpp"
#include "SecretaryApp.hpp"

namespace {

constexpr char TAG[] = "Secretary";
constexpr size_t READER_PAGE_BYTES = 3400;

struct FetchCompletion {
    SecretaryApp *app;
    SecretaryContentTransport::Result result;
};

struct NavigationRequest {
    SecretaryApp *app;
    SecretaryPage page;
    bool back;
};

struct HomeItem {
    SecretaryPage page;
    const char *symbol;
    const char *label;
};

const HomeItem HOME_ITEMS[] = {
    {SecretaryPage::Reader, LV_SYMBOL_LIST, "Reader"},
    {SecretaryPage::Library, LV_SYMBOL_FILE, "Library"},
    {SecretaryPage::Clock, LV_SYMBOL_REFRESH, "Clock"},
    {SecretaryPage::Calendar, LV_SYMBOL_EDIT, "Calendar"},
    {SecretaryPage::Calculator, "+", "Calculator"},
    {SecretaryPage::Music, LV_SYMBOL_AUDIO, "Music"},
    {SecretaryPage::Chat, LV_SYMBOL_BELL, "Chat"},
    {SecretaryPage::Game, LV_SYMBOL_PLAY, "Game"},
    {SecretaryPage::Voice, LV_SYMBOL_AUDIO, "Voice"},
    {SecretaryPage::Recording, LV_SYMBOL_SAVE, "Recording"},
    {SecretaryPage::Poem, LV_SYMBOL_FILE, "Poems"},
    {SecretaryPage::Word, LV_SYMBOL_EDIT, "Words"},
    {SecretaryPage::Cartoon, LV_SYMBOL_IMAGE, "Cartoons"},
    {SecretaryPage::Radio, LV_SYMBOL_WIFI, "Radio"},
    {SecretaryPage::FindHome, LV_SYMBOL_GPS, "Find Home"},
    {SecretaryPage::Settings, LV_SYMBOL_SETTINGS, "Settings"},
};

void styleSurface(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xF3F4F0), 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
}

}

SecretaryApp::SecretaryApp():
    ESP_Brookesia_PhoneApp("Secretary", nullptr, true),
    _page(SecretaryPage::Home),
    _body(nullptr),
    _url_input(nullptr),
    _reader(nullptr),
    _status(nullptr),
    _clock(nullptr),
    _url{},
    _request_url{},
    _content(nullptr),
    _content_size(0),
    _reader_page(0),
    _fetching(false),
    _fetch_page(SecretaryPage::Reader)
{
}

SecretaryApp::~SecretaryApp()
{
    free(_content);
}

bool SecretaryApp::init(void)
{
    return SecretaryContentStore::loadUrl(_url, sizeof(_url));
}

bool SecretaryApp::run(void)
{
    showHome();
    return true;
}

bool SecretaryApp::back(void)
{
    if (_page == SecretaryPage::Home) {
        return notifyCoreClosed();
    }
    showHome();
    return true;
}

bool SecretaryApp::close(void)
{
    _fetching = false;
    return true;
}

void SecretaryApp::createHeader(const char *title)
{
    lv_obj_t *screen = lv_scr_act();
    styleSurface(screen);
    lv_obj_update_layout(screen);
    const lv_coord_t screenHeight = lv_obj_get_height(screen);
    lv_obj_t *header = lv_obj_create(screen);
    lv_obj_set_size(header, lv_pct(100), 74);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x202A33), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_pad_hor(header, 18, 0);

    lv_obj_t *back = lv_btn_create(header);
    lv_obj_set_size(back, 46, 46);
    lv_obj_align(back, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(back, lv_color_hex(0x344552), 0);
    lv_obj_set_style_radius(back, 4, 0);
    lv_obj_add_event_cb(back, pageButtonEvent, LV_EVENT_CLICKED, this);
    lv_obj_t *back_label = lv_label_create(back);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);

    lv_obj_t *label = lv_label_create(header);
    lv_label_set_text(label, title);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 62, 0);

    _body = lv_obj_create(screen);
    lv_obj_set_size(_body, lv_pct(100), std::max<lv_coord_t>(0, screenHeight - 74));
    lv_obj_align(_body, LV_ALIGN_BOTTOM_MID, 0, 0);
    styleSurface(_body);
    lv_obj_set_style_pad_all(_body, 16, 0);
}

void SecretaryApp::showHome(void)
{
    _page = SecretaryPage::Home;
    _reader = nullptr;
    _url_input = nullptr;
    lv_obj_clean(lv_scr_act());
    createHeader("AI Secretary");

    lv_obj_t *grid = lv_obj_create(_body);
    lv_obj_set_size(grid, lv_pct(100), lv_pct(100));
    lv_obj_center(grid);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 4, 0);
    lv_obj_set_style_pad_row(grid, 6, 0);
    lv_obj_set_style_pad_column(grid, 6, 0);
    static lv_coord_t columns[] = {
        LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST
    };
    static lv_coord_t rows[] = {
        LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST
    };
    lv_obj_set_grid_dsc_array(grid, columns, rows);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);

    for (size_t index = 0; index < sizeof(HOME_ITEMS) / sizeof(HOME_ITEMS[0]); ++index) {
        const HomeItem &item = HOME_ITEMS[index];
        lv_obj_t *button = lv_btn_create(grid);
        lv_obj_set_grid_cell(button, LV_GRID_ALIGN_STRETCH, index % 4, 1,
                             LV_GRID_ALIGN_STRETCH, index / 4, 1);
        lv_obj_set_style_bg_color(button, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_border_color(button, lv_color_hex(0xCCD4D7), 0);
        lv_obj_set_style_border_width(button, 1, 0);
        lv_obj_set_style_radius(button, 4, 0);
        lv_obj_set_style_pad_all(button, 4, 0);
        lv_obj_add_event_cb(button, pageButtonEvent, LV_EVENT_CLICKED, this);
        lv_obj_set_user_data(button, reinterpret_cast<void *>(static_cast<uintptr_t>(item.page)));

        lv_obj_t *icon = lv_label_create(button);
        lv_label_set_text(icon, item.symbol);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_28, 0);
        lv_obj_set_style_text_color(icon, lv_color_hex(0x16697A), 0);
        lv_obj_align(icon, LV_ALIGN_CENTER, 0, -22);
        lv_obj_t *name = lv_label_create(button);
        lv_label_set_text(name, item.label);
        lv_label_set_long_mode(name, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(name, lv_pct(100));
        lv_obj_set_style_text_font(name, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(name, lv_color_hex(0x263238), 0);
        lv_obj_set_style_text_align(name, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(name, LV_ALIGN_CENTER, 0, 24);
    }
}

void SecretaryApp::showSettings(void)
{
    _page = SecretaryPage::Settings;
    lv_obj_clean(lv_scr_act());
    createHeader("Content URL");

    lv_obj_t *hint = lv_label_create(_body);
    lv_label_set_text(hint, "Use Wi-Fi now. 4G can be added as another transport later.");
    lv_label_set_long_mode(hint, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(hint, lv_pct(100));
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x52606D), 0);
    lv_obj_align(hint, LV_ALIGN_TOP_LEFT, 0, 0);

    _url_input = lv_textarea_create(_body);
    lv_obj_set_size(_url_input, lv_pct(100), 100);
    lv_obj_align(_url_input, LV_ALIGN_TOP_LEFT, 0, 70);
    lv_textarea_set_text(_url_input, _url);
    lv_textarea_set_one_line(_url_input, false);
    lv_textarea_set_placeholder_text(_url_input, "https://content.example");
    lv_obj_set_style_text_font(_url_input, &lv_font_montserrat_16, 0);

    lv_obj_t *save = lv_btn_create(_body);
    lv_obj_set_size(save, 150, 54);
    lv_obj_align(save, LV_ALIGN_TOP_LEFT, 0, 190);
    lv_obj_set_style_bg_color(save, lv_color_hex(0x16697A), 0);
    lv_obj_set_style_radius(save, 4, 0);
    lv_obj_add_event_cb(save, saveUrlEvent, LV_EVENT_CLICKED, this);
    lv_obj_t *save_label = lv_label_create(save);
    lv_label_set_text(save_label, "Save URL");
    lv_obj_center(save_label);

    _status = lv_label_create(_body);
    lv_label_set_text(_status, "Saved URL is shared by Reader, Library, Chat, Music, Game, and Find Home.");
    lv_label_set_long_mode(_status, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(_status, lv_pct(100));
    lv_obj_set_style_text_font(_status, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(_status, lv_color_hex(0x52606D), 0);
    lv_obj_align(_status, LV_ALIGN_TOP_LEFT, 0, 270);
}

void SecretaryApp::showReader(void)
{
    _page = SecretaryPage::Reader;
    lv_obj_clean(lv_scr_act());
    createHeader("Content Reader");

    _url_input = lv_textarea_create(_body);
    lv_obj_set_size(_url_input, lv_pct(100) - 116, 46);
    lv_obj_align(_url_input, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_textarea_set_one_line(_url_input, true);
    lv_textarea_set_text(_url_input, _url);
    lv_obj_set_style_text_font(_url_input, &lv_font_montserrat_16, 0);

    lv_obj_t *fetch = lv_btn_create(_body);
    lv_obj_set_size(fetch, 104, 46);
    lv_obj_align(fetch, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(fetch, lv_color_hex(0x16697A), 0);
    lv_obj_set_style_radius(fetch, 4, 0);
    lv_obj_add_event_cb(fetch, fetchButtonEvent, LV_EVENT_CLICKED, this);
    lv_obj_t *fetch_label = lv_label_create(fetch);
    lv_label_set_text(fetch_label, "Fetch");
    lv_obj_center(fetch_label);

    _status = lv_label_create(_body);
    lv_label_set_text(_status, _fetching ? "Fetching content..." : "Enter a content URL and fetch it over Wi-Fi.");
    lv_obj_set_style_text_font(_status, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(_status, lv_color_hex(0x52606D), 0);
    lv_obj_align(_status, LV_ALIGN_TOP_LEFT, 0, 58);

    _reader = lv_textarea_create(_body);
    lv_obj_set_size(_reader, lv_pct(100), lv_pct(100) - 185);
    lv_obj_align(_reader, LV_ALIGN_BOTTOM_MID, 0, -54);
    lv_textarea_set_text(_reader, "No content loaded.");
    lv_textarea_set_cursor_click_pos(_reader, false);
    lv_obj_set_style_text_font(_reader, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(_reader, lv_color_hex(0x1F2933), 0);

    lv_obj_t *previous = lv_btn_create(_body);
    lv_obj_set_size(previous, 110, 42);
    lv_obj_align(previous, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_add_event_cb(previous, readerNavEvent, LV_EVENT_CLICKED, this);
    lv_obj_set_user_data(previous, reinterpret_cast<void *>(-1));
    lv_obj_t *previous_label = lv_label_create(previous);
    lv_label_set_text(previous_label, LV_SYMBOL_LEFT " Prev");
    lv_obj_center(previous_label);

    lv_obj_t *next = lv_btn_create(_body);
    lv_obj_set_size(next, 110, 42);
    lv_obj_align(next, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_add_event_cb(next, readerNavEvent, LV_EVENT_CLICKED, this);
    lv_obj_set_user_data(next, reinterpret_cast<void *>(1));
    lv_obj_t *next_label = lv_label_create(next);
    lv_label_set_text(next_label, "Next " LV_SYMBOL_RIGHT);
    lv_obj_center(next_label);

    appendReaderText(_content == nullptr ? nullptr : _content);
}

void SecretaryApp::showInfoPage(SecretaryPage page)
{
    _page = page;
    lv_obj_clean(lv_scr_act());
    createHeader(secretaryPageTitle(page));

    if (page == SecretaryPage::Clock) {
        _clock = SecretaryClockPage::create(_body);
        lv_timer_create(tickClock, 1000, this);
        return;
    }

    if (page == SecretaryPage::Calendar) {
        SecretaryCalendarPage::create(_body);
        return;
    }

    if (page == SecretaryPage::Library) {
        ESP_LOGI(TAG, "Library opened url=%s", _url);
        SecretaryLibraryPage::create(_body, _url);
        char endpoint[256] = {};
        if (!SecretaryServiceRoutes::build(page, _url, endpoint, sizeof(endpoint))) {
            ESP_LOGE(TAG, "Library endpoint build failed url=%s", _url);
            _status = lv_label_create(_body);
            lv_label_set_text(_status, "Configure the content URL in Settings first.");
            return;
        }
        std::strncpy(_request_url, endpoint, sizeof(_request_url) - 1);
        _request_url[sizeof(_request_url) - 1] = '\0';
        _fetch_page = page;
        _fetching = true;
        ESP_LOGI(TAG, "Library request endpoint=%s", _request_url);
        xTaskCreate(serviceFetchTask, "Booklist fetch", 8192, this, 3, nullptr);
        return;
    }

    const char *description = "This page is part of the e-paper workspace port. Its data source uses the shared content URL and Wi-Fi transport.";
    if (page == SecretaryPage::Library || page == SecretaryPage::Chat || page == SecretaryPage::Music ||
            page == SecretaryPage::Game || page == SecretaryPage::FindHome) {
        description = "Open Content Reader to configure or fetch from the shared content URL. This page is ready for its service-specific endpoint when that backend is available.";
    } else if (page == SecretaryPage::Calculator) {
        description = "The existing Calculator app remains available from the phone launcher. This workspace page keeps the e-paper layout hierarchy intact.";
    } else if (page == SecretaryPage::Calendar) {
        description = "Calendar layout and date navigation are reserved here. Network calendar sources will use the shared transport.";
    }

    const bool remote_service = SecretaryServiceRoutes::isRemoteService(page);
    lv_obj_t *open_reader = SecretaryContentPage::create(
        _body, secretaryPageTitle(page), description, remote_service ? "Load Service" : "Open Reader");
    lv_obj_add_event_cb(open_reader, remote_service ? serviceFetchEvent : pageButtonEvent, LV_EVENT_CLICKED, this);
    lv_obj_set_user_data(open_reader, reinterpret_cast<void *>(static_cast<uintptr_t>(remote_service ? page : SecretaryPage::Reader)));
}

void SecretaryApp::showPage(SecretaryPage page)
{
    if (page == SecretaryPage::Home) {
        showHome();
    } else if (page == SecretaryPage::Reader) {
        showReader();
    } else if (page == SecretaryPage::Settings) {
        showSettings();
    } else {
        showInfoPage(page);
    }
}

void SecretaryApp::appendReaderText(const char *text)
{
    if (_reader == nullptr) {
        return;
    }
    if (text == nullptr || _content_size == 0) {
        lv_textarea_set_text(_reader, "No content loaded.\n\nSet a URL in Settings, connect Wi-Fi with the existing Settings app, then use Fetch.");
        return;
    }

    const size_t offset = static_cast<size_t>(_reader_page) * READER_PAGE_BYTES;
    if (offset >= _content_size) {
        _reader_page = 0;
    }
    const size_t start = static_cast<size_t>(_reader_page) * READER_PAGE_BYTES;
    const size_t length = std::min(READER_PAGE_BYTES, _content_size - start);
    char *page = static_cast<char *>(malloc(length + 1));
    if (page == nullptr) {
        return;
    }
    std::memcpy(page, text + start, length);
    page[length] = '\0';
    lv_textarea_set_text(_reader, page);
    free(page);
    if (_status != nullptr) {
        char status[64];
        std::snprintf(status, sizeof(status), "Content page %u", static_cast<unsigned>(_reader_page + 1));
        lv_label_set_text(_status, status);
    }
}

void SecretaryApp::pageButtonEvent(lv_event_t *event)
{
    SecretaryApp *app = static_cast<SecretaryApp *>(lv_event_get_user_data(event));
    lv_obj_t *target = lv_event_get_target(event);
    if (app == nullptr) {
        return;
    }
    if (target == nullptr || lv_obj_get_parent(target) == nullptr) {
        lv_async_call(navigateAsync, new NavigationRequest{app, SecretaryPage::Home, true});
        return;
    }
    void *data = lv_obj_get_user_data(target);
    if (data == nullptr) {
        lv_async_call(navigateAsync, new NavigationRequest{app, SecretaryPage::Home, true});
        return;
    }
    lv_async_call(
        navigateAsync,
        new NavigationRequest{app, static_cast<SecretaryPage>(reinterpret_cast<uintptr_t>(data)), false}
    );
}

void SecretaryApp::navigateAsync(void *arg)
{
    NavigationRequest *request = static_cast<NavigationRequest *>(arg);
    if (request == nullptr || request->app == nullptr) {
        delete request;
        return;
    }
    if (request->back) {
        request->app->back();
    } else {
        request->app->showPage(request->page);
    }
    delete request;
}

void SecretaryApp::saveUrlEvent(lv_event_t *event)
{
    SecretaryApp *app = static_cast<SecretaryApp *>(lv_event_get_user_data(event));
    if (app == nullptr || app->_url_input == nullptr) {
        return;
    }
    const char *url = lv_textarea_get_text(app->_url_input);
    if (std::strncmp(url, "http://", 7) != 0 && std::strncmp(url, "https://", 8) != 0) {
        lv_label_set_text(app->_status, "Use a URL beginning with http:// or https://");
        return;
    }
    std::strncpy(app->_url, url, sizeof(app->_url) - 1);
    app->_url[sizeof(app->_url) - 1] = '\0';
    lv_label_set_text(app->_status, SecretaryContentStore::saveUrl(app->_url) ? "Content URL saved." : "Unable to save the URL.");
}

void SecretaryApp::fetchButtonEvent(lv_event_t *event)
{
    SecretaryApp *app = static_cast<SecretaryApp *>(lv_event_get_user_data(event));
    if (app == nullptr || app->_fetching || app->_url_input == nullptr) {
        return;
    }
    std::strncpy(app->_url, lv_textarea_get_text(app->_url_input), sizeof(app->_url) - 1);
    app->_url[sizeof(app->_url) - 1] = '\0';
    std::strncpy(app->_request_url, app->_url, sizeof(app->_request_url) - 1);
    app->_request_url[sizeof(app->_request_url) - 1] = '\0';
    if (!SecretaryContentStore::saveUrl(app->_url)) {
        lv_label_set_text(app->_status, "Unable to save content URL.");
        return;
    }
    app->_fetching = true;
    lv_label_set_text(app->_status, "Fetching content over Wi-Fi...");
    xTaskCreate(fetchTask, "Content fetch", 8192, app, 3, nullptr);
}

void SecretaryApp::serviceFetchEvent(lv_event_t *event)
{
    SecretaryApp *app = static_cast<SecretaryApp *>(lv_event_get_user_data(event));
    if (app == nullptr || app->_fetching) {
        return;
    }
    SecretaryPage page = static_cast<SecretaryPage>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(lv_event_get_target(event))));
    char endpoint[256] = {};
    if (!SecretaryServiceRoutes::build(page, app->_url, endpoint, sizeof(endpoint))) {
        ESP_LOGW(TAG, "Configure a valid content URL before loading %s", secretaryPageTitle(page));
        return;
    }
    std::strncpy(app->_request_url, endpoint, sizeof(app->_request_url) - 1);
    app->_request_url[sizeof(app->_request_url) - 1] = '\0';
    app->_fetch_page = page;
    app->_fetching = true;
    xTaskCreate(serviceFetchTask, "Service fetch", 8192, app, 3, nullptr);
}

void SecretaryApp::fetchTask(void *arg)
{
    SecretaryApp *app = static_cast<SecretaryApp *>(arg);
    FetchCompletion *completion = new FetchCompletion{app, SecretaryContentTransport::get(app->_request_url)};
    lv_async_call(completeFetch, completion);
    vTaskDelete(nullptr);
}

void SecretaryApp::serviceFetchTask(void *arg)
{
    SecretaryApp *app = static_cast<SecretaryApp *>(arg);
    ESP_LOGI(TAG, "service fetch start page=%s url=%s", secretaryPageTitle(app->_fetch_page), app->_request_url);
    FetchCompletion *completion = new FetchCompletion{app, SecretaryContentTransport::get(app->_request_url)};
    ESP_LOGI(TAG, "service fetch complete page=%s ok=%d status=%d bytes=%u", secretaryPageTitle(app->_fetch_page),
             completion->result.ok, completion->result.status_code,
             static_cast<unsigned>(completion->result.size));
    lv_async_call(completeFetch, completion);
    vTaskDelete(nullptr);
}

void SecretaryApp::completeFetch(void *arg)
{
    FetchCompletion *completion = static_cast<FetchCompletion *>(arg);
    SecretaryApp *app = completion->app;
    app->_fetching = false;
    ESP_LOGI(TAG, "complete fetch page=%s ok=%d status=%d bytes=%u", secretaryPageTitle(app->_fetch_page),
             completion->result.ok, completion->result.status_code,
             static_cast<unsigned>(completion->result.size));
    if (!completion->result.ok) {
        if (app->_fetch_page == SecretaryPage::Library && app->_body != nullptr) {
            SecretaryLibraryPage::updateStatus(app->_body, "Booklist fetch failed. Check Wi-Fi and the content URL.");
        } else if (app->_status != nullptr) {
            lv_label_set_text(app->_status, "Fetch failed. Connect Wi-Fi in Settings and verify the URL.");
        }
        delete completion;
        return;
    }
    free(app->_content);
    app->_content = completion->result.data;
    app->_content_size = completion->result.size;
    completion->result.data = nullptr;
    app->_reader_page = 0;
    if (app->_fetch_page == SecretaryPage::Library) {
        app->_page = SecretaryPage::Library;
        SecretaryLibraryPage::update(app->_body, app->_url, app->_content, app->_content_size);
        SecretaryContentTransport::release(&completion->result);
        delete completion;
        return;
    }
    app->_page = SecretaryPage::Reader;
    app->showReader();
    app->appendReaderText(app->_content);
    SecretaryContentTransport::release(&completion->result);
    delete completion;
}

void SecretaryApp::readerNavEvent(lv_event_t *event)
{
    SecretaryApp *app = static_cast<SecretaryApp *>(lv_event_get_user_data(event));
    if (app == nullptr || app->_content == nullptr) {
        return;
    }
    intptr_t delta = reinterpret_cast<intptr_t>(lv_obj_get_user_data(lv_event_get_target(event)));
    if (delta < 0 && app->_reader_page > 0) {
        --app->_reader_page;
    } else if (delta > 0 && (static_cast<size_t>(app->_reader_page + 1) * READER_PAGE_BYTES) < app->_content_size) {
        ++app->_reader_page;
    }
    app->appendReaderText(app->_content);
}

void SecretaryApp::tickClock(lv_timer_t *timer)
{
    SecretaryApp *app = timer == nullptr ? nullptr : static_cast<SecretaryApp *>(timer->user_data);
    if (timer == nullptr) {
        return;
    }
    if (app == nullptr || app->_page != SecretaryPage::Clock || app->_clock == nullptr) {
        lv_timer_del(timer);
        return;
    }
    SecretaryClockPage::update(app->_clock);
}