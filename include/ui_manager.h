#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <M5Unified.h>

typedef enum {
    UI_PAGE_DASHBOARD, 
    UI_PAGE_PASSENGER, 
    UI_PAGE_UNION,     
    UI_PAGE_REPORTS,   
    UI_PAGE_SETTINGS,  
    UI_PAGE_HISTORY,   
    UI_PAGE_LOGOUT     
} ui_page_t;

// Maintain original function signatures
void ui_init(void);
void ui_manager_handle_touch(m5::touch_detail_t &t);
void ui_create_header(void); // Removed parent as M5Unified uses a global display
void ui_goto_page(ui_page_t page);
void ui_update_status_bar(bool gps_fixed, bool cloud_conn, int batt);
void ui_back_to_dash_cb(void); // Signature kept simple for your button logic

#endif