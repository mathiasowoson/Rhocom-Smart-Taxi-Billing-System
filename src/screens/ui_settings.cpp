#include "screens/ui_settings.h"
#include "driver_logic.h"
#include "ui_manager.h"

static bool is_reset_modal_active = false;

void ui_settings_init(void) {
    is_reset_modal_active = false;
    M5.Display.startWrite();
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextSize(1);
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    ui_create_header();

    // 1. Static Back Button (Fixed)
    M5.Display.fillRoundRect(5, 40, 50, 35, 4, TFT_DARKGREY);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setTextSize(1);
    M5.Display.setFont(&fonts::FreeSans12pt7b);
    M5.Display.drawCenterString("<", 30, 50);

    // 2. Settings Title
    M5.Display.drawCenterString("SETTINGS", 160, 48);
    M5.Display.drawFastHLine(20, 75, 280, 0x4444);

    // --- SCROLLABLE CONTENT START ---
    // We apply ui_scroll_offset to all Y coordinates below
    int base_y = 85 - ui_scroll_offset;

    // 3. Brightness Section
    M5.Display.drawString("Brightness", 20, base_y);
    M5.Display.drawRect(20, base_y + 20, 280, 25, TFT_WHITE); // Larger hit box
    int barWidth = map(last_brightness, 10, 255, 0, 276);
    M5.Display.fillRect(22, base_y + 22, barWidth, 21, TFT_ORANGE);

    // 4. Mode Toggle
    M5.Display.drawString("Public Mode", 20, base_y + 65);
    uint32_t toggleCol = isPublicMode ? TFT_GREEN : 0x5555;
    M5.Display.fillRoundRect(240, base_y + 60, 60, 30, 15, toggleCol);
    M5.Display.fillCircle(isPublicMode ? 285 : 255, base_y + 75, 12, TFT_WHITE);

    // 5. Action Buttons (Restart/Shutdown)
    M5.Display.fillRoundRect(20, base_y + 110, 135, 45, 4, 0x3333);
    M5.Display.setTextSize(1);
    M5.Display.setFont(&fonts::FreeSans12pt7b);
    M5.Display.drawCenterString("Restart", 87, base_y + 125);

    M5.Display.fillRoundRect(165, base_y + 110, 135, 45, 4, 0x8000); // Maroon/Red
    M5.Display.setTextSize(1);
    M5.Display.setFont(&fonts::FreeSans12pt7b);
    M5.Display.drawCenterString("Shutdown", 232, base_y + 125);

    // 6. Factory Reset (Danger Zone)
    M5.Display.setTextColor(TFT_RED);
    M5.Display.setTextSize(1);
    M5.Display.setFont(&fonts::FreeSans12pt7b);
    M5.Display.drawCenterString("FACTORY RESET", 160, base_y + 175);

    M5.Display.endWrite();
}

void ui_show_factory_reset_warning(void) {
    is_reset_modal_active = true;
    M5.Display.startWrite();
    // Dim background using the solid fill method we established
    M5.Display.fillRect(0, 0, 320, 240, M5.Display.color565(20, 20, 20));
    
    M5.Display.fillRoundRect(30, 60, 260, 120, 8, TFT_RED);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setTextSize(1);              // Force size back to small
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.drawCenterString("FACTORY RESET?", 160, 80);
    M5.Display.drawCenterString("Wipe all revenue data?", 160, 105);

    // Modal Buttons
    M5.Display.fillRoundRect(50, 135, 90, 35, 4, TFT_BLACK);
    M5.Display.setTextSize(1);
    M5.Display.setFont(&fonts::FreeSans12pt7b);
    M5.Display.drawCenterString("YES", 95, 145);
    
    M5.Display.fillRoundRect(180, 135, 90, 35, 4, TFT_DARKGREY);
    M5.Display.setTextSize(1);
    M5.Display.setFont(&fonts::FreeSans12pt7b);
    M5.Display.drawCenterString("NO", 225, 145);
    M5.Display.endWrite();
}

void ui_settings_handle_touch(m5::touch_detail_t &t) {

    // --- SCROLL BUTTON DETECTION ---
// x > 275 is the blue button area on the right
if (t.x > 270) {
    if (t.y > 85 && t.y < 145) { // Up Button
        ui_scroll_offset -= 40;
        if (ui_scroll_offset < 0) ui_scroll_offset = 0;
        ui_settings_init(); // Redraw to move content
        return;
    } 
    else if (t.y > 160 && t.y < 220) { // Down Button
        ui_scroll_offset += 40;
        ui_settings_init(); // Redraw to move content
        return;
    }
}

    if (!t.wasPressed()) return;

    // --- LAYER 1: MODAL (Safety Lock) ---
    if (is_reset_modal_active) {
        if (t.y > 135 && t.y < 170) {
            if (t.x > 50 && t.x < 140) { // YES
                if (isSoundEnabled) M5.Speaker.tone(1000, 100);
                driver_factory_reset(); 
            }
            if (t.x > 180 && t.x < 270) { // NO
                is_reset_modal_active = false;
                ui_settings_init(); 
            }
        }
        return;
    }

    // --- LAYER 2: STATIC BACK BUTTON ---
    if (t.y > 35 && t.y < 80 && t.x < 70) {
        if (isSoundEnabled) M5.Speaker.tone(1000, 50);
        ui_goto_page(UI_PAGE_DASHBOARD);
        return;
    }

    // --- LAYER 3: VIRTUAL Y MAPPING ---
    int virtual_y = t.y + ui_scroll_offset;

    // 3. Brightness Slider (Increased touch height for better response)
    if (virtual_y > 100 && virtual_y < 135 && t.x > 15 && t.x < 305) {
        int newBr = map(t.x, 20, 300, 10, 255);
        newBr = constrain(newBr, 10, 255);
        last_brightness = newBr; // Update variable
        driver_set_brightness(newBr);
        ui_settings_init(); 
        return;
    }

    // 4. Public Mode Toggle
    if (virtual_y > 140 && virtual_y < 180 && t.x > 220) {
        isPublicMode = !isPublicMode;
        if (isSoundEnabled) M5.Speaker.tone(1500, 30);
        ui_settings_init();
        return;
    }

    // 5. Restart / Shutdown
    if (virtual_y > 190 && virtual_y < 240) {
        if (t.x > 15 && t.x < 155) { // Restart
            if (isSoundEnabled) M5.Speaker.tone(1200, 50);
            driver_system_restart();
        }
        if (t.x > 160 && t.x < 305) { // Shutdown
            if (isSoundEnabled) M5.Speaker.tone(800, 50);
            driver_system_shutdown();
        }
        return;
    }

    // 6. Factory Reset Trigger
    if (virtual_y > 250 && t.x > 80 && t.x < 240) {
        ui_show_factory_reset_warning();
    }
}