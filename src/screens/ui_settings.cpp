#include "screens/ui_settings.h"
#include "driver_logic.h"

static bool is_reset_modal_active = false;

void ui_settings_init(void) {
    is_reset_modal_active = false;
    M5.Display.startWrite();
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextSize(1);              // Force size back to small
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    ui_create_header(); // Global header

    // 1. BACK Button
    M5.Display.fillRoundRect(5, 40, 50, 35, 4, TFT_DARKGREY);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.drawCenterString("<", 30, 50);

    // 2. Brightness Section (Replacing LVGL Slider)
    M5.Display.drawString("Brightness", 20, 85);
    M5.Display.drawRect(20, 105, 280, 20, TFT_WHITE); // Slider track
    // Map current brightness (10-255) to bar width (0-278)
    int barWidth = map(last_brightness, 10, 255, 0, 276);
    M5.Display.fillRect(22, 107, barWidth, 16, TFT_ORANGE);

    // 3. Mode Toggle (Replacing LVGL Switch)
    M5.Display.drawString("Public Mode", 20, 140);
    uint32_t toggleCol = isPublicMode ? TFT_GREEN : TFT_DARKGREY;
    M5.Display.fillRoundRect(240, 135, 60, 25, 12, toggleCol);
    M5.Display.fillCircle(isPublicMode ? 285 : 255, 147, 10, TFT_WHITE);

    // 4. Action Buttons (Replacing LVGL List)
    // Restart System
    M5.Display.fillRoundRect(20, 175, 135, 40, 4, 0x5555); // Grey
    M5.Display.drawCenterString("Restart", 87, 188);

    // Shutdown
    M5.Display.fillRoundRect(165, 175, 135, 40, 4, TFT_RED);
    M5.Display.drawCenterString("Shutdown", 232, 188);

    // Factory Reset (Bottom)
    M5.Display.setTextColor(TFT_ORANGE);
    M5.Display.drawCenterString("FACTORY RESET", 160, 225);
    
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
    M5.Display.drawCenterString("YES", 95, 145);
    
    M5.Display.fillRoundRect(180, 135, 90, 35, 4, TFT_DARKGREY);
    M5.Display.drawCenterString("NO", 225, 145);
    M5.Display.endWrite();
}

void ui_settings_handle_touch(m5::touch_detail_t &t) {
    if (!t.wasPressed()) return;

    // 1. Handle Reset Modal (Highest Priority)
    if (is_reset_modal_active) {
        if (t.y > 135 && t.y < 170) {
            // YES Button
            if (t.x > 50 && t.x < 140) {
                if (isSoundEnabled) M5.Speaker.tone(1000, 100);
                driver_factory_reset(); 
            }
            // NO Button (Reload screen to close modal)
            if (t.x > 180 && t.x < 270) {
                is_reset_modal_active = false;
                ui_settings_init(); 
            }
        }
        return; // Lock interaction to the modal only
    }

    // 2. Back Button (Top Left)
    if (t.y > 40 && t.y < 75 && t.x < 60) {
        if (isSoundEnabled) M5.Speaker.tone(1000, 50);
        ui_goto_page(UI_PAGE_DASHBOARD); // Centralized transition
        return;
    }

    // 3. Brightness Slider Widget
    if (t.y > 95 && t.y < 130 && t.x > 20 && t.x < 300) {
        int newBr = map(t.x, 20, 300, 10, 255);
        driver_set_brightness(newBr);
        ui_settings_init(); // Redraw to update the visual slider bar
    }

    // 4. Public Mode Toggle Widget
    if (t.y > 135 && t.y < 165 && t.x > 230) {
        isPublicMode = !isPublicMode;
        if (isSoundEnabled) M5.Speaker.tone(1500, 30);
        ui_settings_init(); // Redraw to update toggle switch visual
    }

    // 5. Bottom Action Buttons (Restart/Shutdown)
    if (t.y > 175 && t.y < 215) {
        // Restart
        if (t.x > 20 && t.x < 155) {
            if (isSoundEnabled) M5.Speaker.tone(1200, 50);
            driver_system_restart();
        }
        // Shutdown
        if (t.x > 165 && t.x < 300) {
            if (isSoundEnabled) M5.Speaker.tone(800, 50);
            driver_system_shutdown();
        }
    }
    
    // 6. Factory Reset Warning Trigger
    if (t.y > 215 && t.x > 100 && t.x < 220) {
        ui_show_factory_reset_warning(); // Opens the reset modal
    }
}