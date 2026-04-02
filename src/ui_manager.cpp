#include "ui_manager.h"
// Include the headers for individual screens (we will create these next)
#include "screens/ui_dashboard.h"
#include "screens/ui_passenger.h"
#include "screens/ui_union.h"
#include "screens/ui_settings.h"
#include "screens/ui_history.h"
#include "screens/ui_reports.h"


static lv_obj_t * current_screen = NULL;

// Objects for the Header
static lv_obj_t * header_bar;
static lv_obj_t * label_gps;
static lv_obj_t * label_cloud;
static lv_obj_t * label_batt;


void ui_init(void) {
   ui_goto_page(UI_PAGE_DASHBOARD);
}

void ui_create_header(lv_obj_t* parent) {
    // 1. Create the Header Container
    header_bar = lv_obj_create(parent);
    lv_obj_set_size(header_bar, 320, 35);
    lv_obj_set_pos(header_bar, 0, 0);
    lv_obj_set_style_bg_color(header_bar, lv_palette_main(LV_PALETTE_BLUE_GREY), 0);
    lv_obj_set_style_border_side(header_bar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_radius(header_bar, 0, 0);

    // 2. Company Name (RHOCOM)
    lv_obj_t * title = lv_label_create(header_bar);
    lv_label_set_text(title, COMPANY_NAME);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 5, 0);

    // 3. GPS Icon (🛰️)
    label_gps = lv_label_create(header_bar);
    lv_label_set_text(label_gps, LV_SYMBOL_GPS);
    lv_obj_set_style_text_color(label_gps, lv_palette_main(LV_PALETTE_GREY), 0); // Default: Grey (No Fix)
    lv_obj_align(label_gps, LV_ALIGN_CENTER, -20, 0);

    // 4. Cloud/Blynk Icon (☁️)
    label_cloud = lv_label_create(header_bar);
    lv_label_set_text(label_cloud, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(label_cloud, lv_palette_main(LV_PALETTE_RED), 0); // Default: Red (No Connection)
    lv_obj_align(label_cloud, LV_ALIGN_CENTER, 20, 0);

    // 5. Battery Icon (🔋)
    label_batt = lv_label_create(header_bar);
    lv_label_set_text(label_batt, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(label_batt, lv_color_white(), 0);
    lv_obj_align(label_batt, LV_ALIGN_RIGHT_MID, -5, 0);
}



void ui_goto_page(ui_page_t page) {
    switch (page) {
        case UI_PAGE_DASHBOARD:
            if (ui_dashboard_screen == NULL) ui_dashboard_init();
            lv_scr_load(ui_dashboard_screen);
            break;

        case UI_PAGE_PASSENGER:
            if (ui_passenger_screen == NULL) ui_passenger_init();
            lv_scr_load(ui_passenger_screen);
            break;

        case UI_PAGE_UNION:
            if (ui_union_screen == NULL) ui_union_init();
            lv_scr_load(ui_union_screen);
            break;

        case UI_PAGE_SETTINGS:
            if (ui_settings_screen == NULL) ui_settings_init();
            lv_scr_load(ui_settings_screen);
            break;

        case UI_PAGE_REPORTS:
            if (ui_reports_screen == NULL) ui_reports_init();
            lv_scr_load(ui_reports_screen);
            break;

        case UI_PAGE_HISTORY:
            if (ui_history_screen == NULL) ui_history_init();
            lv_scr_load(ui_history_screen);
            break;

        case UI_PAGE_LOGOUT:
            ESP.restart(); 
            break;
    }
}
// This updates the header icons in real-time
void ui_update_status_bar(bool gps_fixed, bool cloud_conn, int batt) {
    if (header_bar == NULL) return;

    // Update GPS Color
    lv_obj_set_style_text_color(label_gps, gps_fixed ? lv_palette_main(LV_PALETTE_GREEN) : lv_palette_main(LV_PALETTE_GREY), 0);
    
    // Update Cloud/Blynk Color
    lv_obj_set_style_text_color(label_cloud, cloud_conn ? lv_palette_main(LV_PALETTE_BLUE) : lv_palette_main(LV_PALETTE_RED), 0);
    
    // Update Battery Label (simplified)
    if (batt < 20) lv_label_set_text(label_batt, LV_SYMBOL_BATTERY_EMPTY);
    else if (batt < 80) lv_label_set_text(label_batt, LV_SYMBOL_BATTERY_3);
    else lv_label_set_text(label_batt, LV_SYMBOL_BATTERY_FULL);
}

// Global dashboard call back button declaration
void ui_back_to_dash_cb(lv_event_t * e) {
    // We only care about the CLICKED event
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        Serial.println("Nav: Returning to Dashboard");
        ui_goto_page(UI_PAGE_DASHBOARD);
    }
}