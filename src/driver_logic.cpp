#include "driver_logic.h"
#include <Preferences.h>

bool screen_sleeping = false;
bool isSoundEnabled = true;
bool isPublicMode = false;
int batteryLevel = 100;
int last_brightness = 128;

Preferences preferences;

void driver_logic_init(void) {

    M5.Display.setBrightness(last_brightness);
    
    if (isSoundEnabled) {
        M5.Speaker.setVolume(128);
        M5.Speaker.tone(2000, 100);
    }
}

void driver_handle_hardware(void) {
    // update() is essential for processing BtnPWR and Power state changes
    M5.update();

    // USB Connection Detection using the unified Power API
    static bool was_charging = false;
    bool is_charging = M5.Power.isCharging();

    if (is_charging && !was_charging) {
        if (isSoundEnabled) M5.Speaker.tone(2000, 100);
        Serial.println("USB Connected");
    } else if (!is_charging && was_charging) {
        if (isSoundEnabled) M5.Speaker.tone(1000, 100);
        Serial.println("USB Removed");
    }
    was_charging = is_charging;

    // Handle Power Button (M5Core2 side button)
    if (M5.BtnPWR.wasHold()) {
        driver_system_shutdown();
    }

    if (M5.BtnPWR.wasClicked()) {
        if (!screen_sleeping) {
            M5.Display.setBrightness(0);
            screen_sleeping = true;
            if (isSoundEnabled) M5.Speaker.tone(1000, 50);
        } else {
            M5.Display.setBrightness(last_brightness);
            screen_sleeping = false;
            if (isSoundEnabled) M5.Speaker.tone(2000, 50);
        }
    }

    // Standardized battery level retrieval
    batteryLevel = M5.Power.getBatteryLevel();
}

void driver_set_brightness(int level) {
    last_brightness = constrain(level, 0, 255);
    if (!screen_sleeping) {
        M5.Display.setBrightness(last_brightness);
    }
}

void driver_system_restart(void) {
    if (isSoundEnabled) {
        M5.Speaker.tone(1500, 100);
        delay(150); 
    }
    ESP.restart();
}

void driver_system_shutdown(void) {
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.drawCenterString("Powering Off", 160, 110);
    if (isSoundEnabled) M5.Speaker.tone(800, 500);
    delay(1000);
    M5.Power.powerOff();
}

void driver_factory_reset(void) {
    // Use Preferences to clear the specific taxi namespace
    preferences.begin("taxi_billing", false);
    preferences.clear();
    preferences.end();

    M5.Display.fillScreen(TFT_RED);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.drawCenterString("FACTORY RESET", 160, 100);
    
    if (isSoundEnabled) {
        M5.Speaker.tone(2000, 200);
        delay(250);
        M5.Speaker.tone(1000, 500);
    }
    
    delay(2000);
    driver_system_restart();
}