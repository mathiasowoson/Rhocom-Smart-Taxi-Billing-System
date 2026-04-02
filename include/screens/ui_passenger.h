#ifndef UI_PASSENGER_H
#define UI_PASSENGER_H

#include "config.h"

extern lv_obj_t* ui_passenger_screen;

// Initialize the screen layout (List container + Header)
void ui_passenger_init(void);

// Add a new clickable Tag (Button) to the list
// This can be called from blynk_logic.cpp when a cloud trigger arrives
void ui_add_passenger_tag(int tag_id, const char* name);

// The "Pop-Over" modal for a specific tag
void ui_show_passenger_modal(int tag_id);

#endif