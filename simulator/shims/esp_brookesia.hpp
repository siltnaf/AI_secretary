#pragma once

#include "lvgl.h"

class ESP_Brookesia_PhoneApp {
public:
    using CloseHandler = void (*)();

    ESP_Brookesia_PhoneApp(const char *, const void *, bool) {}
    virtual ~ESP_Brookesia_PhoneApp() = default;
    virtual bool init() { return true; }
    virtual bool run() = 0;
    virtual bool back() = 0;
    virtual bool close() { return true; }
    static void setCloseHandler(CloseHandler handler) { close_handler_ = handler; }
    bool notifyCoreClosed() const {
        if (close_handler_ != nullptr) close_handler_();
        return true;
    }

private:
    inline static CloseHandler close_handler_ = nullptr;
};