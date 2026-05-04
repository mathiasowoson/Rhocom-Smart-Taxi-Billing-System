#ifndef UI_SETTINGS_H
#define UI_SETTINGS_H

#include <M5Unified.h>
#include "ui_manager.h"

// Initialize the screen and global settings state
void ui_settings_init(void);

// Handle touch events for sliders and buttons
void ui_settings_handle_touch(m5::touch_detail_t &t);

// Modal for dangerous actions
void ui_show_factory_reset_warning(void);

#endif