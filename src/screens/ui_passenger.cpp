#include "screens/ui_passenger.h"
#include "driver_logic.h"
#include "billing_logic.h" // For access to tags[] and billing functions
#include "ui_manager.h"

// Local state tracking
static int active_modal_id = -1; // -1 means no modal is open
static bool is_modal_showing = false;

void ui_passenger_init(void) {
    is_modal_showing = false; // Reset modal state on page entry
    M5.Display.startWrite();
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextSize(1);
    ui_create_header(); 

    // 1. Static Navigation (Top Bar)
    // BACK Button (Top Left)
    M5.Display.fillRoundRect(5, 40, 50, 35, 4, TFT_DARKGREY);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setTextSize(1);
    M5.Display.setFont(&fonts::FreeSans12pt7b);
    M5.Display.drawCenterString("<", 30, 50);

    // NEW Button (Top Right)
    M5.Display.fillRoundRect(265, 40, 50, 35, 4, TFT_GREEN);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.setTextSize(1);
    M5.Display.setFont(&fonts::FreeSans12pt7b);
    M5.Display.drawCenterString("+", 290, 50);

    // 2. Scroll Controls (Static Right Side)
    M5.Display.fillRoundRect(280, 85, 35, 60, 4, TFT_BLUE);
    M5.Display.setTextSize(1);
    M5.Display.setFont(&fonts::FreeSans12pt7b); 
    M5.Display.drawCenterString("^", 297, 105);

    M5.Display.fillRoundRect(280, 160, 35, 60, 4, TFT_BLUE);
    M5.Display.setTextSize(1);
    M5.Display.setFont(&fonts::FreeSans12pt7b);
    M5.Display.drawCenterString("v", 297, 180);

    M5.Display.endWrite();
    ui_refresh_passenger_list();
}

void ui_refresh_passenger_list(void) {
    if (is_modal_showing) return; 

    M5.Display.startWrite();
    // 1. Reset Font to Default (prevents "Big Text" bug)
    M5.Display.setFont(&fonts::FreeSans9pt7b); 
    M5.Display.setTextSize(1); 

    // 2. Clear the scrollable area
    M5.Display.fillRect(5, 80, 270, 155, TFT_BLACK); 

    int active_count = 0;
    for (int i = 0; i < 10; i++) {
        if (tags[i].isActive) {
            // FIX: Subtract ui_scroll_offset to actually move the items up/down
            int draw_y = 85 + (active_count * 40) - ui_scroll_offset;
            
            char buf[32];
            snprintf(buf, sizeof(buf), "Tag %d: N%.2f", i + 1, tags[i].currentFare);
            
            // Only draw if it's within the visible window (y: 80 to 240)
            if (draw_y >= 80 && draw_y < 230) {
                draw_scrollable_button(10, draw_y, 260, 35, buf, 0x3333);
            }
            active_count++;
        }
    }
    M5.Display.endWrite();
}

void ui_show_passenger_modal(int tag_id) {
    is_modal_showing = true;
    active_modal_id = tag_id;

    M5.Display.startWrite();
    // Dim the background
    M5.Display.fillRect(0, 0, 320, 240, M5.Display.color565(20, 20, 20));

    // Modal Box
    M5.Display.fillRoundRect(30, 40, 260, 160, 10, 0x2222);
    M5.Display.drawRoundRect(30, 40, 260, 160, 10, TFT_WHITE);

    M5.Display.setFont(&fonts::FreeSans12pt7b);
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.drawCenterString("PASSENGER #" + String(tag_id + 1), 160, 55);
    M5.Display.drawCenterString("Fare: N" + String(tags[tag_id].currentFare, 2), 160, 95);

    // END TRIP Button (Red) - Hitbox: x:40-150, y:140-185
    M5.Display.fillRoundRect(40, 140, 110, 45, 5, TFT_RED);
    M5.Display.drawCenterString("END", 95, 153);

    // BACK Button (Grey) - Hitbox: x:170-280, y:140-185
    M5.Display.fillRoundRect(170, 140, 110, 45, 5, TFT_DARKGREY);
    M5.Display.drawCenterString("BACK", 225, 153);
    
    M5.Display.endWrite();
}

void ui_passenger_handle_touch(m5::touch_detail_t &t) {
    if (!t.wasPressed()) return;

    // --- LAYER 1: MODAL INPUT (HIGHEST PRIORITY) ---
    if (is_modal_showing) {
        // Check Y-range for both buttons first
        if (t.y >= 140 && t.y <= 185) {
            // END Button
            if (t.x >= 40 && t.x <= 150) {
                if (isSoundEnabled) M5.Speaker.tone(1500, 50);
                calculate_final_fare(active_modal_id); //
                is_modal_showing = false;
                ui_passenger_init(); // Full refresh
            }
            // BACK Button
            else if (t.x >= 170 && t.x <= 280) {
                if (isSoundEnabled) M5.Speaker.tone(800, 50);
                is_modal_showing = false;
                ui_passenger_init(); // Full refresh
            }
        }
        return; // CRITICAL: Stop here so background clicks don't trigger
    }

    // --- LAYER 2: TOP BAR (BACK & NEW) ---
    if (t.y >= 40 && t.y <= 75) {
        if (t.x <= 60) { // BACK to Dash
            ui_goto_page(UI_PAGE_DASHBOARD);
            return;
        }
        if (t.x >= 260) { // NEW Passenger
            for(int i=0; i<10; i++) { 
                if(!tags[i].isActive) { 
                    billing_start_trip(i); //
                    ui_refresh_passenger_list();
                    break; 
                } 
            }
            return;
        }
    }

    // --- LAYER 3: SCROLL CONTROLS ---
    if (t.x > 270) {
        if (t.y > 80 && t.y < 150) { // Up
            ui_scroll_offset -= 40;
            if (ui_scroll_offset < 0) ui_scroll_offset = 0;
            ui_refresh_passenger_list();
        } 
        else if (t.y > 155 && t.y < 230) { // Down
            ui_scroll_offset += 40;
            ui_refresh_passenger_list();
        }
        return;
    }

    // --- LAYER 4: LIST ITEMS ---
    if (t.x < 270 && t.y > 80) {
        int clicked_y = t.y + ui_scroll_offset;
        int clicked_slot = (clicked_y - 85) / 40;

        int found_idx = 0;
        for (int i = 0; i < 10; i++) {
            if (tags[i].isActive) {
                if (found_idx == clicked_slot) {
                    ui_show_passenger_modal(i);
                    return;
                }
                found_idx++;
            }
        }
    }
}