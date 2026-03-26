#ifndef BILLING_LOGIC_H
#define BILLING_LOGIC_H

#include "config.h"
#include "blynk_logic.h" // <--- This gives billing access to the Modem
// 1. Add the Modem definitions so the header knows what "TinyGsm" is
// #define TINY_GSM_MODEM_SIM7080 
// #include <TinyGsmClient.h>

// 2. Now you can safely declare the extern modem
// extern TinyGsm modem;

// --- Global Variables (Shared across files) ---
extern float fuelPrice;       
extern float kmlEfficiency;   
extern float gpsSpeed;


// --- Core Billing Functions ---
void billing_init(void);
// Update the fare for all active passengers (Call this in the main loop)
void billing_update_all(void);

// Calculate the final fare for a specific passenger when the trip ends
float calculate_final_fare(int tag_id);

// Start tracking a new passenger
void billing_start_trip(int tag_id);

// Reset a passenger slot after payment
void billing_reset_tag(int tag_id);

// --- GPS AT Command Helpers ---
void set_gps_power(bool on);
bool get_at_gps_data(float &lat, float &lon, float &speed);
float calculate_haversine(float lat1, float lon1, float lat2, float lon2);

#endif