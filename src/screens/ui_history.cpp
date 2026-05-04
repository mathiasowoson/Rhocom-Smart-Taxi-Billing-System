#include "screens/ui_history.h"
#include "driver_logic.h"

void ui_history_init(void) {
    M5.Display.startWrite();
    M5.Display.setTextSize(1);              // Force size back to small
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.fillScreen(TFT_BLACK);

    // 1. Draw Global Status Bar
    ui_create_header(); 

    // 2. Draw Back Button
    M5.Display.fillRoundRect(5, 40, 65, 35, 4, 0x3186); // Dark Grey (v565)
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setTextSize(1);
    M5.Display.drawString("< BACK", 12, 50);

    // 3. Draw Page Title
    M5.Display.setTextSize(1);
    M5.Display.drawCenterString("TRIP HISTORY", 160, 48);
    
    // Draw a separator line
    M5.Display.drawFastHLine(20, 75, 280, TFT_DARKGREY);

    // 4. Placeholder for Trip Data
    // For now, we display the "No trips" message. 
    // Later, you can use a loop here to list tags[i] or NVS data.
    M5.Display.setTextColor(TFT_SILVER);
    M5.Display.setTextSize(1);
    M5.Display.drawCenterString("No recent trips found.", 160, 130);

    M5.Display.endWrite();
}

void ui_history_handle_touch(m5::touch_detail_t &t) {
    // 1. Only process the initial press
    if (!t.wasPressed()) return;

    // 2. Back Button Widget (Top Left)
    // Coords: x:5-70, y:40-75
    if (t.y > 40 && t.y < 75 && t.x > 5 && t.x < 70) {
        
        // Feedback logic based on your preferences
        if (isSoundEnabled) {
            M5.Speaker.tone(2000, 50);
        }
        
        // Use the centralized navigation to clear the screen 
        // and reset the text size/font automatically.
        ui_goto_page(UI_PAGE_DASHBOARD);
    }
    
    // Add any future History-specific widgets (like scrolling) here
}