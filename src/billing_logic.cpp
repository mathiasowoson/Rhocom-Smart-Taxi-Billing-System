#include <esp_task_wdt.h>
#include "config.h"
#include <M5Unified.h>
#include "billing_logic.h"

#define TINY_GSM_MODEM_SIM7600
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

// =========================
// GPS STATE CONTROL
// =========================
 bool gpsInitialized = false;
 unsigned long gpsStartTime = 0;


// =========================
// GET GPS USING TINYGSM (STABLE)
// =========================
bool get_gps_data(float &lat, float &lon, float &speed) {
    if (!gpsInitialized) return false;

    // 1. Take the Semaphore to lock out Blynk
    if (xSemaphoreTake(xSerialSemaphore, pdMS_TO_TICKS(2000)) == pdTRUE) {
        
        float alt;
        int dummySats;
        
        // 2. Get the standard data from the TinyGSM library
        // This handles the heavy lifting of parsing the basic coordinates
        bool ok = modem.getGPS(&lat, &lon, &speed, &alt, &dummySats);

        // 3. MANUAL FIX: Raw AT command for logs/satellite info
        // We use SerialAT directly to avoid the 'readResponseUntil' error
        modem.sendAT("+CGPSINFO");
        
        String res = "";
        uint32_t startWait = millis();
        
        // Wait up to 200ms for a response from the SIM7600
        while (millis() - startWait < 200) {
            while (SerialAT.available()) {
                char c = SerialAT.read();
                res += c;
            }
            if (res.indexOf("OK") != -1) break; 
        }

        // 4. Release the line so Blynk can send its heartbeats
        xSemaphoreGive(xSerialSemaphore);

        // 5. Validation for Production
        if (ok && lat != 0.0) {
            // Optional: You could log 'res' here for debugging satellite count
            // Serial.println("Raw GPS Info: " + res);
            return true;
        }
        
    } else {
        // Log this less often to avoid Serial clutter during high Blynk activity
        static uint32_t lastBlockMsg = 0;
        if (millis() - lastBlockMsg > 20000) {
            Serial.println(">>> GPS: SEMAPHORE TIMEOUT - Blynk is hogging Serial");
            lastBlockMsg = millis();
        }
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

// --- 1. LOCAL DATABASE (Simulating Backend) ---
// This struct array is our "White List". 
// In the future, we will replace this with a JSON parser from your server.
// UnionMember unionDb[] = {
//     {"ID100", "Ikeja", "Park Fee", 200.0},
//     {"ID101", "Oshodi", "Park Fee", 200.0},
//     {"ID200", "Lekki", "Maintenance", 500.0},
//     {"ID300", "Ajah", "Checkpoint", 100.0},
//     {"ID400", "Yaba", "Emergency", 150.0}
// };
// const int dbSize = sizeof(unionDb) / sizeof(unionDb[0]);

// --- 3. UNION VALIDATION LOGIC ---
/**
 * Returns:
 * 1 = Success (ID matches and belongs to the selected union)
 * 2 = Wrong Union (ID exists but driver picked the wrong category)
 * 0 = Fail (ID not found)
 */
// int validate_union_id_status(String inputId, String selectedUnion) {
//     for (int i = 0; i < dbSize; i++) {
//         if (unionDb[i].id == inputId) {
//             // Check if the ID belongs to the union type the driver selected on screen
//             // We use 'indexOf' to be safe with strings like "Park Fee (N200)"
//             if (selectedUnion.indexOf(unionDb[i].unionType) != -1) {
//                 dailyUnionTotal += unionDb[i].fee;
//                 validCheckinsToday++;
                
//                 Serial.printf("LOGIC: Validated %s at %s. N%.2f added.\n", 
//                               unionDb[i].unionType.c_str(), unionDb[i].branch.c_str(), unionDb[i].fee);
                
//                 // Push to Cloud immediately
//                 Serial.println("Logic: Union Fee Added. Pending Cloud Sync...");
//                 return 1; 
//             } else {
//                 Serial.println("LOGIC: ID valid but WRONG UNION category.");
//                 return 2; 
//             }
//         }
//     }
//     Serial.println("LOGIC: ID NOT FOUND.");
//     return 0; 
// }