#include "config.h"
#include "ui_manager.h"
#include "billing_logic.h"
#include "blynk_logic.h"
#include "config.h"



// Global Data Init
PassengerTag tags[10];
NetMode currentNetMode = MODE_LTE; // Default to LTE on startup
int activeCount = 0;
bool isPublicMode = true;

// LVGL Buffer
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[320 * 24];

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    
    
    // 1. LVGL Display Setup
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

    // 2. Touch Driver Setup
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

    // 3. Start System UI
    ui_init(); // Initialize the Rhocom UI Manager
}

void loop() {
    M5.update();
    lv_timer_handler(); // Refresh screen
    blynk_update();      // Process Cloud signals
    billing_update_all(); // Update the fares in background
    delay(5);
}