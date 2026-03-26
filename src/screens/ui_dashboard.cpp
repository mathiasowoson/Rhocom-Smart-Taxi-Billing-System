#include "screens/ui_dashboard.h"
#include "ui_manager.h"

// Tell the compiler these are external CONSTANT arrays (how LVGL stores fonts)
extern "C" {
    extern const lv_font_t lv_font_montserrat_48;
    extern const lv_font_t lv_font_montserrat_12;
}

lv_obj_t * ui_dashboard_screen = NULL;
lv_obj_t * speed_label = NULL;

// Event handler for dashboard buttons
static void dashboard_event_cb(lv_event_t * e) {
    lv_obj_t * btn = lv_event_get_target(e);
    int btn_id = (int)lv_event_get_user_data(e);

    switch(btn_id) {
        case 1: ui_goto_passenger_mgmt(); break; // PASSENGERS
        case 2: ui_goto_union_validation(); break; // UNION/LEVY
        // Add other cases as we build them
    }
}

void ui_dashboard_init(void) {
    ui_dashboard_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui_dashboard_screen, lv_color_hex(0x000000), 0);

    // 1. Create the Global Rhocom Header
    ui_create_header(ui_dashboard_screen);

    // 2. SPEEDOMETER AREA (Added between Header and Buttons)
    speed_label = lv_label_create(ui_dashboard_screen);
    lv_obj_set_style_text_font(speed_label, &lv_font_montserrat_48, 0); 
    lv_obj_set_style_text_color(speed_label, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(speed_label, "00");
    lv_obj_align(speed_label, LV_ALIGN_TOP_MID, 0, 45); // Just below header

    lv_obj_t * unit_lbl = lv_label_create(ui_dashboard_screen);
    lv_label_set_text(unit_lbl, "km/h");
    lv_obj_set_style_text_color(unit_lbl, lv_color_hex(0xFF0000), 0); // Red Accent
    lv_obj_align_to(unit_lbl, speed_label, LV_ALIGN_OUT_RIGHT_BOTTOM, 5, -10);

    // 2. Create a Container for the 6 Buttons (Grid)
    lv_obj_t * cont = lv_obj_create(ui_dashboard_screen);
    lv_obj_set_size(cont, 300, 130);
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_bg_opa(cont, 0, 0); // Transparent background
    lv_obj_set_style_border_width(cont, 0, 0);

    // Define 2 columns and 3 rows
    static lv_coord_t col_dsc[] = {140, 140, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {50, 50, 50, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_layout(cont, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(cont, col_dsc, row_dsc);

    // 3. Helper to create buttons inside the grid
    const char * btn_names[] = {"PASSENGERS", "UNION/LEVY", "REPORTS", "SETTINGS", "HISTORY", "LOGOUT"};
    
    for(int i = 0; i < 6; i++) {
        int col = i % 2;
        int row = i / 2;

        lv_obj_t * btn = lv_btn_create(cont);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x1A1A1A), 0); // Dark Grey
        lv_obj_set_style_border_color(btn, lv_color_hex(0x444444), 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);
        
        // Pass the index (i+1) as user data so the callback knows which button was clicked
        lv_obj_add_event_cb(btn, dashboard_event_cb, LV_EVENT_CLICKED, (void*)(i + 1));

        lv_obj_t * label = lv_label_create(btn);
        lv_label_set_text(label, btn_names[i]);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
        lv_obj_center(label);
    }
}

void ui_dashboard_display(void) {
    if(ui_dashboard_screen) {
        lv_scr_load(ui_dashboard_screen);
    }
}

void ui_update_dashboard_speed(float speed) {
    // if(speed_label != NULL && lv_scr_act() == ui_dashboard_screen) {
    //     lv_label_set_text_fmt(speed_label, "%02d", (int)speed);
    // }
    if(speed_label != NULL) { 
        lv_label_set_text_fmt(speed_label, "%02d", (int)speed);
    } else {
        Serial.println("UI ERROR: speed_label is NULL!");
    }
}

