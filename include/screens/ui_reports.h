#ifndef UI_REPORTS_H
#define UI_REPORTS_H

#include <M5Unified.h>
#include "ui_manager.h"

// Draws the static report structure and live data
void ui_reports_init(void);

// Monitors touch for the back button and potential "Sync" button
void ui_reports_handle_touch(m5::touch_detail_t &t);

#endif