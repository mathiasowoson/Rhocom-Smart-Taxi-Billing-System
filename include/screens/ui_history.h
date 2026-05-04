#ifndef UI_HISTORY_H
#define UI_HISTORY_H

#include <M5Unified.h>
#include "ui_manager.h"

// Initialize and draw the history screen
void ui_history_init(void);

// Handle touch for the back button or list scrolling
void ui_history_handle_touch(m5::touch_detail_t &t);

#endif