#pragma once

#include "esp_brookesia.hpp"
#include "pages/PageCatalog.hpp"

class SecretaryApp : public ESP_Brookesia_PhoneApp {
public:
    SecretaryApp();
    ~SecretaryApp();

    bool init(void) override;
    bool run(void);
    bool back(void);
    bool close(void);

private:
    static void pageButtonEvent(lv_event_t *event);
    static void navigateAsync(void *arg);
    static void fetchButtonEvent(lv_event_t *event);
    static void serviceFetchEvent(lv_event_t *event);
    static void saveUrlEvent(lv_event_t *event);
    static void readerNavEvent(lv_event_t *event);
    static void tickClock(lv_timer_t *timer);
    static void fetchTask(void *arg);
    static void serviceFetchTask(void *arg);
    static void completeFetch(void *arg);

    void showPage(SecretaryPage page);
    void showHome(void);
    void showReader(void);
    void showSettings(void);
    void showInfoPage(SecretaryPage page);
    void createHeader(const char *title);
    void appendReaderText(const char *text);

    SecretaryPage _page;
    lv_obj_t *_body;
    lv_obj_t *_url_input;
    lv_obj_t *_reader;
    lv_obj_t *_status;
    lv_obj_t *_clock;
    char _url[256];
    char _request_url[256];
    char *_content;
    size_t _content_size;
    uint16_t _reader_page;
    bool _fetching;
    SecretaryPage _fetch_page;
};