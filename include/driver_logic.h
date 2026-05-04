#ifndef DRIVER_LOGIC_H
#define DRIVER_LOGIC_H

#include <M5Unified.h>

// Global States
extern bool isSoundEnabled;
extern bool screen_sleeping;
extern bool isPublicMode;
extern int last_brightness;
extern int batteryLevel;

// Initialization
void driver_logic_init(void);

// Hardware Actions
void driver_set_brightness(int level);
void driver_system_shutdown(void);
void driver_system_restart(void);
void driver_factory_reset(void);

// Hardware monitoring
void driver_handle_hardware(void);

#endif