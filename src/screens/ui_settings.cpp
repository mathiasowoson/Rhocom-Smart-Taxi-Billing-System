#include "ui_manager.h"
#include "screens/ui_settings.h"
#include "screens/ui_dashboard.h"
#include "driver_logic.h"
#include "config.h"

lv_obj_t * ui_settings_screen = NULL;

// --- Event Handlers ---

static void brightness_slider_cb(lv_event_t * e) {
    lv_obj_t * slider = lv_event_get_target(e);
    driver_set_brightness(lv_slider_get_value(slider));
}

static void mode_sw_cb(lv_event_t * e) {
    lv_obj_t * sw = lv_event_get_target(e);
    isPublicMode = lv_obj_has_state(sw, LV_STATE_CHECKED);
    // You could also call blynk_sync_data() here if needed
}

static void sound_sw_cb(lv_event_t * e) {
    lv_obj_t * sw = lv_event_get_target(e);
    driver_toggle_sound(lv_obj_has_state(sw, LV_STATE_CHECKED));
}

static void factory_reset_confirm_cb(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_current_target(e);
    uint16_t btn_id = lv_msgbox_get_active_btn(obj);

    if(btn_id == 0) { // Index 0 is "Proceed"
        driver_factory_reset(); // NOW we do the hardware reset
    } else {
        lv_msgbox_close(obj);   // Just close the box and go back to settings
    }
}

void ui_show_factory_reset_warning(void) {
    static const char * btns[] = {"Proceed", "Cancel", ""};

    lv_obj_t * mbox = lv_msgbox_create(NULL, "FACTORY RESET", 
        "This will wipe all revenue and settings. Are you sure?", 
        btns, true);
    
    lv_obj_set_style_bg_color(mbox, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_text_color(mbox, lv_color_white(), 0);
    lv_obj_center(mbox);

    // This tells the box to run 'factory_reset_confirm_cb' when a button is clicked
    lv_obj_add_event_cb(mbox, factory_reset_confirm_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

static void action_btn_cb(lv_event_t * e) {
    // Get the hidden "user data" string we attached to the button (e.g., "off", "reset", "factory")
    const char * action = (const char *)lv_event_get_user_data(e);
    
    if (strcmp(action, "off") == 0) {
        // Still works: Shuts down the M5Core2 immediately
        driver_system_shutdown();
    } 
    else if (strcmp(action, "reset") == 0) {
        // Still works: Reboots the ESP32 immediately
        driver_system_restart();
    } 
    else if (strcmp(action, "factory") == 0) {
        // CHANGED: Instead of resetting now, show the warning box
        ui_show_factory_reset_warning(); 
    }
}


void ui_settings_init(void) {
    ui_settings_screen = lv_obj_create(NULL);

    // 1. Header & Back Button
    lv_obj_t * header = lv_label_create(ui_settings_screen);
    lv_label_set_text(header, "SYSTEM SETTINGS");
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t * back_btn = lv_btn_create(ui_settings_screen);
    lv_obj_set_size(back_btn, 50, 35);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_t * back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT);
    lv_obj_add_event_cb(back_btn, ui_back_to_dash_cb, LV_EVENT_CLICKED, NULL);


    // 2. Scrolling Container for Settings
    lv_obj_t * cont = lv_obj_create(ui_settings_screen);
    lv_obj_set_size(cont, 320, 190);
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(cont, 15, 0);

    // --- Public/Private Mode ---
    lv_obj_t * row_mode = lv_obj_create(cont);
    lv_obj_set_size(row_mode, 270, 45);
    lv_obj_t * lbl_mode = lv_label_create(row_mode);
    lv_label_set_text(lbl_mode, "Public Mode");
    lv_obj_align(lbl_mode, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_t * sw_mode = lv_switch_create(row_mode);
    lv_obj_align(sw_mode, LV_ALIGN_RIGHT_MID, 0, 0);
    if(isPublicMode) lv_obj_add_state(sw_mode, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw_mode, mode_sw_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // --- Brightness ---
    lv_obj_t * row_br = lv_obj_create(cont);
    lv_obj_set_size(row_br, 270, 60);
    lv_obj_t * lbl_br = lv_label_create(row_br);
    lv_label_set_text(lbl_br, "Brightness");
    lv_obj_align(lbl_br, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_t * slider = lv_slider_create(row_br);
    lv_obj_set_size(slider, 180, 10);
    lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_slider_set_range(slider, 10, 255);
    lv_slider_set_value(slider, last_brightness, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, brightness_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // --- Sound Toggle ---
    lv_obj_t * row_snd = lv_obj_create(cont);
    lv_obj_set_size(row_snd, 270, 45);
    lv_obj_t * lbl_snd = lv_label_create(row_snd);
    lv_label_set_text(lbl_snd, "Button Sounds");
    lv_obj_align(lbl_snd, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_t * sw_snd = lv_switch_create(row_snd);
    lv_obj_align(sw_snd, LV_ALIGN_RIGHT_MID, 0, 0);
    if(isSoundEnabled) lv_obj_add_state(sw_snd, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw_snd, sound_sw_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // --- Action Buttons ---
    lv_obj_t * btn_restart = lv_btn_create(cont);
    lv_obj_set_size(btn_restart, 270, 40);
    lv_obj_t * lbl_res = lv_label_create(btn_restart);
    lv_label_set_text(lbl_res, "Restart System");
    lv_obj_center(lbl_res);
    lv_obj_add_event_cb(btn_restart, action_btn_cb, LV_EVENT_CLICKED, (void*)"reset");

    lv_obj_t * btn_factory = lv_btn_create(cont);
    lv_obj_set_size(btn_factory, 270, 40);
    lv_obj_set_style_bg_color(btn_factory, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_t * lbl_fac = lv_label_create(btn_factory);
    lv_label_set_text(lbl_fac, "Factory Reset");
    lv_obj_center(lbl_fac);
    lv_obj_add_event_cb(btn_factory, action_btn_cb, LV_EVENT_CLICKED, (void*)"factory");

    lv_obj_t * btn_off = lv_btn_create(cont);
    lv_obj_set_size(btn_off, 270, 40);
    lv_obj_set_style_bg_color(btn_off, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_t * lbl_off = lv_label_create(btn_off);
    lv_label_set_text(lbl_off, "SHUTDOWN");
    lv_obj_center(lbl_off);
    lv_obj_add_event_cb(btn_off, action_btn_cb, LV_EVENT_CLICKED, (void*)"off");

}