#ifndef UI_DASHBOARD_H
#define UI_DASHBOARD_H

#include <lvgl.h>
#include "config.h"
// --- UI Elements (Extern so billing_logic can update them) ---
extern lv_obj_t * speed_label;
extern lv_obj_t * unit_label;
extern lv_obj_t * ui_dashboard_screen;

// page navigation 
// void ui_goto_dashboard(void);

// Initialize the dashboard layout in memory
void ui_dashboard_init(void);

// Clear the screen and display the dashboard
void ui_dashboard_display(void);
void ui_update_dashboard_speed(float speed);

#endif