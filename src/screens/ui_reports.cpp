#include "screens/ui_reports.h"
#include "driver_logic.h"

// Note: Ensure dailyUnionTotal and validCheckinsToday are externed 
// from your billing_logic.cpp or similar.
extern float dailyUnionTotal;
extern int validCheckinsToday;

void ui_reports_init(void) {
    M5.Display.startWrite();
    M5.Display.fillScreen(TFT_BLACK);

    // 1. Global Header
    ui_create_header();

    // 2. Back Button
    M5.Display.fillRoundRect(5, 40, 65, 35, 4, 0x39C7); // Grey-Blue
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setTextSize(1);
    M5.Display.drawString("< BACK", 12, 50);

    // 3. Page Title
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.drawCenterString("DAILY REPORTS", 160, 48);

    // 4. Summary Box (Replaces LVGL Container)
    M5.Display.drawRoundRect(20, 90, 280, 110, 8, TFT_DARKGREY);
    M5.Display.fillRect(21, 91, 278, 108, 0x18C3); // Very dark grey background

    // 5. Report Data
    M5.Display.setTextSize(1);              // Force size back to small
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.setTextColor(TFT_GREEN);
    char revBuf[32];
    sprintf(revBuf, "Total Revenue: N%.2f", dailyUnionTotal);
    M5.Display.drawCenterString(revBuf, 160, 110);

    M5.Display.setTextColor(TFT_WHITE);
    char tripBuf[32];
    sprintf(tripBuf, "Total Trips: %d", validCheckinsToday);
    M5.Display.drawCenterString(tripBuf, 160, 135);

    M5.Display.setTextColor(TFT_CYAN);
    M5.Display.drawCenterString("Status: Synced", 160, 165);

    M5.Display.endWrite();
}

void ui_reports_handle_touch(m5::touch_detail_t &t) {
    // 1. Guard to ensure we only process the initial touch
    if (!t.wasPressed()) return;

    // 2. Back Button Widget (Top Left)
    // Coords: x:5-70, y:40-75 (aligned with your visual layout)
    if (t.y > 40 && t.y < 75 && t.x > 5 && t.x < 70) {
        
    
        if (isSoundEnabled) {
            M5.Speaker.tone(2000, 50);
        }
        
        ui_goto_page(UI_PAGE_DASHBOARD);
    }

    // Add future report interactions (e.g., date selection, export) here
}