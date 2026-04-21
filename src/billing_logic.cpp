#include <esp_task_wdt.h>
#include "billing_logic.h"

#define TINY_GSM_MODEM_SIM7600
#include <TinyGsmClient.h>

#include "blynkGsm_logic.h"
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

    float alt;
    int sats = 0;

    // Try to take the semaphore. If it fails, we want to know!
    if (xSemaphoreTake(xSerialSemaphore, pdMS_TO_TICKS(2000)) == pdTRUE) {
        
        Serial.println(">>> GPS: Querying Modem..."); 
        
        // We use the more detailed getGPS call to see satellite count
        bool ok = modem.getGPS(&lat, &lon, &speed, &alt, &sats);
        
        xSemaphoreGive(xSerialSemaphore);

        if (ok) {
            if (lat != 0.0) {
                Serial.printf(">>> GPS: FIX OK! Sats: %d, Lat: %.6f\n", sats, lat);
                return true;
            } else {
                Serial.printf(">>> GPS: NO LOCK. Sats: %d (Modem responded but no fix)\n", sats);
            }
        } else {
            Serial.println(">>> GPS: MODEM TIMEOUT (No response from SIM7600)");
        }
    } else {
        Serial.println(">>> GPS: SEMAPHORE BLOCKED by Blynk");
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

    static uint32_t last_read = 0;
    if (millis() - last_read < 5000) return;
    last_read = millis();


    float nLat, nLon, nSpeed;
    bool hasFix = get_gps_data(nLat, nLon, nSpeed);

    // =========================
    // DEBUG UI MODE
    // =========================
    if (!hasFix) {
        static float debugSpeed = 0;
        debugSpeed += 1.5;
        if (debugSpeed > 120) debugSpeed = 0;

        Serial.println("GPS: No Fix - UI Test Mode");
        ui_update_dashboard_speed(debugSpeed);
        return;
    }

    // =========================
    // REAL GPS DATA
    // =========================
    gpsSpeed = nSpeed;
    ui_update_dashboard_speed(gpsSpeed);

    // =========================
    // BILLING CALCULATION
    // =========================
    for (int i = 0; i < 10; i++) {

        if (!tags[i].isActive) continue;

        // =========================
        // FIRST GPS LOCK FOR THIS PASSENGER
        // =========================
        if (tags[i].lastLat == 0) {

            tags[i].lastLat = nLat;
            tags[i].lastLon = nLon;

            // Save trip starting point ONCE
            tags[i].startLat = nLat;
            tags[i].startLon = nLon;

            Serial.printf("Passenger %d trip started at %.6f, %.6f\n", i, nLat, nLon);

            continue;
        }

        // =========================
        // DISTANCE CALCULATION PER PASSENGER
        // =========================
        float delta = calculate_haversine(
            tags[i].lastLat,
            tags[i].lastLon,
            nLat,
            nLon
        );

        // Optional: Total trip distance
        float totalDistance = calculate_haversine(
            tags[i].startLat,
            tags[i].startLon,
            nLat,
            nLon
        );

        Serial.printf("Passenger %d total distance: %.2f m\n", i, totalDistance);

        // =========================
        // FILTER GPS NOISE
        // =========================
        if (delta > 5.0 && delta < 500.0) {

            float deltaKM = delta / 1000.0;
            float fuelCost = (deltaKM / kmlEfficiency) * fuelPrice;

            tags[i].currentFare += fuelCost;

            stationaryStartTime[i] = 0;
        }

        // =========================
        // WAITING CHARGE
        // =========================
        if (gpsSpeed < 2.0) {

            if (stationaryStartTime[i] == 0) {
                stationaryStartTime[i] = millis();
            } 
            else if (millis() - stationaryStartTime[i] >= FIVE_MINUTES) {
                tags[i].currentFare += WAITING_CHARGE;
                stationaryStartTime[i] = millis();
            }
        }

        // =========================
        // UPDATE POSITION (CRITICAL)
        // =========================
        tags[i].lastLat = nLat;
        tags[i].lastLon = nLon;
    }
}

void calculate_final_fare(int slot) {
    if(tags[slot].isActive) {
        totalFaresCollectedToday += tags[slot].currentFare;
        tags[slot].isActive = false;
        tags[slot].currentFare = 0;
        
        // Sync to Cloud immediately
        blynk_gsm_sync(); 
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
UnionMember unionDb[] = {
    {"ID100", "Ikeja", "Park Fee", 200.0},
    {"ID101", "Oshodi", "Park Fee", 200.0},
    {"ID200", "Lekki", "Maintenance", 500.0},
    {"ID300", "Ajah", "Checkpoint", 100.0},
    {"ID400", "Yaba", "Emergency", 150.0}
};
const int dbSize = sizeof(unionDb) / sizeof(unionDb[0]);

// --- 3. UNION VALIDATION LOGIC ---
/**
 * Returns:
 * 1 = Success (ID matches and belongs to the selected union)
 * 2 = Wrong Union (ID exists but driver picked the wrong category)
 * 0 = Fail (ID not found)
 */
int validate_union_id_status(String inputId, String selectedUnion) {
    for (int i = 0; i < dbSize; i++) {
        if (unionDb[i].id == inputId) {
            // Check if the ID belongs to the union type the driver selected on screen
            // We use 'indexOf' to be safe with strings like "Park Fee (N200)"
            if (selectedUnion.indexOf(unionDb[i].unionType) != -1) {
                dailyUnionTotal += unionDb[i].fee;
                validCheckinsToday++;
                
                Serial.printf("LOGIC: Validated %s at %s. N%.2f added.\n", 
                              unionDb[i].unionType.c_str(), unionDb[i].branch.c_str(), unionDb[i].fee);
                
                // Push to Cloud immediately
                blynk_gsm_sync();
                return 1; 
            } else {
                Serial.println("LOGIC: ID valid but WRONG UNION category.");
                return 2; 
            }
        }
    }
    Serial.println("LOGIC: ID NOT FOUND.");
    return 0; 
}