#include "config.h"
#include "billing_logic.h"
#include <math.h>
#include <TinyGsmClient.h>
#include "screens/ui_dashboard.h"

// Initialize Globals
float fuelPrice = 1300.0;    // Default price per Liter
float kmlEfficiency = 10.0;  // Assume 10km per Liter for now
float last_lat = 0, last_lon = 0;
float gpsSpeed = 0.0;

// Constants
const float WAITING_CHARGE = 10.0;    // N10 per 5 minutes
const uint32_t FIVE_MINUTES = 300000; // 5 minutes in milliseconds

// Timers for Waiting logic
uint32_t stationaryStartTime[10] = {0};

void billing_init(void) {
    for (int i = 0; i < 10; i++) {
        billing_reset_tag(i);
    }
    set_gps_power(true); // Turn on GNSS at startup
}

// 1. AT Commands for SIM7600G
void set_gps_power(bool on) {
    if(on) {
        modem.sendAT("+CGPS=1"); // Turn on GPS engine
    } else {
        modem.sendAT("+CGPS=0"); // Turn off
    }
    modem.waitResponse();
}

// 2. Parsed SIM7600G GPS Data (NMEA to Decimal)
bool get_at_gps_data(float &lat, float &lon, float &speed) {
    modem.sendAT("+CGPSINFO");
    // Wait for the response
    if (modem.waitResponse(1000, "+CGPSINFO: ") != 1) {
        // This means the modem didn't even answer the command
        Serial.println("MODEM ERROR: No response to AT+CGPSINFO");
        return false;
    }

    String resp = modem.stream.readStringUntil('\n');
    
    // PRINT THE RAW DATA TO TERMINAL
    Serial.print("RAW GPS DATA: ");
    Serial.println(resp); 

    if (resp.indexOf(",,,,") != -1) {
        Serial.println("GPS STATUS: Searching for Satellites...");
        return false; 
    }

    // Parse CSV: [Lat],[N],[Lon],[E],[Date],[Time],[Alt],[Speed]
    // Example: 0631.4640,N,00322.7520,E...
    int p[8];
    int lastPos = 0;
    for(int i=0; i<8; i++) {
        p[i] = resp.indexOf(',', lastPos);
        lastPos = p[i] + 1;
    }

    // Convert Latitude (DDMM.MMMM to DD.DDDD)
    String rawLat = resp.substring(0, p[0]);
    float latDeg = rawLat.substring(0, 2).toFloat();
    float latMin = rawLat.substring(2).toFloat();
    lat = latDeg + (latMin / 60.0);
    if (resp.substring(p[0]+1, p[1]) == "S") lat *= -1;

    // Convert Longitude (DDDMM.MMMM to DD.DDDD)
    String rawLon = resp.substring(p[1]+1, p[2]);
    float lonDeg = rawLon.substring(0, 3).toFloat();
    float lonMin = rawLon.substring(3).toFloat();
    lon = lonDeg + (lonMin / 60.0);
    if (resp.substring(p[2]+1, p[3]) == "W") lon *= -1;

    // Speed (Knots to KM/H)
    speed = resp.substring(p[6]+1, p[7]).toFloat() * 1.852; 

    return true;
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

void billing_start_trip(int id) {
    if (id >= 0 && id < 10) {
        tags[id].isActive = true;
        tags[id].currentFare = 200.0; // Starting Base Fare (e.g. N200)
        tags[id].startTime = millis();
    }
}

float calculate_final_fare(int tag_id) {
    if (tag_id < 0 || tag_id >= 10) return 0.0;
    tags[tag_id].isActive = false; 
    return tags[tag_id].currentFare;
}

void billing_reset_tag(int id) {
    tags[id].isActive = false;
    tags[id].currentFare = 0.0;
    tags[id].id = -1;
}