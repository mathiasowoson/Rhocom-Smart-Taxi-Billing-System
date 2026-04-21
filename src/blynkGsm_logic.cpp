// 1. ABSOLUTE FIRST: Blynk Template Credentials
#define BLYNK_TEMPLATE_ID   "TMPL2KreLiWWX"
#define BLYNK_TEMPLATE_NAME "Rhocom Smart Taxi Billing System"
#define BLYNK_AUTH_TOKEN    "S5S6SLRbnWT3yYnN-w9JU4dcARAt2TYQ"

#include <esp_task_wdt.h>
#include "blynkGsm_logic.h"
#include "billing_logic.h"
#include "screens/ui_passenger.h"
#include "config.h"
#include <TinyGsmClient.h>
#include <BlynkSimpleTinyGSM.h>

// Definitions
#define MODEM_PWRKEY 4
#define SerialAT Serial2

TinyGsm modem(SerialAT);
extern SemaphoreHandle_t xSerialSemaphore; // Prevents GPS and Blynk from crashing Serial2

// Helper for non-blocking delay that feeds the watchdog
void watchdogSafeDelay(uint32_t ms) {
    uint32_t start = millis();
    while (millis() - start < ms) {
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void powerOnModem() {
    pinMode(MODEM_PWRKEY, OUTPUT);
    digitalWrite(MODEM_PWRKEY, LOW); delay(1000);
    digitalWrite(MODEM_PWRKEY, HIGH); delay(2000);
    digitalWrite(MODEM_PWRKEY, LOW); delay(3000); 
}

// The Background Task Function
void gsmTask(void *pvParameters) {
    powerOnModem();
    SerialAT.begin(115200);

    // 1. Wait for Modem Boot
    vTaskDelay(pdMS_TO_TICKS(5000));

    if (xSemaphoreTake(xSerialSemaphore, portMAX_DELAY)) {
        // 2. MODEM_INIT (Matching your debug code)
        Serial.println("GSM Task: MODEM RESTART...");
        modem.restart();
        
        // 3. NETWORK_READY
        Serial.println("GSM Task: SETTING NETWORK...");
        modem.sendAT("+CGACT=1,1");
        modem.waitResponse();
        modem.sendAT("+NETOPEN");
        modem.waitResponse(10000);

        // 4. GPS_ON
        Serial.println("GSM Task: TURNING GPS ON...");
        modem.enableGPS();
        modem.sendAT("+CGPSHOT"); // Hot start
        modem.waitResponse();
        modem.sendAT("+CGPSANT=1"); // Antenna Power
        modem.waitResponse();

        // Mark ready for Billing Logic
        extern bool gpsInitialized;
        extern unsigned long gpsStartTime;
        gpsStartTime = millis();
        gpsInitialized = true;

        // 5. BLYNK_CONFIG (Last step)
        Serial.println("GSM Task: STARTING BLYNK...");
        Blynk.config(modem, BLYNK_AUTH_TOKEN, "blynk.cloud", 80);
        
        xSemaphoreGive(xSerialSemaphore);
    }

    for (;;) {
        // Only run Blynk if the semaphore is available quickly. 
        // If the GPS task is holding it, Blynk will wait.
        if (xSemaphoreTake(xSerialSemaphore, pdMS_TO_TICKS(50))) {
            if (Blynk.connected()) {
                Blynk.run();
            } else {
                Blynk.connect();
            }
            xSemaphoreGive(xSerialSemaphore);
        }
        // IMPORTANT: Give the modem a "rest" every 500ms 
        // This gap allows the Billing Logic to slide in a GPS request!
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
}

void blynk_gsm_setup(void) {
    xSerialSemaphore = xSemaphoreCreateMutex();
    // Create the task on Core 0 (Main UI/Billing stays on Core 1)
    xTaskCreatePinnedToCore(
        gsmTask,        // Function
        "GSM_Task",     // Name
        10000,          // Stack size
        NULL,           // Parameter
        1,              // Priority
        NULL,           // Task Handle
        0               // Core ID (0)
    );
}

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

            // CRITICAL: Refresh the UI so the driver sees the new tag pop up!
            ui_refresh_passenger_list();
        }
    }
}

void blynk_gsm_update(void) {
    // Empty! Handled by gsmTask on Core 0
}

void blynk_gsm_sync(void) {
    if (!Blynk.connected()) return;
    
    // We attempt to take the lock. If GPS is busy, we wait briefly.
    if (xSemaphoreTake(xSerialSemaphore, pdMS_TO_TICKS(100))) {
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
        
        xSemaphoreGive(xSerialSemaphore);
    }
}