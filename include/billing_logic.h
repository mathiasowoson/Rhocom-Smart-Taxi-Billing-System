#ifndef BILLING_LOGIC_H
#define BILLING_LOGIC_H

#include <Arduino.h>
#include "config.h"


// --- 2. GLOBAL SHARED VARIABLES ---
// These are defined in billing_logic.cpp and used in UI/Blynk
extern float fuelPrice;      
extern float kmlEfficiency;  
extern float gpsSpeed;
extern float dailyUnionTotal;
extern int validCheckinsToday;
extern float totalFaresCollectedToday;
extern int activeCount;

// --- 3. DATA STRUCTURES ---
// struct UnionMember {
//     String id;
//     String branch;
//     String unionType; // e.g., "Maintenance", "Emergency"
//     float fee;
// };

// --- Passenger Data Structure ---
struct PassengerTag {
    int id;
    float startLat;
    float startLon;

    float lastLat;
    float lastLon;

    float currentFare;
    uint32_t startTime;
    bool isActive;
};

// --- Global Variables (Shared across all .cpp files) ---
extern PassengerTag tags[10];
// --- 4. CORE BILLING FUNCTIONS ---

// Trip history data structure
struct TripHistory {
    int tagId;
    float startLat, startLon;
    float endLat, endLon;
    float fare;
    uint32_t duration; // in seconds
    bool isSynced;
    bool isValid = false; // To check if the slot is occupied
};

extern TripHistory tripHistory[10]; // Store last 10 trips

// Initialize timers and turn on GPS hardware
void billing_init(void);

// Main loop function to calculate distance and fares for all passengers
void billing_update_all(void);

// Start tracking a new passenger in a specific slot
void billing_start_trip(int tag_id);

// Finalize fare, lock in the total, and trigger Blynk sync
void calculate_final_fare(int slot);

// Reset a passenger slot data
void billing_reset_tag(int tag_id);

// Validate ID against the local/remote union database
int validate_union_id_status(String inputId, String selectedUnion);

// --- 5. GPS & MATH HELPERS ---
bool get_gps_data(float &lat, float &lon, float &speed);
float calculate_haversine(float lat1, float lon1, float lat2, float lon2);

#endif