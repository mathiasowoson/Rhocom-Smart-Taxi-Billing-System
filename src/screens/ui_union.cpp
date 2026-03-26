#include "screens/ui_union.h"
#include "screens/ui_dashboard.h"
#include "ui_manager.h"

static lv_obj_t * union_screen;
static lv_obj_t * kb;
static lv_obj_t * ta; // Text area for Station ID

// Event for the 4 Grid Buttons
static void union_btn_event_cb(lv_event_t * e) {
    lv_obj_t * btn = lv_event_get_target(e);
    const char * txt = lv_list_get_btn_text(NULL, btn);
    Serial.printf("Union Fee Selected: %s\n", txt);
    
    // Show the Keypad to enter Station ID
    lv_obj_clear_flag(ta, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
}

// Event for the Back Button
static void back_to_dash_cb(lv_event_t * e) {
    ui_goto_dashboard();
}

void ui_goto_union_validation(void) {
    union_screen = lv_obj_create(NULL);
    
    // 1. Header Title
    lv_obj_t * label = lv_label_create(union_screen);
    lv_label_set_text(label, "UNION VALIDATION");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    // 2. The 4-Button Grid (List style for easy clicking)
    lv_obj_t * list = lv_list_create(union_screen);
    lv_obj_set_size(list, 280, 180);
    lv_obj_center(list);

    lv_obj_t * btn1 = lv_list_add_btn(list, LV_SYMBOL_HOME, "Park Fee (N200)");
    lv_obj_t * btn2 = lv_list_add_btn(list, LV_SYMBOL_SETTINGS, "Maintenance");
    lv_obj_t * btn3 = lv_list_add_btn(list, LV_SYMBOL_OK, "Checkpoint (N100)");
    lv_obj_t * btn4 = lv_list_add_btn(list, LV_SYMBOL_WARNING, "Emergency Levy");

    lv_obj_add_event_cb(btn1, union_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn2, union_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn3, union_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn4, union_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // 3. Station ID Text Area (Hidden until a fee is picked)
    ta = lv_textarea_create(union_screen);
    lv_obj_set_size(ta, 200, 40);
    lv_obj_align(ta, LV_ALIGN_CENTER, 0, -20);
    lv_textarea_set_placeholder_text(ta, "Enter Station ID");
    lv_obj_add_flag(ta, LV_OBJ_FLAG_HIDDEN);

    // 4. Numeric Keypad (Hidden until a fee is picked)
    kb = lv_keyboard_create(union_screen);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);
    lv_obj_set_size(kb, 320, 120);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(kb, ta);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);

    // 5. Back Button
    lv_obj_t * back_btn = lv_btn_create(union_screen);
    lv_obj_set_size(back_btn, 60, 30);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_t * back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_add_event_cb(back_btn, back_to_dash_cb, LV_EVENT_CLICKED, NULL);

    lv_scr_load(union_screen);
}