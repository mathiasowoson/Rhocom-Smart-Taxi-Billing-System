// 1. ABSOLUTE FIRST: Blynk Template Credentials
#define BLYNK_TEMPLATE_ID   "TMPL2KreLiWWX"
#define BLYNK_TEMPLATE_NAME "Rhocom Smart Taxi Billing System"
#define BLYNK_AUTH_TOKEN    "S5S6SLRbnWT3yYnN-w9JU4dcARAt2TYQ"

// 2. MODEM & PRINT SETTINGS
#define BLYNK_PRINT Serial
#ifndef TINY_GSM_MODEM_SIM7600
  #define TINY_GSM_MODEM_SIM7600
#endif

// 3. LIBRARIES (They will now "see" the defines above)
#include <TinyGsmClient.h>
#include <BlynkSimpleTinyGSM.h>

// 4. PROJECT HEADERS
#include "blynkGsm_logic.h"
#include "billing_logic.h"
#include "config.h"

// --- Global Modem Definition ---
TinyGsm modem(Serial2);

// --- Blynk Virtual Pin Triggers ---
BLYNK_WRITE(VPIN_ADD_TAG) {
    if (param.asInt() == 1) {
        int slot = -1;
        for(int i=0; i<10; i++) {
            if(!tags[i].isActive) { slot = i; break; }
        }
        if (slot != -1) {
            billing_start_trip(slot);
            Serial.printf("Blynk: Added Passenger to Slot %d\n", slot);
        }
    }
}

// --- Core Functions ---

void blynk_gsm_setup(void) {
    // Start Serial2 for the COM.X (M5Core2 Pins 13 & 14)
    Serial2.begin(115200, SERIAL_8N1, 13, 14);
    delay(3000); 

    Serial.println("GSM: Initializing SIM7600...");
    if (!modem.restart()) {
        Serial.println("GSM: Modem Restart Failed");
        // We continue anyway, Blynk will try to reconnect later
    }

    // Blynk.begin handles the GPRS connection (using 5 arguments as per your example)
    Blynk.begin(BLYNK_AUTH_TOKEN, modem, "internet", "", "");

}

void blynk_gsm_update(void) {
    Blynk.run();
}

void blynk_gsm_sync(void) {
    if (!Blynk.connected()) return;

    float activeFares = 0;
    for(int i = 0; i < 10; i++) {
        if(tags[i].isActive) activeFares += tags[i].currentFare;
    }
    
    float grossPassengerFares = totalFaresCollectedToday + activeFares;
    float driverNetEarn = grossPassengerFares - dailyUnionTotal;
    float grandTotal = grossPassengerFares + dailyUnionTotal;

    // Send everything to the cloud
    Blynk.virtualWrite(VPIN_PASSENGER_REVENUE, grossPassengerFares);
    Blynk.virtualWrite(VPIN_UNION_TOTAL, dailyUnionTotal);
    Blynk.virtualWrite(VPIN_DRIVER_NET, driverNetEarn);
    Blynk.virtualWrite(VPIN_TOTAL_REVENUE, grandTotal);
    Blynk.virtualWrite(VPIN_TODAYCHECKIN, validCheckinsToday);
    Blynk.virtualWrite(VPIN_FUEL_PRICE, fuelPrice);
    Blynk.virtualWrite(VPIN_KMLEFFICIENCY, kmlEfficiency);

    Serial.println("Blynk GSM: Full Sync Complete.");
}