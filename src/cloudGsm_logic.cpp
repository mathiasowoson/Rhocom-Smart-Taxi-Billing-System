#include <esp_task_wdt.h> 
#include "cloudGsm_logic.h"
#include "screens/ui_passenger.h"
#include "billing_logic.h"
#include "ThingSpeak.h"
#include <TinyGsmClient.h>

#define SerialAT Serial2
TinyGsm modem(SerialAT);
extern SemaphoreHandle_t xSerialSemaphore; // Prevents GPS and Blynk from crashing Serial2

TinyGsmClient client(modem);

#define MODEM_PWRKEY 4

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

        ThingSpeak.begin(client);

        for (;;) {
            // We sync every 30 seconds for stability and to respect ThingSpeak limits
            cloud_gsm_sync();
             vTaskDelay(pdMS_TO_TICKS(30000)); 
        }
        
        xSemaphoreGive(xSerialSemaphore);
    }

}


void processCloudCommand(String cmd) {
    cmd.trim(); // Clean up any hidden whitespace
    if (cmd.length() == 0 || cmd == "OK") return; // Ignore empty or status responses

    if (cmd == "ADD_PASSENGER") {
        int slot = -1;
        for(int i = 0; i < 10; i++) {
            if(!tags[i].isActive) { slot = i; break; }
        }
        
        if (slot != -1) {
            billing_start_trip(slot);
            Serial.printf("Cloud: Added Passenger to Slot %d\n", slot);
            
            // This is critical for the M5Stack screen to update
            ui_refresh_passenger_list(); 
        }
    } 
    else if (cmd.startsWith("SET_FUEL_")) {
        String val = cmd.substring(9);
        fuelPrice = val.toFloat();
        Serial.printf("Cloud Update: Fuel Price is now %.2f\n", fuelPrice);
    } 
    else if (cmd.startsWith("SET_KML_")) {
        String val = cmd.substring(8);
        kmlEfficiency = val.toFloat();
        Serial.printf("Cloud Update: KML Efficiency is now %.2f\n", kmlEfficiency);
    }
}

void checkTalkBack() {
    // 1. Send Command to Execute (This fetches and deletes the top command)
    String url = "api.thingspeak.com/talkbacks/" + String(SECRET_TALKBACK_ID) + "/commands/execute?api_key=" + String(SECRET_TALKBACK_KEY);
    
    // 2. Lock the Serial line
    if (xSemaphoreTake(xSerialSemaphore, pdMS_TO_TICKS(2000)) == pdTRUE) {
        
        modem.sendAT("+HTTPINIT");
        modem.waitResponse();
        modem.sendAT("+HTTPPARA=\"URL\",\"" + url + "\"");
        modem.waitResponse();
        modem.sendAT("+HTTPACTION=0"); // GET
        
        // Wait for the action completion (+HTTPACTION: 0,200,[len])
        if (modem.waitResponse(10000, "+HTTPACTION: 0,200,") == 1) {
            String responseLen = SerialAT.readStringUntil('\n');
            int len = responseLen.toInt();
            
            if (len > 0) {
                modem.sendAT("+HTTPREAD");
                if (modem.waitResponse(5000, "+HTTPREAD: ") == 1) {
                    String skipHeader = SerialAT.readStringUntil('\n'); // Skip length line
                    String commandBody = SerialAT.readStringUntil('\n'); // This is your "ADD_PASSENGER"
                    
                    // 3. Process the command using your new logic
                    processCloudCommand(commandBody);
                }
            }
        }
        
        modem.sendAT("+HTTPTERM");
        modem.waitResponse();
        
        xSemaphoreGive(xSerialSemaphore);
    }
}

void cloud_gsm_sync(void) {
    if (xSemaphoreTake(xSerialSemaphore, pdMS_TO_TICKS(5000)) == pdTRUE) {
        
        // 1. CALCULATE REVENUE TOTALS
        float activeFares = 0;
        for(int i = 0; i < 10; i++) {
            if(tags[i].isActive) activeFares += tags[i].currentFare;
        }
        float grossPassengerFares = totalFaresCollectedToday + activeFares;
        float driverNetEarn = grossPassengerFares - dailyUnionTotal;
        float grandTotal = grossPassengerFares + dailyUnionTotal;

        // 2. SET THINGSPEAK FIELDS
        ThingSpeak.setField(1, grandTotal);
        ThingSpeak.setField(2, driverNetEarn);
        ThingSpeak.setField(3, grossPassengerFares);
        ThingSpeak.setField(4, dailyUnionTotal);
        ThingSpeak.setField(5, (float)validCheckinsToday);
        ThingSpeak.setField(6, fuelPrice);
        ThingSpeak.setField(7, kmlEfficiency);

        // 3. SET GPS LOCATION (For the Map Widget)
        float flat, flon, fspeed;
        if(get_gps_data(flat, flon, fspeed)) {
            ThingSpeak.setLatitude(flat);
            ThingSpeak.setLongitude(flon);
        }

        // 4. WRITE TO CLOUD
        int x = ThingSpeak.writeFields(SECRET_CH_ID, SECRET_WRITE_APIKEY);
        if(x == 200) Serial.println("Cloud: Update Successful.");

        // 5. CHECK TALKBACK COMMANDS
        checkTalkBack();

        xSemaphoreGive(xSerialSemaphore);
    }
}

void cloud_gsm_setup(void) {
    // We use xTaskCreatePinnedToCore to ensure the GSM logic 
    // stays on Core 0, leaving Core 1 free for LVGL UI and Billing logic.
    xTaskCreatePinnedToCore(
        gsmTask,            /* Function that implements the task */
        "GSM_Task",         /* Stack Name */
        10000,              /* Stack size in words (Adjust if you see stack overflow) */
        NULL,               /* Task input parameter */
        1,                  /* Priority of the task */
        NULL,               /* Task handle */
        0                   /* Core ID: 0 */
    );

    Serial.println(">>> [SYSTEM] GSM Task Launched on Core 0");
}