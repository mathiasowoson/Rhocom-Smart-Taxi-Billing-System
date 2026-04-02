#include "ui_manager.h"
#include "screens/ui_reports.h"

lv_obj_t* ui_reports_screen = NULL;

void ui_reports_init(void) {
     ui_reports_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui_reports_screen, lv_color_hex(0x000000), 0);

   lv_obj_t * header = lv_label_create(ui_reports_screen);
    lv_label_set_text(header, "DAILY REPORTS");
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t * back_btn = lv_btn_create(ui_reports_screen);
    lv_obj_set_size(back_btn, 50, 35);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_t * back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT);
    lv_obj_add_event_cb(back_btn, ui_back_to_dash_cb, LV_EVENT_CLICKED, NULL);

}