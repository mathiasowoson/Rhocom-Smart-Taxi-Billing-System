#ifndef BLYNK_LOGIC_H
#define BLYNK_LOGIC_H

#include "config.h"

// 1. Modem Driver Configuration (One place to change it)
#define TINY_GSM_MODEM_SIM7600 // CRITICAL: Tells library to use SIM7600 commands 
#include <TinyGsmClient.h>

// 2. Global Modem Objects
extern TinyGsm modem;
extern TinyGsmClient gsm_client;

// Initialize Blynk with COM.X (LTE) or WiFi
void blynk_setup(void);

// Call this in your main loop to handle cloud communication
void blynk_update(void);

// Sync local data to the Blynk App (e.g., total earnings)
void blynk_sync_data(void);

#endif