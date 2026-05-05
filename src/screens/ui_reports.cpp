#include "screens/ui_reports.h"
#include "driver_logic.h"
#include "ui_manager.h"

// Note: Ensure dailyUnionTotal and validCheckinsToday are externed 
// from your billing_logic.cpp or similar.
extern float dailyUnionTotal;
extern int validCheckinsToday;

void ui_reports_init(void) {
    M5.Display.startWrite();
    M5.Display.fillScreen(TFT_BLACK);
    
    // 1. Global Header
    ui_create_header();

    // 2. Back Button (Top Left)
    M5.Display.fillRoundRect(5, 40, 65, 35, 4, 0x39C7); // Grey-Blue
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.drawCenterString("<", 37, 50);

    // 3. Page Title
    M5.Display.drawCenterString("DAILY REPORTS", 160, 48);
    

    // 4. Summary Box Background
    M5.Display.drawRoundRect(20, 90, 280, 115, 8, TFT_DARKGREY);
    M5.Display.fillRect(21, 91, 278, 113, 0x18C3); // Very dark grey background

    // 5. Dynamic Report Data
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    
    // Total Revenue (Uses dailyUnionTotal)
    M5.Display.setTextColor(TFT_GREEN);
    char revBuf[32];
    snprintf(revBuf, sizeof(revBuf), "Total Revenue: N%.2f", dailyUnionTotal);
    M5.Display.drawCenterString(revBuf, 160, 100);

    // Total Union Station check-in today (Uses validCheckinsToday)
    M5.Display.setTextColor(TFT_WHITE);
    char unionBuf[32];
    snprintf(unionBuf, sizeof(unionBuf), "Valid Union: %d", validCheckinsToday);
    M5.Display.drawCenterString(unionBuf, 160, 120);

    // Total Trip today 
    M5.Display.setTextColor(TFT_WHITE);
    char tripBuf[32];
    snprintf(tripBuf, sizeof(tripBuf), "Total Trips: %d", validCheckinsToday);
    M5.Display.drawCenterString(tripBuf, 160, 140);

    // Sync Status
    M5.Display.setTextColor(TFT_CYAN);
    M5.Display.drawCenterString("Status: Synced", 160, 160);

    M5.Display.endWrite();
}

void ui_reports_handle_touch(m5::touch_detail_t &t) {

    // --- SCROLL BUTTON DETECTION ---
// x > 275 is the blue button area on the right
if (t.x > 270) {
    if (t.y > 85 && t.y < 145) { // Up Button
        ui_scroll_offset -= 40;
        if (ui_scroll_offset < 0) ui_scroll_offset = 0;
        ui_reports_init(); // Redraw to move content
        return;
    } 
    else if (t.y > 160 && t.y < 220) { // Down Button
        ui_scroll_offset += 40;
        ui_reports_init(); // Redraw to move content
        return;
    }
}
    
    // 1. Guard to ensure we only process the initial touch
    if (!t.wasPressed()) return;

    // --- ZONE 1: STATIC BACK BUTTON (Top Left) ---
    // Coords: x:5-70, y:40-75
    if (t.x > 5 && t.x < 70 && t.y > 40 && t.y < 75) {
        if (isSoundEnabled) {
            M5.Speaker.tone(2000, 50); // Feedback
        }
        
        // Return to Dashboard and reset any local page states
        ui_goto_page(UI_PAGE_DASHBOARD);
    }
}