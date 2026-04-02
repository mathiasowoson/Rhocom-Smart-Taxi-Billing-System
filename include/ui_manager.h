#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "config.h"

// 1. The Master List of all pages
typedef enum {
    UI_PAGE_DASHBOARD, 
    UI_PAGE_PASSENGER, 
    UI_PAGE_UNION,     
    UI_PAGE_REPORTS,   
    UI_PAGE_SETTINGS,  
    UI_PAGE_HISTORY,   
    UI_PAGE_LOGOUT     
} ui_page_t;

// 2. Navigation & Manager Functions
void ui_init(void);
void ui_goto_page(ui_page_t page);
void ui_back_to_dash_cb(lv_event_t * e);

// 3. Page Initialization Functions (The "Factories")
// These MUST return lv_obj_t* now so the manager can store them in RAM
// lv_obj_t* ui_dashboard_init(void);
// lv_obj_t* ui_passenger_init(void);
// lv_obj_t* ui_union_init(void);    
// lv_obj_t* ui_reports_init(void);  
// lv_obj_t* ui_settings_init(void); 
// lv_obj_t* ui_history_init(void);  

// 4. Shared UI Elements
void ui_create_header(lv_obj_t* parent);
void ui_update_status_bar(bool gps_fixed, bool cloud_conn, int batt);

#endif