#include "screens/ui_passenger.h"
#include "driver_logic.h"
#include "billing_logic.h" // For access to tags[] and billing functions

// Local state tracking
static int active_modal_id = -1; // -1 means no modal is open
static bool is_modal_showing = false;

void ui_passenger_init(void) {
    M5.Display.startWrite();
    M5.Display.setTextSize(1);              // Force size back to small
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.fillScreen(TFT_BLACK);
    ui_create_header(); // From ui_manager

    // 1. BACK Button (Top Left)
    M5.Display.fillRoundRect(5, 40, 50, 35, 4, TFT_DARKGREY);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.drawCenterString("<", 30, 50);

    // 2. NEW Button (Top Right)
    M5.Display.fillRoundRect(265, 40, 50, 35, 4, TFT_GREEN);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.drawCenterString("+ NEW", 290, 50);

    // 3. List Container Area
    M5.Display.drawRect(5, 80, 310, 155, 0x4444); // Border for the list area
    M5.Display.endWrite();

    ui_refresh_passenger_list();
}

void ui_refresh_passenger_list(void) {
    if (is_modal_showing) return; // Don't redraw list under modal

    M5.Display.startWrite();
    M5.Display.fillRect(6, 81, 308, 153, 0x1A1A); // Clear list area

    int y_pos = 85;
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    
    for (int i = 0; i < 10; i++) {
        if (tags[i].isActive) {
            // Draw list item "button"
            M5.Display.fillRoundRect(10, y_pos, 300, 30, 4, 0x3333);
            M5.Display.setTextColor(TFT_WHITE);
            
            char buf[32];
            snprintf(buf, sizeof(buf), "Tag %d: N%.2f", i + 1, tags[i].currentFare);
            M5.Display.drawString(buf, 20, y_pos + 7);
            
            y_pos += 35; // Move to next slot
        }
    }
    M5.Display.endWrite();
}

void ui_show_passenger_modal(int tag_id) {
    is_modal_showing = true;
    active_modal_id = tag_id;

    M5.Display.startWrite();
    M5.Display.setTextSize(1);              // Force size back to small
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    
   // This color (0x1082) is a very dark grey that looks like a dimmed black screen.
    M5.Display.fillRect(0, 0, 320, 240, M5.Display.color565(20, 20, 20));

    // 2. Modal Box
    M5.Display.fillRoundRect(30, 40, 260, 160, 10, 0x2222);
    M5.Display.drawRoundRect(30, 40, 260, 160, 10, TFT_WHITE);

    // 3. Content
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.drawCenterString("PASSENGER #" + String(tag_id + 1), 160, 50);
    
    M5.Display.setFont(&fonts::FreeSans12pt7b);
    M5.Display.drawCenterString("Current: N" + String(tags[tag_id].currentFare, 2), 160, 90);

    // 4. END TRIP Button (Red)
    M5.Display.fillRoundRect(40, 140, 110, 45, 5, TFT_RED);
    M5.Display.drawCenterString("END", 95, 153);

    // 5. BACK Button (Grey)
    M5.Display.fillRoundRect(170, 140, 110, 45, 5, TFT_DARKGREY);
    M5.Display.drawCenterString("BACK", 225, 153);
    
    M5.Display.endWrite();
}

void ui_passenger_handle_touch(m5::touch_detail_t &t) {
    if (!t.wasPressed()) return;

    // 1. Handle Modal Input (The Pop-up details)
    if (is_modal_showing) {
        // END TRIP Button Clicked (x:40-150, y:140-185)
        if (t.x > 40 && t.x < 150 && t.y > 140 && t.y < 185) {
            if (isSoundEnabled) M5.Speaker.tone(1500, 50);
            calculate_final_fare(active_modal_id);
            is_modal_showing = false;
            ui_passenger_init(); // Redraws the list to reset text/UI
        }
        // CLOSE/BACK Button Clicked (x:170-280, y:140-185)
        else if (t.x > 170 && t.x < 280 && t.y > 140 && t.y < 185) {
            if (isSoundEnabled) M5.Speaker.tone(800, 50);
            is_modal_showing = false;
            ui_passenger_init(); // Cleanly redraws the list
        }
        return; // Don't allow background clicks while modal is open
    }

    // 2. Handle Top Bar Buttons
    if (t.y > 40 && t.y < 75) {
        // BACK to Dashboard (Top Left)
        if (t.x < 60) {
            if (isSoundEnabled) M5.Speaker.tone(1000, 50);
            ui_goto_page(UI_PAGE_DASHBOARD); // Centralized transition
            return;
        }
        
        // NEW Passenger (Top Right)
        if (t.x > 260) { 
            if (isSoundEnabled) M5.Speaker.tone(2000, 50);
            int slot = -1;
            // Searching tags array
            for(int i=0; i<10; i++) { 
                if(!tags[i].isActive) { slot = i; break; } 
            }
            if (slot != -1) {
                billing_start_trip(slot);
                ui_refresh_passenger_list();
            }
        }
    }

    // 3. Handle List Item Clicks (Below top bar)
    if (t.y > 80) {
        int clicked_idx = (t.y - 85) / 35;
        int count = 0;
        // Search for which active passenger tag was clicked
        for (int i = 0; i < 10; i++) {
            if (tags[i].isActive) {
                if (count == clicked_idx) {
                    if (isSoundEnabled) M5.Speaker.tone(1200, 30);
                    ui_show_passenger_modal(i);
                    break;
                }
                count++;
            }
        }
    }
}