#include <esp_task_wdt.h>  // Essential for handling GSM delays
#include "config.h"
#include "ui_manager.h"
#include "billing_logic.h"
#include "blynkGsm_logic.h"  // Unified GSM Logic
#include "driver_logic.h"

// Timer for syncing data (every 5 seconds)
unsigned long lastSyncTime = 0;
const unsigned long syncInterval = 5000;

// LVGL Buffer
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[320 * 24];

PassengerTag tags[10];     
bool isPublicMode = false;             

void setup() {
    // 1. Initialize M5Stack Core2 via M5Unified
    auto cfg = M5.config();
    M5.begin(cfg);
    Serial.begin(115200);

    esp_task_wdt_init(45, true); 
    esp_task_wdt_add(NULL); 
    esp_task_wdt_reset();

    Serial.println("System: Starting Smart Taxi (GSM Mode)...");

    // 2. LVGL Display Setup
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, 320 * 24);
    
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 240;
    disp_drv.flush_cb = [](lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
        M5.Display.pushImageDMA(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, (uint16_t*)color_p);
        lv_disp_flush_ready(disp);
    };
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // 3. Touch Driver Setup
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = [](lv_indev_drv_t* indev, lv_indev_data_t* data) {
        M5.update();
        auto touch = M5.Touch.getDetail();
        if (touch.isPressed()) {
            data->state = LV_INDEV_STATE_PR;
            data->point.x = touch.x;
            data->point.y = touch.y;
        } else {
            data->state = LV_INDEV_STATE_REL;
        }
    };
    lv_indev_drv_register(&indev_drv);

    // 6. Start System UI
    ui_init(); 
    Serial.println("System: Initialization Complete.");


    // 4. Initialize Hardware Components
    driver_logic_init();
    billing_init();      // Starts GPS tracking logic
    
    // 5. Start Blynk via SIM7600
    blynk_gsm_setup();
}

void loop() {
    esp_task_wdt_reset();
    M5.update();
    lv_timer_handler(); 
    
    // Process Blynk Cloud connection
    blynk_gsm_update(); 

    // Update GPS coordinates and calculate fares
    billing_update_all();

    // Automatic Data Sync every 5 seconds
    if (millis() - lastSyncTime >= syncInterval) {
        lastSyncTime = millis();
        blynk_gsm_sync();
    }

    // Handle physical hardware buttons (Power, etc)
    driver_handle_power_button();
    
    delay(5);
}