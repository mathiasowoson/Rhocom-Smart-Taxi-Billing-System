#include "billing_logic.h"
#define TINY_GSM_MODEM_SIM7600 
#include <TinyGsmClient.h>
#include "blynkGsm_logic.h" // Points to our new unified GSM Blynk file
#include <math.h>
#include "screens/ui_dashboard.h"

// --- 1. LINK TO THE MODEM DEFINED IN blynkGsm_logic.cpp ---
extern TinyGsm modem;

// Initialize Globals
float fuelPrice = 1300.0;
float kmlEfficiency = 10.0;
float last_lat = 0, last_lon = 0;
float gpsSpeed = 0.0;
float dailyUnionTotal = 0.0;
float totalFaresCollectedToday = 0.0f;
int validCheckinsToday = 0;

const float WAITING_CHARGE = 10.0;
const uint32_t FIVE_MINUTES = 300000;
uint32_t stationaryStartTime[10] = {0};

// 1. AT Commands for SIM7600G
void set_gps_power(bool on) {
    // We use modem.sendAT directly to bypass high-level library delays
    if(on) {
        modem.sendAT("+CGPS=1"); 
    } else {
        modem.sendAT("+CGPS=0"); 
    }
    modem.waitResponse();
}

bool get_at_gps_data(float &lat, float &lon, float &speed) {
    modem.sendAT("+CGPSINFO");
    if (modem.waitResponse(1000, "+CGPSINFO: ") != 1) {
        return false;
    }

    String resp = modem.stream.readStringUntil('\n');
    resp.trim();

    if (resp.length() < 10 || resp.indexOf(",,,,") != -1) {
        return false; 
    }

    // Parse CSV: [Lat],[N],[Lon],[E],[Date],[Time],[Alt],[Speed]
    int p[8];
    int lastPos = 0;
    for(int i=0; i<8; i++) {
        p[i] = resp.indexOf(',', lastPos);
        if (p[i] == -1) break;
        lastPos = p[i] + 1;
    }

    // Latitude
    String rawLat = resp.substring(0, p[0]);
    lat = rawLat.substring(0, 2).toFloat() + (rawLat.substring(2).toFloat() / 60.0);
    if (resp.substring(p[0]+1, p[1]) == "S") lat *= -1;

    // Longitude
    String rawLon = resp.substring(p[1]+1, p[2]);
    lon = rawLon.substring(0, 3).toFloat() + (rawLon.substring(3).toFloat() / 60.0);
    if (resp.substring(p[2]+1, p[3]) == "W") lon *= -1;

    // Speed (Knots to KM/H)
    speed = resp.substring(p[6]+1, p[7]).toFloat() * 1.852; 
    return true;
}

// --- 3. TAXI BILLING CALCULATIONS ---

void billing_init(void) {
    for (int i = 0; i < 10; i++) {
        billing_reset_tag(i);
    }
    set_gps_power(true); 
}

// 3. Haversine Math
float calculate_haversine(float lat1, float lon1, float lat2, float lon2) {
    float dLat = (lat2 - lat1) * M_PI / 180.0;
    float dLon = (lon2 - lon1) * M_PI / 180.0;
    float a = pow(sin(dLat/2), 2) + cos(lat1*M_PI/180.0) * cos(lat2*M_PI/180.0) * pow(sin(dLon/2), 2);
    return 2 * asin(sqrt(a)) * 6371000; // Meters
}

// 4. Main Update Logic
void billing_update_all(void) {
    static uint32_t last_read = 0;
    if (millis() - last_read < 2000) return; 
    last_read = millis();

    float nLat, nLon, nSpeed;
    bool hasFix = get_at_gps_data(nLat, nLon, nSpeed);

    // ==========================================
    // DEBUGGING / TESTING LOGIC
    // ==========================================
    if (!hasFix) {
        // If NO GPS signal, show a "Test Speed" that counts up
        static float debugSpeed = 0;
        debugSpeed += 1.5;
        if(debugSpeed > 120) debugSpeed = 0;
        
        Serial.println("GPS: No Fix - Running UI Test Mode");
        ui_update_dashboard_speed(debugSpeed); 
    } else {
        // Real GPS Data
        gpsSpeed = nSpeed;
        Serial.printf("GPS FIX! Speed: %.2f km/h\n", gpsSpeed);
        ui_update_dashboard_speed(gpsSpeed);
    }
    // ==========================================

    if (hasFix) {
        for (int i = 0; i < 10; i++) {
            if (!tags[i].isActive) continue;

            // --- Fuel Based Calculation ---
            if (last_lat != 0) {
                float delta = calculate_haversine(last_lat, last_lon, nLat, nLon);
                // Filter jumps (ignore small drift < 5m or GPS teleportation > 500m)
                if (delta > 5.0 && delta < 500.0) {
                    float deltaKM = delta / 1000.0;
                    float fuelCost = (deltaKM / kmlEfficiency) * fuelPrice;
                    tags[i].currentFare += fuelCost;
                    stationaryStartTime[i] = 0; 
                }
            }

            // --- Waiting Charge Calculation ---
            if (gpsSpeed < 2.0) {
                if (stationaryStartTime[i] == 0) {
                    stationaryStartTime[i] = millis();
                } else if (millis() - stationaryStartTime[i] >= FIVE_MINUTES) {
                    tags[i].currentFare += WAITING_CHARGE;
                    stationaryStartTime[i] = millis(); 
                }
            }
        }
        last_lat = nLat; last_lon = nLon;
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
        tags[id].currentFare = 200.0; // Base Fare
        tags[id].startTime = millis();
    }
}

void billing_reset_tag(int id) {
    tags[id].isActive = false;
    tags[id].currentFare = 0.0;
    tags[id].id = -1;
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