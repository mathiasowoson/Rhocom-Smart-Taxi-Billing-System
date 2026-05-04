#ifndef UI_UNION_H
#define UI_UNION_H

#include <M5Unified.h>
#include "ui_manager.h"

void ui_union_init(void);
void ui_union_handle_touch(m5::touch_detail_t &t);
void show_status_msg(const char * msg, uint32_t color);

#endif