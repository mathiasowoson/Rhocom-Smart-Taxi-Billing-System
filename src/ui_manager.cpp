#include "ui_manager.h"
// Keeping your original includes
#include "screens/ui_dashboard.h"
#include "screens/ui_passenger.h"
#include "screens/ui_union.h"
#include "screens/ui_settings.h"
#include "screens/ui_history.h"
#include "screens/ui_reports.h"

static ui_page_t current_active_page = UI_PAGE_DASHBOARD;

// Internal state variables to track status colors/values
static uint32_t gps_color = 0x7BEF;   // LV_PALETTE_GREY equivalent
static uint32_t cloud_color = TFT_RED; 
static const char* batt_sym = "F";    // Battery Full symbol representation
static int current_batt_val = 100;

void ui_init(void) {
    // Exactly as your original: start at dashboard
    ui_goto_page(UI_PAGE_DASHBOARD);
}

void ui_create_header(void) {
    // 1. Draw Header Bar (320x35, Blue-Grey)
    // Blue-Grey color: 0x6477 (approximate for M5GFX)
    M5.Display.fillRect(0, 0, 320, 35, M5.Display.color565(96, 125, 139));
    
    // 2. Title: "TAXI PAY" (Aligned Left-Mid)
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setTextSize(1);
    M5.Display.drawString("RHOCOM", 5, 10);

    // 3. GPS Status (Aligned Center -20)
    M5.Display.setTextColor(gps_color);
    M5.Display.drawString("GPS", 130, 10);

    // 4. Cloud Status (Aligned Center +20)
    M5.Display.setTextColor(cloud_color);
    M5.Display.drawString("NET", 180, 10);

    // 5. Battery Status (Aligned Right-Mid)
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.drawRightString("BAT:" + String(current_batt_val) + "%", 315, 10);
}

void ui_goto_page(ui_page_t page) {
    // Clear screen for every page change as per original logic
    current_active_page = page; // Remember the new page
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextSize(1);              // Force reset size
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    ui_create_header();

    switch (page) {
        case UI_PAGE_DASHBOARD:
            ui_dashboard_init(); // Call your existing screen init functions
            break;

        case UI_PAGE_PASSENGER:
            ui_passenger_init(); 
            break;

        case UI_PAGE_UNION:
            ui_union_init();
            break;

        case UI_PAGE_SETTINGS:
            ui_settings_init();
            break;

        case UI_PAGE_REPORTS:
            ui_reports_init();
            break;

        case UI_PAGE_HISTORY:
            ui_history_init();
            break;

        case UI_PAGE_LOGOUT:
            M5_LOGI("System: Logging out and Restarting...");
            ESP.restart(); 
            break;
    }
    M5_LOGI("NAV: Switched to page %d", page);
}

void ui_update_status_bar(bool gps_fixed, bool cloud_conn, int batt) {
    // Map status to colors (Matching your LV_PALETTE logic)
    gps_color = gps_fixed ? TFT_GREEN : 0x7BEF; 
    cloud_color = cloud_conn ? TFT_BLUE : TFT_RED;
    current_batt_val = batt;
    
    // Redraw header with updated status
    ui_create_header();
}

void ui_back_to_dash_cb(void) {
    // Logic: Return to dash and log to Serial as per original
    ui_goto_page(UI_PAGE_DASHBOARD);
    Serial.print("NAV: Returning back to dashboard");
}

// You'll need to make sure the dashboard handle is declared in ui_dashboard.h
void ui_manager_handle_touch(m5::touch_detail_t &t) {
    if (!t.wasPressed()) return; // Only process actual clicks

    // 1. Check Global Header (Visible on ALL pages)
    if (t.y < 35) {
        Serial.println("Header Clicked");
        return;
    }

    // 2. Route the touch ONLY to the visible page
    // This prevents the Dashboard buttons from clicking while you're in Union/Passenger
    switch (current_active_page) {
        case UI_PAGE_DASHBOARD:
            ui_dashboard_handle_touch(t); 
            break;
        case UI_PAGE_PASSENGER:
            ui_passenger_handle_touch(t); 
            break;
        case UI_PAGE_UNION:
            ui_union_handle_touch(t); 
            break;
        case UI_PAGE_SETTINGS:
            ui_union_handle_touch(t); 
            break;
        case UI_PAGE_REPORTS:
            ui_union_handle_touch(t); 
            break;
        case UI_PAGE_HISTORY:
            ui_union_handle_touch(t); 
            break;
        // Add handlers for other pages as you build them
        default:
            break;
    }
}

// Add this to the bottom of your ui_manager.cpp

// void ui_manager_handle_touch(m5::touch_detail_t &t) {
//     // We only trigger when the user first lifts their finger (wasReleased)
//     // or initially touches (wasPressed) to prevent rapid "ghost" clicking.
//     if (t.wasPressed()) {
//         int x = t.x;
//         int y = t.y;

//         // --- HEADER CLICKS ---
//         // If user touches the header bar (y < 35), refresh the status
//         if (y < 35) {
//             ui_create_header();
//             return;
//         }

//         // --- DASHBOARD BUTTONS ---
//         // These coordinates assume 2 columns and 3 rows of buttons
        
//         // Row 1: Passenger (Left) and Union (Right)
//         if (y > 40 && y < 100) {
//             if (x > 10 && x < 150) ui_goto_page(UI_PAGE_PASSENGER);
//             if (x > 170 && x < 310) ui_goto_page(UI_PAGE_UNION);
//         }
        
//         // Row 2: Reports (Left) and History (Right)
//         else if (y > 110 && y < 170) {
//             if (x > 10 && x < 150) ui_goto_page(UI_PAGE_REPORTS);
//             if (x > 170 && x < 310) ui_goto_page(UI_PAGE_SETTINGS);
//         }

//         // Row 3: Settings (Left) and Logout (Right)
//         else if (y > 180 && y < 240) {
//             if (x > 10 && x < 150) ui_goto_page(UI_PAGE_HISTORY);
//             if (x > 170 && x < 310) ui_goto_page(UI_PAGE_LOGOUT);
//         }
//     }
// }