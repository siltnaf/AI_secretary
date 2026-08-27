#include "ContentPage.hpp"

namespace SecretaryContentPage {

lv_obj_t *create(lv_obj_t *parent, const char *title, const char *description, const char *action_label)
{
    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_set_size(container, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 10, 0);

    lv_obj_t *heading = lv_label_create(container);
    lv_label_set_text(heading, title);
    lv_obj_set_style_text_font(heading, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(heading, lv_color_hex(0x16697A), 0);
    lv_obj_align(heading, LV_ALIGN_TOP_MID, 0, 24);

    lv_obj_t *copy = lv_label_create(container);
    lv_label_set_text(copy, description);
    lv_label_set_long_mode(copy, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(copy, lv_pct(100) - 30);
    lv_obj_set_style_text_align(copy, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(copy, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(copy, lv_color_hex(0x3E4C59), 0);
    lv_obj_align(copy, LV_ALIGN_TOP_MID, 0, 105);

    lv_obj_t *action = lv_btn_create(container);
    lv_obj_set_size(action, 210, 56);
    lv_obj_align(action, LV_ALIGN_BOTTOM_MID, 0, -42);
    lv_obj_set_style_bg_color(action, lv_color_hex(0x16697A), 0);
    lv_obj_set_style_radius(action, 4, 0);
    lv_obj_t *label = lv_label_create(action);
    lv_label_set_text(label, action_label);
    lv_obj_center(label);
    return action;
}

}