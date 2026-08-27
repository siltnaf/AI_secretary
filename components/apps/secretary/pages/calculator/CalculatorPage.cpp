#include "CalculatorPage.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

struct State {
    lv_obj_t *display;
    char expression[96];
};

const char *KEYS[] = {
    "C", LV_SYMBOL_BACKSPACE, "/", "x", "\n",
    "7", "8", "9", "-", "\n",
    "4", "5", "6", "+", "\n",
    "1", "2", "3", "%", "\n",
    "0", ".", "=", ""
};

double evaluate(const char *input)
{
    double total = 0;
    double term = 0;
    double number = 0;
    char add = '+';
    char multiply = 0;
    const char *cursor = input;
    while (*cursor) {
        char *end = nullptr;
        number = std::strtod(cursor, &end);
        if (end == cursor) return NAN;
        cursor = end;
        if (*cursor == '%') {
            number /= 100.0;
            ++cursor;
        }
        if (multiply == 'x') term *= number;
        else if (multiply == '/') {
            if (number == 0) return NAN;
            term /= number;
        } else term = number;

        if (*cursor == 'x' || *cursor == '/') {
            multiply = *cursor++;
            continue;
        }
        total += add == '-' ? -term : term;
        multiply = 0;
        if (*cursor == '+' || *cursor == '-') add = *cursor++;
        else if (*cursor != '\0') return NAN;
    }
    return total;
}

void destroy(lv_event_t *event)
{
    delete static_cast<State *>(lv_event_get_user_data(event));
}

void press(lv_event_t *event)
{
    State *state = static_cast<State *>(lv_event_get_user_data(event));
    lv_obj_t *matrix = lv_event_get_target(event);
    const char *key = lv_btnmatrix_get_btn_text(matrix, lv_btnmatrix_get_selected_btn(matrix));
    if (state == nullptr || key == nullptr) return;
    if (std::strcmp(key, "C") == 0) state->expression[0] = '\0';
    else if (std::strcmp(key, LV_SYMBOL_BACKSPACE) == 0) {
        const size_t length = std::strlen(state->expression);
        if (length > 0) state->expression[length - 1] = '\0';
    } else if (std::strcmp(key, "=") == 0) {
        const double result = evaluate(state->expression);
        if (std::isnan(result)) std::strcpy(state->expression, "Error");
        else std::snprintf(state->expression, sizeof(state->expression), "%.10g", result);
    } else if (std::strlen(state->expression) + std::strlen(key) < sizeof(state->expression) - 1) {
        if (std::strcmp(state->expression, "Error") == 0) state->expression[0] = '\0';
        std::strcat(state->expression, key);
    }
    lv_label_set_text(state->display, state->expression[0] ? state->expression : "0");
}

}

namespace SecretaryCalculatorPage {

lv_obj_t *create(lv_obj_t *parent)
{
    State *state = new State{};
    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_set_size(container, lv_pct(100), lv_pct(100));
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(container, 8, 0);
    lv_obj_add_event_cb(container, destroy, LV_EVENT_DELETE, state);

    state->display = lv_label_create(container);
    lv_label_set_text(state->display, "0");
    lv_obj_set_width(state->display, lv_pct(100));
    lv_obj_set_style_text_align(state->display, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_font(state->display, &lv_font_montserrat_38, 0);
    lv_obj_align(state->display, LV_ALIGN_TOP_RIGHT, -10, 35);

    lv_obj_t *matrix = lv_btnmatrix_create(container);
    lv_btnmatrix_set_map(matrix, KEYS);
    lv_obj_set_size(matrix, lv_pct(100), lv_pct(72));
    lv_obj_align(matrix, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_text_font(matrix, &lv_font_montserrat_24, 0);
    lv_obj_add_event_cb(matrix, press, LV_EVENT_VALUE_CHANGED, state);
    return container;
}

}