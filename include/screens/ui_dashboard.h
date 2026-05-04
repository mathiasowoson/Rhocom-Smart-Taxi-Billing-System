#ifndef UI_DASHBOARD_H
#define UI_DASHBOARD_H

#include <M5Unified.h>
#include "ui_manager.h"

// Externs for global access
extern float current_speed;
extern bool dashboard_needs_update;

// Initialize and display the dashboard
void ui_dashboard_init(void);

// Update logic
void ui_update_dashboard_speed(float speed);

// Function to handle touches specifically for the dashboard
void ui_dashboard_handle_touch(m5::touch_detail_t &t);

#endif