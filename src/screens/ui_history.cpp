#include "screens/ui_history.h"
#include "driver_logic.h"
#include "ui_manager.h"
#include "billing_logic.h"

// Keep track of which trip modal is open. -1 means no modal is showing.
int activeModalIdx = -1; 

// --- THE MODAL DRAWING FUNCTION ---
void draw_history_modal(int idx) {
    TripHistory &t = tripHistory[idx]; // Use the data from billing_logic
    
    // 1. Draw Modal Box (Dark Navy with White Border)
    M5.Display.fillRoundRect(20, 50, 280, 180, 8, 0x1082); 
    M5.Display.drawRoundRect(20, 50, 280, 180, 8, TFT_WHITE);

    M5.Display.setTextColor(TFT_YELLOW);
    M5.Display.drawCenterString("TRIP DETAILS", 160, 60);
    
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setTextSize(1);
    
    int startY = 85;
    M5.Display.setCursor(30, startY);
    M5.Display.printf("Tag ID: %d", t.tagId);
    M5.Display.setCursor(30, startY + 20);
    M5.Display.printf("Start: %.4f, %.4f", t.startLat, t.startLon);
    M5.Display.setCursor(30, startY + 40);
    M5.Display.printf("End: %.4f, %.4f", t.endLat, t.endLon);
    M5.Display.setCursor(30, startY + 60);
    M5.Display.printf("Fare: N%.2f", t.fare);
    M5.Display.setCursor(30, startY + 80);
    M5.Display.printf("Time: %lum %lus", t.duration / 60, t.duration % 60);
    
    // 2. Sync Status Text
    M5.Display.setCursor(30, startY + 110);
    if(t.isSynced) {
        M5.Display.setTextColor(TFT_GREEN);
        M5.Display.print("STATUS: SYNCED");
    } else {
        M5.Display.setTextColor(TFT_RED);
        M5.Display.print("STATUS: PENDING SYNC");
    }

    // 3. Close Button
    M5.Display.fillRoundRect(110, 200, 100, 25, 4, TFT_RED);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.drawCenterString("CLOSE", 160, 205);
}

// --- MAIN SCREEN INIT ---
void ui_history_init(void) {
    M5.Display.startWrite();
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextSize(1);

    ui_create_header(); // Global Status Bar

    // 1. Static Navigation & Title
    M5.Display.fillRoundRect(5, 40, 65, 35, 4, 0x3186); // Back Button
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.drawString("<", 12, 50);
    M5.Display.drawCenterString("TRIP HISTORY", 160, 48);
    M5.Display.drawFastHLine(20, 75, 280, TFT_DARKGREY);

    // 2. Dynamic Trip List (Now using TripHistory array)
    bool hasHistory = false;
    int itemCount = 0;

    for (int i = 0; i < 10; i++) {
        // Only draw if the history slot actually contains a finished trip
        if (tripHistory[i].isValid) { 
            hasHistory = true;
            int virtual_y = 85 + (itemCount * 38) - ui_scroll_offset;
            
            // Only draw if within visible bounds
            if (virtual_y >= 80 && virtual_y < 220) {
                char buf[32];
                snprintf(buf, sizeof(buf), "Trip %d: N%.2f", tripHistory[i].tagId, tripHistory[i].fare);
                draw_scrollable_button(10, virtual_y, 260, 32, buf, 0x3333);
            }
            itemCount++;
        }
    }

    if (!hasHistory) {
        M5.Display.setTextColor(TFT_SILVER);
        M5.Display.drawCenterString("No history recorded today.", 160, 130);
    } else {
        // 3. Scroll Controls
        M5.Display.fillRoundRect(280, 85, 35, 60, 4, TFT_BLUE);  // UP
        M5.Display.drawCenterString("^", 297, 105);
        M5.Display.fillRoundRect(280, 160, 35, 60, 4, TFT_BLUE); // DOWN
        M5.Display.drawCenterString("v", 297, 180);
    }

    // 4. OVERLAY: If a modal is supposed to be open, draw it last
    if (activeModalIdx != -1) {
        draw_history_modal(activeModalIdx);
    }

    M5.Display.endWrite();
}

// --- TOUCH HANDLING ---
void ui_history_handle_touch(m5::touch_detail_t &t) {
    if (!t.wasPressed()) return;

    // A. IF MODAL IS OPEN: Only listen for the "CLOSE" button
    if (activeModalIdx != -1) {
        if (t.x > 110 && t.x < 210 && t.y > 200 && t.y < 230) {
            activeModalIdx = -1; // Close it
            ui_history_init();   // Redraw the list
        }
        return; // Don't let touches pass through to the list below
    }

    // B. BACK BUTTON
    if (t.x > 5 && t.x < 70 && t.y > 40 && t.y < 75) {
        ui_goto_page(UI_PAGE_DASHBOARD);
        return;
    }

    // C. SCROLL CONTROLS
    if (t.x > 275) {
        if (t.y > 85 && t.y < 145) ui_scroll_offset = max(0, ui_scroll_offset - 38);
        else if (t.y > 160 && t.y < 220) ui_scroll_offset += 38;
        ui_history_init();
        return;
    }

    // D. ITEM CLICK: Open the Modal
    int virtual_y = t.y + ui_scroll_offset;
    if (t.x > 10 && t.x < 270 && virtual_y > 85) {
        int clicked_idx = (virtual_y - 85) / 38;
        if (clicked_idx < 10 && tripHistory[clicked_idx].isValid) {
            if (isSoundEnabled) M5.Speaker.tone(1500, 30);
            activeModalIdx = clicked_idx;
            ui_history_init(); // Redraw will trigger the modal overlay
        }
    }
}