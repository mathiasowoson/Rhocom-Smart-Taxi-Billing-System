#include "config.h"
#include "driver_logic.h"

bool screen_sleeping = false;
bool isSoundEnabled = true;
int batteryLevel = 100;
int last_brightness = 128;

void driver_logic_init(void) {
    M5.Display.setBrightness(last_brightness);
}

void driver_set_brightness(int level) {
    if (!screen_sleeping) {
        last_brightness = level;
        M5.Display.setBrightness(level);
    }
}

void driver_toggle_sound(bool on) {
    isSoundEnabled = on;
    M5.Speaker.setVolume(on ? 128 : 0);
}

void driver_system_shutdown(void) {
    M5.Display.setBrightness(128);
    M5.Display.clear();
    M5.Display.setCursor(60, 110);
    M5.Display.setTextSize(2);
    M5.Display.print("Shutting Down...");
    delay(1000);
    M5.Power.powerOff();
}

void driver_system_restart(void) {
    ESP.restart();
}

void driver_factory_reset(void) {
    // Logic to clear NVS/Preferences would go here
    Serial.println("Factory Reset Triggered...");
    driver_system_restart();
}

void driver_handle_power_button(void) {
    M5.update(); 

    // LONG PRESS: Power Off
    if (M5.BtnPWR.wasHold()) {
        driver_system_shutdown();
    }

    // SHORT PRESS: Sleep/Wake
    if (M5.BtnPWR.wasClicked()) {
        if (!screen_sleeping) {
            M5.Display.setBrightness(0);
            screen_sleeping = true;
        } else {
            M5.Display.setBrightness(last_brightness);
            screen_sleeping = false;
        }
    }
}