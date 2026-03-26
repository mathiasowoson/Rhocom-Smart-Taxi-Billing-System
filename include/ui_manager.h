#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "config.h"

// --- Function Prototypes ---

// Initialize the UI system and show the first screen
void ui_init(void);

// Universal Header: Call this in every screen_init function
void ui_create_header(lv_obj_t* parent);

// Navigation: Use these to switch between your .cpp screens
void ui_goto_dashboard(void);
void ui_goto_passenger_mgmt(void);
void ui_goto_union_validation(void);
void ui_goto_reports(void);
void ui_goto_settings(void);

// Update functions for the Header icons (call these from main loop or logic)
void ui_update_status_bar(bool gps_fixed, bool cloud_conn, int batt);

#endif