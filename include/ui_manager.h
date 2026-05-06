#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <M5Unified.h>

extern int ui_scroll_offset;

typedef enum {
    UI_PAGE_DASHBOARD, 
    UI_PAGE_PASSENGER, 
    UI_PAGE_UNION,     
    UI_PAGE_REPORTS,   
    UI_PAGE_SETTINGS,  
    UI_PAGE_HISTORY,   
    UI_PAGE_LOGOUT     
} ui_page_t;

extern ui_page_t current_active_page;

// Maintain original function signatures
void ui_init(void);
void ui_manager_handle_touch(m5::touch_detail_t &t);
void ui_create_header(void); // Removed parent as M5Unified uses a global display
void ui_goto_page(ui_page_t page);
void ui_update_status_bar(bool gps_fixed, bool cloud_conn, int batt);
void draw_scrollable_button(int x, int y, int w, int h, const char* label, uint32_t color);

#endif