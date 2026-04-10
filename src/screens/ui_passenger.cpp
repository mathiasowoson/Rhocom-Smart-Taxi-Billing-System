#include "ui_manager.h"
#include "screens/ui_passenger.h"
#include "billing_logic.h"

lv_obj_t* ui_passenger_screen = NULL;

// static lv_obj_t * ui_passenger_screen;
static lv_obj_t * tag_list; // The scrolling container

// --- Event Handlers ---

// When a Tag in the list is clicked
static void tag_clicked_cb(lv_event_t * e) {
    int tag_id = (int)lv_event_get_user_data(e);
    ui_show_passenger_modal(tag_id); // Open the Pop-Over
}

// When "END TRIP" is clicked inside the Pop-Over
static void end_trip_cb(lv_event_t * e) {
    int tag_id = (int)(uintptr_t)lv_event_get_user_data(e);
    
    // 1. Call logic to finalize fare
    calculate_final_fare(tag_id); 
    // 2. Read the final value from our data structure instead of the function return
    float final_fare = tags[tag_id].currentFare;
    Serial.printf("UI: Trip ended for Tag %d. Final Fare: N%.2f\n", tag_id, final_fare);
    
    // 2. Transition to QR screen (We will build this later)
    // ui_goto_qr_display(tag_id, final_fare);
    
    // For now, close modal
    lv_obj_t * target = lv_event_get_target(e);
    lv_obj_t * modal = lv_obj_get_parent(target);
    if(modal) lv_obj_del(modal);
}

// --- Screen Initialization ---

void ui_passenger_init(void) {
    ui_passenger_screen = lv_obj_create(NULL);

    lv_obj_set_style_bg_color(ui_passenger_screen, lv_color_hex(0x000000), 0);
    ui_create_header(ui_passenger_screen);

    // Create a Scrolling List for Tags
    tag_list = lv_list_create(ui_passenger_screen);
    lv_obj_set_size(tag_list, 300, 180);
    lv_obj_align(tag_list, LV_ALIGN_BOTTOM_MID, 0, -5);

     lv_obj_t * back_btn = lv_btn_create(ui_passenger_screen);
    lv_obj_set_size(back_btn, 50, 35);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_t * back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT);
    lv_obj_add_event_cb(back_btn, ui_back_to_dash_cb, LV_EVENT_CLICKED, NULL);

}


// Function to add a Tag (can be triggered by Blynk/Cloud)
void ui_add_passenger_tag(int tag_id, const char* name) {
    if(!tag_list) return;

    char buf[32];
    snprintf(buf, sizeof(buf), "Passenger Tag #%03d", tag_id);

    lv_obj_t * btn = lv_list_add_btn(tag_list, LV_SYMBOL_DIRECTORY, buf);
    lv_obj_add_event_cb(btn, tag_clicked_cb, LV_EVENT_CLICKED, (void*)tag_id);
}

// --- The Pop-Over (Modal) ---

void ui_show_passenger_modal(int tag_id) {
    // Create a dark background overlay (dimming effect)
    lv_obj_t * obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(obj, 320, 240);
    lv_obj_set_style_bg_color(obj, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_50, 0);
    lv_obj_set_pos(obj, 0, 0);

    // The actual Pop-Over window
    lv_obj_t * modal = lv_obj_create(obj);
    lv_obj_set_size(modal, 260, 160);
    lv_obj_center(modal);

    lv_obj_t * title = lv_label_create(modal);
    lv_label_set_text_fmt(title, "PASSENGER #%03d", tag_id);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    // Current Fare Display (Placeholder)
    lv_obj_t * fare = lv_label_create(modal);
    lv_label_set_text(fare, "Current Fare: N0.00");
    lv_obj_align(fare, LV_ALIGN_CENTER, 0, -10);

    // END TRIP Button
    lv_obj_t * btn_end = lv_btn_create(modal);
    lv_obj_set_size(btn_end, 100, 40);
    lv_obj_align(btn_end, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_bg_color(btn_end, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_add_event_cb(btn_end, end_trip_cb, LV_EVENT_CLICKED, (void*)tag_id);

    lv_obj_t * l_end = lv_label_create(btn_end);
    lv_label_set_text(l_end, "END TRIP");
    lv_obj_center(l_end);

    // CONTINUE Button
    lv_obj_t * btn_cont = lv_btn_create(modal);
    lv_obj_set_size(btn_cont, 100, 40);
    lv_obj_align(btn_cont, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    lv_obj_add_event_cb(btn_cont, [](lv_event_t * e){
    // 1. Get the button
    lv_obj_t * btn = lv_event_get_target(e);
    // 2. Get the Modal (Parent of Button)
    lv_obj_t * modal_win = lv_obj_get_parent(btn);
    // 3. Get the Overlay (Parent of Modal)
    lv_obj_t * overlay = lv_obj_get_parent(modal_win);
    
    // Delete the Overlay and everything inside it will die too
    lv_obj_del(overlay);
}, LV_EVENT_CLICKED, NULL);

    lv_obj_t * l_cont = lv_label_create(btn_cont);
    lv_label_set_text(l_cont, "BACK");
    lv_obj_center(l_cont);
}