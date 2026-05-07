#include <esp_task_wdt.h> 
#include "cloudGsm_logic.h"
#include "screens/ui_passenger.h"
#include "billing_logic.h"
#include "ThingSpeak.h"
#include <TinyGsmClient.h>

#define SerialAT Serial2
TinyGsm modem(SerialAT);
extern SemaphoreHandle_t xSerialSemaphore; // Prevents GPS and Blynk from crashing Serial2

// ThingSpeak Credentials
#define SECRET_CH_ID 3357569              // Your Channel ID
#define SECRET_WRITE_APIKEY "5DLLTK7JIVQ75BWF" // Your Write API Key

// TalkBack credentials
#define SECRET_TALKBACK_ID 56849
#define SECRET_TALKBACK_KEY "WD74QLONNKXLTJD8"

// ... other existing configs like APN ...
#define APN "your_sim_apn"

extern bool gpsInitialized;
// extern unsigned long gpsStartTime;

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
        Serial.println("GSM Task: Initializing Modem...");
        modem.restart(); // Software reset to clear old states
        
        // 2. POWER ON GPS EXPLICITLY (AT+CGPS=1,1)
        Serial.println("GSM Task: Sending AT+CGPS=1,1...");
        modem.sendAT("+CGPS=1,1"); // Start GPS in Standalone mode
        if (modem.waitResponse(2000) != 1) {
             Serial.println("GSM Task: GPS Power-on failed or already on.");
        }
        
        modem.sendAT("+CGPSHOT"); // Attempt Hot Start for faster fix
        modem.waitResponse();
        
        // 3. NETWORK SETUP
        Serial.println("GSM Task: Connecting to Network...");
        modem.sendAT("+CGACT=1,1");
        modem.waitResponse();
        modem.sendAT("+NETOPEN");
        modem.waitResponse(10000);

        // 4. THINGSPEAK BEGIN
        // This must happen AFTER the modem and client are ready
        ThingSpeak.begin(client); 

        // Mark ready for Billing Logic
        // gpsStartTime = millis();
        gpsInitialized = true; 

        xSemaphoreGive(xSerialSemaphore);
    }

    // 5. THE MAIN SYNC LOOP
    for (;;) {
        cloud_gsm_sync(); 
        vTaskDelay(pdMS_TO_TICKS(30000)); // Respect ThingSpeak 15-30s limits
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
    // We use .json to ensure the response is clean and predictable
    String url = "http://api.thingspeak.com/talkbacks/" + String(SECRET_TALKBACK_ID) + "/commands/execute?api_key=" + String(SECRET_TALKBACK_KEY);
    
    // Use a shorter timeout here; we don't want to block GPS for too long
    if (xSemaphoreTake(xSerialSemaphore, pdMS_TO_TICKS(1000)) == pdTRUE) {
        
        Serial.println("TalkBack: Checking for remote commands...");
        
        modem.sendAT("+HTTPINIT");
        modem.waitResponse();
        
        // Set URL
        modem.sendAT("+HTTPPARA=\"URL\",\"" + url + "\"");
        modem.waitResponse();
        
        // GET Request (Execute next command)
        modem.sendAT("+HTTPACTION=0"); 
        
        // Wait for server response (200 = OK)
        if (modem.waitResponse(10000, "+HTTPACTION: 0,200,") == 1) {
            String responseLen = SerialAT.readStringUntil('\n');
            int len = responseLen.toInt();
            
            if (len > 0) {
                modem.sendAT("+HTTPREAD");
                // The SIM7600 returns "+HTTPREAD: [len]" followed by the data
                if (modem.waitResponse(5000, "+HTTPREAD: ") == 1) {
                    SerialAT.readStringUntil('\n'); // Skip the length line
                    String commandBody = SerialAT.readStringUntil('\n'); 
                    commandBody.trim(); // Remove \r\n
                    
                    Serial.print("TalkBack: Received -> ");
                    Serial.println(commandBody);

                    // Process: e.g., "ADD_PASSENGER"
                    processCloudCommand(commandBody);
                }
            } else {
                Serial.println("TalkBack: Queue empty.");
            }
        }
        
        modem.sendAT("+HTTPTERM");
        modem.waitResponse();
        
        xSemaphoreGive(xSerialSemaphore);
    }
}

void cloud_gsm_sync(void) {
    // 1. Pre-check: Don't even try if the modem isn't connected to the internet.
    // This prevents the semaphore from being held while the modem struggles with a dead connection.
    if (!modem.isGprsConnected()) {
        Serial.println("Cloud: Skipping sync - No GPRS/Data connection.");
        return; 
    }

    // 2. Take the Semaphore with a reasonable timeout.
    if (xSemaphoreTake(xSerialSemaphore, pdMS_TO_TICKS(5000)) == pdTRUE) {
        
        // --- 1. CALCULATE REVENUE TOTALS ---
        float activeFares = 0;
        for(int i = 0; i < 10; i++) {
            if(tags[i].isActive) activeFares += tags[i].currentFare;
        }
        float grossPassengerFares = totalFaresCollectedToday + activeFares;
        float driverNetEarn = grossPassengerFares - dailyUnionTotal;
        float grandTotal = grossPassengerFares;

        // --- 2. SET THINGSPEAK FIELDS ---
        ThingSpeak.setField(1, grandTotal);
        ThingSpeak.setField(2, driverNetEarn);
        if (grossPassengerFares > 0) ThingSpeak.setField(3, grossPassengerFares);
        ThingSpeak.setField(4, dailyUnionTotal);
        ThingSpeak.setField(5, (float)validCheckinsToday);
        ThingSpeak.setField(6, fuelPrice);
        ThingSpeak.setField(7, kmlEfficiency);

        // --- 3. SET GPS LOCATION ---
        // Note: We are already holding the semaphore, so get_gps_data 
        // inside billing_logic needs to be careful not to take it again (Deadlock).
        // It is safer to use the global 'gpsSpeed' and stored coordinates here.
        float flat, flon, fspeed;
        if(get_gps_data(flat, flon, fspeed)) {
            ThingSpeak.setLatitude(flat);
            ThingSpeak.setLongitude(flon);
        }

        // --- 4. WRITE TO CLOUD ---
        Serial.println("Cloud: Sending data to ThingSpeak...");
        int x = ThingSpeak.writeFields(SECRET_CH_ID, SECRET_WRITE_APIKEY);
        
        if(x == 200) {
            Serial.println("Cloud: Update Successful.");
        } else {
            Serial.printf("Cloud: Update Failed (Error %d). Check API Key or Data.\n", x);
        }

        // --- 5. CHECK TALKBACK COMMANDS ---
        // This function must NOT take the semaphore again internally.
        checkTalkBack();

        // 6. Release the line for the Billing/GPS logic
        xSemaphoreGive(xSerialSemaphore);
    } else {
        Serial.println("Cloud: Sync deferred - GPS logic is currently using the Modem.");
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