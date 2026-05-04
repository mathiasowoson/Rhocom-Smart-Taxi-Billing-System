#include <M5Unified.h>
#include <M5GFX.h>
// #include <utility/fonts/FreeSans9pt7b.h>
// #include <Fonts/GFXFF/FreeSans9pt7b.h>
#include "ui_manager.h"
#include "config.h"
#include "soc/rtc_cntl_reg.h"
#include <esp_task_wdt.h>  
#include "billing_logic.h"
#include "cloudGsm_logic.h" 
#include "driver_logic.h"

SemaphoreHandle_t xSerialSemaphore = NULL;

// Global Data
PassengerTag tags[10];     

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    Serial.begin(115200);

    xSerialSemaphore = xSemaphoreCreateMutex();

    esp_task_wdt_init(120, true); 
    esp_task_wdt_add(NULL);

    // M5.Display.setFont(&FreeSans9pt7b); // Set a professional looking font
    M5.Display.setTextSize(1); // Now 1 will look

    // 1. Start Logic Systems
    driver_logic_init(); 
    
    // Using your existing function name
    billing_init(); 
    
    cloud_gsm_setup(); 

    // 2. Start UI
    // Using your existing function name
    ui_init(); 
    
    Serial.println("System Ready.");
}

void loop() {
    esp_task_wdt_reset();
    M5.update();
    
    // 3. UI Touch Handling
    m5::touch_detail_t touch_data = M5.Touch.getDetail();

    ui_manager_handle_touch(touch_data);
    
    // 4. System Logic Updates
    // Using your existing function name
    billing_update_all(); 
    
    driver_handle_hardware(); 

    if (M5.Touch.getCount() > 0) {
    auto detail = M5.Touch.getDetail();
    if (detail.wasPressed()) {
        Serial.printf("Touch Detected at X: %d, Y: %d\n", detail.x, detail.y);
    }
}
    
    vTaskDelay(5); 
}