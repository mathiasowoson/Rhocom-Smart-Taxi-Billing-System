#ifndef UI_UNION_H
#define UI_UNION_H

#include "config.h"

// Launch the Union screen
void ui_goto_union_validation(void);

// Internal helper to create the keypad
void create_station_keypad(lv_obj_t * parent);

#endif