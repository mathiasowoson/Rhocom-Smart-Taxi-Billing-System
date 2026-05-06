#include "screens/ui_dashboard.h"
#include "ui_manager.h"

float current_speed = 0.0;
bool dashboard_needs_update = false;


// Define button areas (Grid-like hotspots)
struct DashboardBtn {
    int x, y, w, h;
    ui_page_t target;
    const char* label;
};

DashboardBtn menu_btns[] = {
    {10,  110, 145, 40, UI_PAGE_PASSENGER, "PASSENGERS"},
    {165, 110, 145, 40, UI_PAGE_UNION,     "UNION/LEVY"},
    {10,  155, 145, 40, UI_PAGE_REPORTS,   "REPORTS"},
    {165, 155, 145, 40, UI_PAGE_SETTINGS,  "SETTINGS"},
    {10,  200, 145, 40, UI_PAGE_HISTORY,   "HISTORY"},
    {165, 200, 145, 40, UI_PAGE_LOGOUT,    "LOGOUT"}
};

void ui_dashboard_init(void) {
    M5.Display.startWrite();
    
    // Project Title
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setTextSize(1);
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.drawCenterString("SMART TAXI BILLING SYSTEM", 160, 40);

    // 1. Draw Speedometer Area
    // M5.Display.setTextColor(TFT_WHITE);
    // M5.Display.setTextSize(1);
    // M5.Display.setFont(&fonts::FreeSansBold24pt7b); // Equivalent to Montserrat 48
    // M5.Display.drawCenterString("00", 160, 45);
    
    M5.Display.setTextColor(TFT_RED);
    M5.Display.setTextSize(1);
    M5.Display.setFont(&fonts::FreeSans9pt7b); // Equivalent to Montserrat 12
    M5.Display.drawString("km/h", 205, 75);

    // 2. Draw Grid Buttons
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    for (int i = 0; i < 6; i++) {
        M5.Display.fillRoundRect(menu_btns[i].x, menu_btns[i].y, menu_btns[i].w, menu_btns[i].h, 4, 0x1A1A); // Hex 0x1A1A1A
        M5.Display.drawRoundRect(menu_btns[i].x, menu_btns[i].y, menu_btns[i].w, menu_btns[i].h, 4, 0x4444); // Border
        M5.Display.setTextColor(TFT_WHITE);
        M5.Display.setTextSize(1);
        M5.Display.drawCenterString(menu_btns[i].label, menu_btns[i].x + (menu_btns[i].w/2), menu_btns[i].y + 12);
    }
    
    M5.Display.endWrite();
}

void ui_update_dashboard_speed(float speed) {
    // 1. Check current_page to stop the overlay on other screens
    if (current_active_page != UI_PAGE_DASHBOARD) return; 

    current_speed = speed;
    
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK); 
    M5.Display.setTextSize(1);
    M5.Display.setFont(&fonts::FreeSansBold24pt7b);
    
    // Dynamic GPS speed display
    char buf[4];
    sprintf(buf, "%02d", (int)speed);

    // 3. Lowered Y to 65 to make room for the Project Name at the top
    M5.Display.drawCenterString(buf, 160, 65); 
}

void ui_dashboard_handle_touch(m5::touch_detail_t &t) {
    if (t.wasPressed()) {
        for (int i = 0; i < 6; i++) {
            // Check if touch is within button boundaries
            if (t.x >= menu_btns[i].x && t.x <= (menu_btns[i].x + menu_btns[i].w) &&
                t.y >= menu_btns[i].y && t.y <= (menu_btns[i].y + menu_btns[i].h)) {
                
                // Visual feedback (Flash button)
                M5.Display.drawRoundRect(menu_btns[i].x, menu_btns[i].y, menu_btns[i].w, menu_btns[i].h, 4, TFT_WHITE);
                M5.Speaker.tone(2000, 50);
                
                // Navigation
                ui_goto_page(menu_btns[i].target);
                break;
            }
        }
    }
}