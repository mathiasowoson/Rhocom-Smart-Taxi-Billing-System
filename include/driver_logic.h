#ifndef DRIVER_LOGIC_H
#define DRIVER_LOGIC_H

#include "config.h"

// Global States
// Sound beeping
extern bool isSoundEnabled;
extern bool screen_sleeping;
extern int last_brightness;
extern int batteryLevel;

// Initialization
void driver_logic_init(void);

// Hardware Actions
void driver_set_brightness(int level);
void driver_toggle_sound(bool on);
void driver_system_shutdown(void);
void driver_system_restart(void);
void driver_factory_reset(void);

// The main hardware monitor (to be called in loop)
void driver_handle_power_button(void);

#endif