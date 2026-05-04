#ifndef UI_PASSENGER_H
#define UI_PASSENGER_H

#include <M5Unified.h>
#include "ui_manager.h"

void ui_passenger_init(void);

// Redraws the list of active passengers
void ui_refresh_passenger_list(void);

// Shows the fare details and "End Trip" option for a specific tag
void ui_show_passenger_modal(int tag_id);

// Handle touch inputs for this specific page
void ui_passenger_handle_touch(m5::touch_detail_t &t);

#endif