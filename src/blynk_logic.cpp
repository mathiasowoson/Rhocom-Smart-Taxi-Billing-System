#include "config.h"
#include <esp_task_wdt.h>
#include "billing_logic.h"
#include "blynk_logic.h"

// 1. GSM / LTE Headers
// #define TINY_GSM_MODEM_SIM7080 // Matches your COM.X specific modem

#include <BlynkSimpleTinyGSM.h> // <--- Put it ONLY here!
#define BLYNK_PRINT Serial
#include <TinyGsmClient.h>

// 2. The ONLY Blynk header we need for Hybrid mode
// This header is flexible enough to handle GSM, and we will manually 
// feed it WiFi if needed.
#include <BlynkSimpleTinyGSM.h> 

// 3. Project Headers
#include "blynk_logic.h"
#include <WiFi.h>
#include "screens/ui_passenger.h"
#include "billing_logic.h"

// --- COM.X LTE Module Hardware Pins ---
#define UART_BAUD 115200
#define RX_PIN 13
#define TX_PIN 14

TinyGsm modem(Serial2);
TinyGsmClient gsm_client(modem);

// --- Blynk Triggers ---

BLYNK_WRITE(VPIN_ADD_TAG) {
    int value = param.asInt(); 
    if (value == 1 && activeCount < 10) {
        billing_start_trip(activeCount);
        ui_add_passenger_tag(activeCount, "Trip Active");
        activeCount++;
        Serial.printf("Cloud Trigger: Added Passenger #%d\n", activeCount - 1);
    }
}

BLYNK_WRITE(VPIN_VALIDATE) { // Fixed VPIN name from your config
    int status = param.asInt();
    // Logic for Union validation
}

BLYNK_READ(VPIN_REVENUE) {
    float total = 0;
    for(int i=0; i<10; i++) {
        if(tags[i].isActive) total += tags[i].currentFare;
    }
    Blynk.virtualWrite(VPIN_REVENUE, total);
}

// Virtual Pin for Fuel Price update(e.g., V10)
BLYNK_WRITE(V10) {
    fuelPrice = param.asFloat();
    Serial.print("Cloud Update: New Fuel Price = N");
    Serial.println(fuelPrice);
}

BLYNK_WRITE(V11) { // Virtual Pin V11 for Efficiency (KM/L)
    kmlEfficiency = param.asFloat();
    Serial.printf("Cloud Sync: Vehicle Efficiency updated to %.1f KM/L\n", kmlEfficiency);
}

// --- The Hybrid Setup Logic ---

void blynk_setup(void) {
    // 1. Disable Watchdog for boot
    esp_task_wdt_deinit(); 

    Serial.println("Blynk Setup Started...");

    // 2. SAFETY CHECK: If you haven't entered credentials, SKIP connection
    // This prevents the infinite loop and the 5-second reset
    if (String(BLYNK_AUTH_TOKEN) == "xxxxxxxxxx" || String("Your_SSID") == "Your_SSID") {
        Serial.println("WDT BYPASS: No Credentials found. Skipping network boot to prevent reset.");
        return; // Exit function early so the rest of the app can run
    }

    Serial2.begin(UART_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);

    if (currentNetMode == MODE_WIFI) {
        Serial.println("Network: WiFi Mode Selected.");
        WiFi.begin("Your_SSID", "Your_PASS"); 
        
        int attempt = 0;
        while (WiFi.status() != WL_CONNECTED && attempt < 20) {
            delay(500);
            Serial.print(".");
            attempt++;
        }

        if(WiFi.status() == WL_CONNECTED) {
            Serial.println("\nWiFi Connected!");
            // Blynk connects via the WiFi Client
            Blynk.config(modem, BLYNK_AUTH_TOKEN);
            Blynk.connect();
        }
    } 
    else {
        Serial.println("Network: LTE Mode (COM.X) Selected.");
        if (!modem.restart()) {
           Blynk.begin(BLYNK_AUTH_TOKEN, modem, "internet", "", "");
        } else {
            Serial.println("Modem not found. Continuing in offline mode.");
        }
        // const char* apn = "internet"; 
        // Blynk.begin(BLYNK_AUTH_TOKEN, modem, apn, "", "");
    }
    // RE-ARM THE WATCHDOG: Now that we are connected, set it to 5 seconds
    esp_task_wdt_init(5, true); 
    esp_task_wdt_add(NULL);
    Serial.println("\nSystem Ready. Watchdog re-enabled.");
}

void blynk_update(void) {
    // 1. Feed the Watchdog first so the system knows the CPU is alive
    esp_task_wdt_reset(); 

    // 2. Only run Blynk if we are actually connected to a network
    // This prevents the "lag" when you haven't entered WiFi/SIM credentials yet
    if (Blynk.connected()) {
        Blynk.run();
    }
}

// void blynk_sync_data(void) {
//     Blynk.virtualWrite(VPIN_VALIDATE, isPublicMode ? 1 : 0);
// }