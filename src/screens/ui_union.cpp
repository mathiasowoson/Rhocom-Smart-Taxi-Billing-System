#include "ui_manager.h"
#include "screens/ui_union.h"
#include "screens/ui_dashboard.h"
#include "billing_logic.h" // Needed to call validation functions
#include "blynkGsm_logic.h"   // Needed to sync revenue to phone

lv_obj_t* ui_union_screen = NULL;

// static lv_obj_t * union_screen;
static lv_obj_t * kb;
static lv_obj_t * ta; // Text area for Station ID
static String current_selected_union = "";

// --- Helper: Show Message Box ---
void show_status_msg(const char * msg, lv_color_t color) {
    // Create the message box on the CURRENT screen, not NULL (NULL creates it on a top layer that can freeze)
    lv_obj_t * mbox = lv_msgbox_create(ui_union_screen, "Union Status", msg, NULL, true); 
    lv_obj_set_style_bg_color(mbox, color, 0);
    lv_obj_set_style_text_color(mbox, lv_color_white(), 0);
    lv_obj_center(mbox);
    
    // Auto-delete after 2 seconds
    lv_obj_del_delayed(mbox, 2000);
}

// Event for the Keyboard "Check" icon
static void kb_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);

    // LV_EVENT_READY is triggered by the "Check" (Enter) icon
    if(code == LV_EVENT_READY) {
        const char * entered_id = lv_textarea_get_text(ta);
        
        // Use the logic function we created at billing_logic
        // the funtion validate base on the input provided here to 
        // return either 1 or 2
        int status = validate_union_id_status(String(entered_id), current_selected_union);

        if(status == 1) {
            show_status_msg("Paid Successfully!", lv_palette_main(LV_PALETTE_GREEN));
            blynk_gsm_sync(); // Push new revenue to Blynk immediately
        } else {
            show_status_msg("Incorrect ID", lv_palette_main(LV_PALETTE_RED));
        }

        // Clean up: Reset text and hide keyboard/textarea
        lv_textarea_set_text(ta, "");
        lv_obj_add_flag(ta, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }

    // Added: Hide keyboard if user clicks "Cancel" (the X button on some kbs)
    if(code == LV_EVENT_CANCEL) {
        lv_obj_add_flag(ta, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
}

// Event for the 4 Grid Buttons
static void union_btn_event_cb(lv_event_t * e) {
    lv_obj_t * btn = lv_event_get_target(e);
    
    // Safety check: get label text
    lv_obj_t * label = lv_obj_get_child(btn, 1); 
    if (label == NULL) return;
    
    String btnText = lv_label_get_text(label);

    // Clean the string (e.g., "Park Fee (N200)" -> "Park Fee")
   if(btnText.indexOf("Park") != -1) current_selected_union = "Park Fee";
    else if(btnText.indexOf("Maintenance") != -1) current_selected_union = "Maintenance";
    else if(btnText.indexOf("Checkpoint") != -1) current_selected_union = "Checkpoint";
    else current_selected_union = "Emergency";

    Serial.printf("Selected for Validation: %s\n", current_selected_union.c_str());
    
    // SHOW AND MOVE TO FRONT
    lv_obj_clear_flag(ta, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_textarea_set_text(ta, "");

    // FORCE TO FRONT: This ensures the list doesn't hide the input
    lv_obj_move_foreground(ta);
    lv_obj_move_foreground(kb);
}



void ui_union_init(void) {
    ui_union_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui_union_screen, lv_color_black(), 0);

    // 1. Header
    lv_obj_t * label = lv_label_create(ui_union_screen);
    lv_label_set_text(label, "UNION VALIDATION");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    // 2. Buttons List (Moved up slightly)
    lv_obj_t * list = lv_list_create(ui_union_screen);
    lv_obj_set_size(list, 280, 140);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 45);

    lv_obj_t * b1 = lv_list_add_btn(list, LV_SYMBOL_HOME, "Park Fee (N200)");
    lv_obj_t * b2 = lv_list_add_btn(list, LV_SYMBOL_SETTINGS, "Maintenance");
    lv_obj_t * b3 = lv_list_add_btn(list, LV_SYMBOL_OK, "Checkpoint (N100)");
    lv_obj_t * b4 = lv_list_add_btn(list, LV_SYMBOL_WARNING, "Emergency Levy");

    lv_obj_add_event_cb(b1, union_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(b2, union_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(b3, union_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(b4, union_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // 3. Text Area - MOVED UP so the keyboard doesn't hide it
    ta = lv_textarea_create(ui_union_screen);
    lv_obj_set_size(ta, 240, 45);
    // Align to the bottom of the list, but above the keyboard
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 60); 
    lv_textarea_set_placeholder_text(ta, "Enter ID");
    lv_obj_add_flag(ta, LV_OBJ_FLAG_HIDDEN);

    // 4. Keyboard
    kb = lv_keyboard_create(ui_union_screen);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_UPPER); 
    lv_obj_set_size(kb, 320, 140);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(kb, ta);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    
    lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_ALL, NULL); // Changed to ALL for safety


    // 5. Back Button
    lv_obj_t * back_btn = lv_btn_create(ui_union_screen);
    lv_obj_set_size(back_btn, 40, 30); // Smaller back button
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_t * back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_add_event_cb(back_btn, ui_back_to_dash_cb, LV_EVENT_CLICKED, NULL);

}