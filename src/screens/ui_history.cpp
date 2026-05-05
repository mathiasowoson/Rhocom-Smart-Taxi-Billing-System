#include "screens/ui_history.h"
#include "driver_logic.h"
#include "ui_manager.h"
#include "billing_logic.h"

void ui_history_init(void) {
    M5.Display.startWrite();
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextSize(1);

    ui_create_header(); // Global Status Bar

    // 1. Static Navigation & Title (Fixed positions)
    M5.Display.fillRoundRect(5, 40, 65, 35, 4, 0x3186); // Back Button
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.drawString("<", 12, 50);
    M5.Display.drawCenterString("TRIP HISTORY", 160, 48);
    M5.Display.drawFastHLine(20, 75, 280, TFT_DARKGREY);

    // 2. Dynamic Trip List (Scrollable)
    bool hasActiveTrips = false;
    int itemCount = 0;

    for (int i = 0; i < 10; i++) {
        if (tags[i].isActive) {
            hasActiveTrips = true;
            // Calculate virtual Y based on index and scroll offset
            int virtual_y = 85 + (itemCount * 38);
            
            // Use the scrollable widget helper from ui_manager
            char buf[32];
            snprintf(buf, sizeof(buf), "Tag %d: N%.2f", i + 1, tags[i].currentFare);
            draw_scrollable_button(10, virtual_y, 260, 32, buf, 0x3333);
            
            itemCount++;
        }
    }

    // 3. Fallback if no trips are active
    if (!hasActiveTrips) {
        M5.Display.setTextColor(TFT_SILVER);
        M5.Display.drawCenterString("No recent trips found.", 160, 130);
    } else {
        // 4. Static Scroll Controls (Only show if there are active trips)
        M5.Display.fillRoundRect(280, 85, 35, 60, 4, TFT_BLUE);  // UP Arrow
        M5.Display.setTextSize(1);
        M5.Display.setFont(&fonts::FreeSans12pt7b);
        M5.Display.drawCenterString("^", 297, 105);

        M5.Display.fillRoundRect(280, 160, 35, 60, 4, TFT_BLUE); // DOWN Arrow
        M5.Display.drawCenterString("v", 297, 180);
    }

    M5.Display.endWrite();
}

void ui_history_handle_touch(m5::touch_detail_t &t) {

    // --- SCROLL BUTTON DETECTION ---
// x > 275 is the blue button area on the right
if (t.x > 270) {
    if (t.y > 85 && t.y < 145) { // Up Button
        ui_scroll_offset -= 40;
        if (ui_scroll_offset < 0) ui_scroll_offset = 0;
        ui_history_init(); // Redraw to move content
        return;
    } 
    else if (t.y > 160 && t.y < 220) { // Down Button
        ui_scroll_offset += 40;
        ui_history_init(); // Redraw to move content
        return;
    }
}

    if (!t.wasPressed()) return; //

    // --- ZONE 1: STATIC BACK BUTTON (Priority) ---
    if (t.x > 5 && t.x < 70 && t.y > 40 && t.y < 75) {
        if (isSoundEnabled) {
            M5.Speaker.tone(2000, 50); //
        }
        ui_goto_page(UI_PAGE_DASHBOARD); // Resets scroll and clears screen
        return;
    }

    // --- ZONE 2: SCROLL CONTROLS (Right Side) ---
    if (t.x > 275) {
        if (t.y > 85 && t.y < 145) { // Scroll Up
            ui_scroll_offset -= 38;
            if (ui_scroll_offset < 0) ui_scroll_offset = 0;
            ui_history_init(); // Redraw with new position
        } 
        else if (t.y > 160 && t.y < 220) { // Scroll Down
            ui_scroll_offset += 38;
            ui_history_init();
        }
        return;
    }

    // --- ZONE 3: TRIP ITEM INTERACTION ---
    // Mapping touch to virtual Y coordinates for scrollable content
    int virtual_y = t.y + ui_scroll_offset;

    if (t.x > 10 && t.x < 270 && virtual_y > 85) {
        int clicked_idx = (virtual_y - 85) / 38;
        
        // This index refers to the visible list; 
        // Logic can be added here later to open trip details
        if (isSoundEnabled) M5.Speaker.tone(1500, 30);
        Serial.printf("Clicked trip at list position: %d\n", clicked_idx);
    }
}