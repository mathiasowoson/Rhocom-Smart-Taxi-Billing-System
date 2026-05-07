#include <esp_task_wdt.h>
#include "config.h"
#include <M5Unified.h>
#include "billing_logic.h"

#ifndef TINY_GSM_MODEM_SIM7600
#define TINY_GSM_MODEM_SIM7600
#endif
#include <TinyGsmClient.h>

#include "cloudGsm_logic.h"
#include <math.h>
#include "screens/ui_dashboard.h"

extern SemaphoreHandle_t xSerialSemaphore;
extern TinyGsm modem;

// =========================
// GLOBALS
// =========================
float fuelPrice = 1300.0;
float kmlEfficiency = 10.0;

float gpsSpeed = 0.0;

float dailyUnionTotal = 0.0;
float totalFaresCollectedToday = 0.0f;
int validCheckinsToday = 0;

const float WAITING_CHARGE = 10.0;
const uint32_t FIVE_MINUTES = 300000;
uint32_t stationaryStartTime[10] = {0};

//Trip History strucure
TripHistory tripHistory[10];
int historyIndex = 0;

// =========================
// GPS STATE CONTROL
// =========================
 bool gpsInitialized = false;
//  unsigned long gpsStartTime = 0;


// =========================
// GET GPS USING TINYGSM (STABLE)
// =========================
bool get_gps_data(float &lat, float &lon, float &speed) {
    if (!gpsInitialized) return false;

    // 1. Try to get the line
    if (xSemaphoreTake(xSerialSemaphore, pdMS_TO_TICKS(500)) == pdTRUE) {
        float alt;
        int dummySats;
        
        // 2. Ask modem for data
        bool ok = modem.getGPS(&lat, &lon, &speed, &alt, &dummySats);

        // 3. Release the line immediately so other tasks can use it
        xSemaphoreGive(xSerialSemaphore);

        // 4. Debug Printing
        if (ok && lat != 0.0) {
            // Print the data you requested to see
            Serial.printf(">>> GPS FIX: Lat: %.6f | Lon: %.6f | Speed: %.1f km/h | Sats: %d\n", 
                          lat, lon, speed, dummySats);
            return true;
        } else {
            // This prints if the modem is communicating but hasn't found satellites yet
            static uint32_t lastNoFixMsg = 0;
            if (millis() - lastNoFixMsg > 5000) { // Limit prints to every 5 seconds
                Serial.println(">>> GPS: Searching for Satellites...");
                lastNoFixMsg = millis();
            }
        }
    } else {
        // This prints if the Cloud Sync task is currently using the modem
        Serial.println(">>> GPS: Waiting for Serial line (Busy)...");
    }
    
    return false;
}

// --- 3. TAXI BILLING CALCULATIONS ---
void billing_init(void) {
    for (int i = 0; i < 10; i++) {
        billing_reset_tag(i);
    }

    // gps_init_sequence();  //  NEW
    Serial.println("Billing: Initialized and waiting for GPS Hardware...");
}

// =========================
// HAVERSINE
// =========================
float calculate_haversine(float lat1, float lon1, float lat2, float lon2) {
    float dLat = (lat2 - lat1) * M_PI / 180.0;
    float dLon = (lon2 - lon1) * M_PI / 180.0;

    float a = pow(sin(dLat/2), 2) +
              cos(lat1*M_PI/180.0) *
              cos(lat2*M_PI/180.0) *
              pow(sin(dLon/2), 2);

    return 2 * asin(sqrt(a)) * 6371000;
}

// =========================
// MAIN UPDATE LOOP
// =========================
void billing_update_all(void) {
    // 1. Timing Control
    static uint32_t last_read = 0;
    if (millis() - last_read < 5000) return;
    last_read = millis();

    // 2. Persistent Storage for Grace Period
    static float lastValidLat = 0, lastValidLon = 0, lastValidSpeed = 0;
    static uint32_t lastFixTimestamp = 0;
    const uint32_t GPS_GRACE_PERIOD = 20000; // 20 seconds grace for Blynk interference

    float nLat, nLon, nSpeed;
    bool hasFix = get_gps_data(nLat, nLon, nSpeed);

    if (hasFix) {
        // Update our "Last Known Good" data
        lastValidLat = nLat;
        lastValidLon = nLon;
        lastValidSpeed = nSpeed;
        lastFixTimestamp = millis();
    } 
    
    // 3. Determine if we use Real Data, Grace Data, or Test Mode
    bool useStoredFix = (millis() - lastFixTimestamp < GPS_GRACE_PERIOD) && (lastValidLat != 0);

    if (!hasFix && !useStoredFix) {
        // =========================
        // DEBUG UI MODE (Only if fix is truly lost > 20s)
        // =========================
        static float debugSpeed = 0;
        debugSpeed += 1.5;
        if (debugSpeed > 120) debugSpeed = 0;

        Serial.println("GPS: No Fix - UI Test Mode");
        ui_update_dashboard_speed(debugSpeed);
        return;
    }

    // If we are here, we are using either fresh data (nLat) or grace data (lastValidLat)
    float activeLat = hasFix ? nLat : lastValidLat;
    float activeLon = hasFix ? nLon : lastValidLon;
    float activeSpeed = hasFix ? nSpeed : lastValidSpeed;

    // =========================
    // REAL GPS DATA (OR GRACE DATA)
    // =========================
    gpsSpeed = activeSpeed;
    ui_update_dashboard_speed(gpsSpeed);

    // =========================
    // BILLING CALCULATION
    // =========================
    for (int i = 0; i < 10; i++) {
        if (!tags[i].isActive) continue;

        // FIRST GPS LOCK FOR THIS PASSENGER
        if (tags[i].lastLat == 0) {
            tags[i].lastLat = activeLat;
            tags[i].lastLon = activeLon;
            tags[i].startLat = activeLat;
            tags[i].startLon = activeLon;
            Serial.printf("Passenger %d trip started at %.6f, %.6f\n", i, activeLat, activeLon);
            continue;
        }

        // Only calculate distance if we have a FRESH fix this loop
        // This prevents charging the passenger multiple times for the same spot during grace period
        if (hasFix) {
            float delta = calculate_haversine(
                tags[i].lastLat,
                tags[i].lastLon,
                activeLat,
                activeLon
            );

            // Total trip distance for logs
            float totalDistance = calculate_haversine(
                tags[i].startLat,
                tags[i].startLon,
                activeLat,
                activeLon
            );

            // FILTER GPS NOISE
            if (delta > 5.0 && delta < 500.0) {
                float deltaKM = delta / 1000.0;
                float fuelCost = (deltaKM / kmlEfficiency) * fuelPrice;
                tags[i].currentFare += fuelCost;
                stationaryStartTime[i] = 0; // Reset waiting timer as they moved
            }

            // UPDATE POSITION
            tags[i].lastLat = activeLat;
            tags[i].lastLon = activeLon;
        }

        // WAITING CHARGE (Logic applies even during brief Blynk blocks)
        if (activeSpeed < 2.0) {
            if (stationaryStartTime[i] == 0) {
                stationaryStartTime[i] = millis();
            } 
            else if (millis() - stationaryStartTime[i] >= FIVE_MINUTES) {
                tags[i].currentFare += WAITING_CHARGE;
                stationaryStartTime[i] = millis();
            }
        }
    }
}

void calculate_final_fare(int slot) {
    if(tags[slot].isActive) {

        // 1. Archive to History
        tripHistory[historyIndex].tagId = slot + 1;
        tripHistory[historyIndex].startLat = tags[slot].startLat;
        tripHistory[historyIndex].startLon = tags[slot].startLon;
        tripHistory[historyIndex].endLat = tags[slot].lastLat;
        tripHistory[historyIndex].endLon = tags[slot].lastLon;
        tripHistory[historyIndex].fare = tags[slot].currentFare;
        tripHistory[historyIndex].duration = (millis() - tags[slot].startTime) / 1000;
        tripHistory[historyIndex].isSynced = false;
        tripHistory[historyIndex].isValid = true;

        historyIndex = (historyIndex + 1) % 10; // Circular buffer

        totalFaresCollectedToday += tags[slot].currentFare;
        tags[slot].isActive = false;
        tags[slot].currentFare = 0;
        
        // Sync to Cloud immediately
        Serial.println("Billing: Trip finalized. Cloud will sync shortly."); 
    }
}

void billing_start_trip(int id) {
    if (id >= 0 && id < 10) {

        tags[id].isActive = true;
        tags[id].currentFare = 200.0;
        tags[id].startTime = millis();

        // Initialize GPS tracking for THIS passenger
        tags[id].startLat = 0;
        tags[id].startLon = 0;
        tags[id].lastLat = 0;
        tags[id].lastLon = 0;
    }
}

void billing_reset_tag(int id) {
    tags[id].isActive = false;
    tags[id].currentFare = 0.0;
    tags[id].id = -1;

    tags[id].startLat = 0;
    tags[id].startLon = 0;
    tags[id].lastLat = 0;
    tags[id].lastLon = 0;
}