#ifndef CONFIG_H
#define CONFIG_H

#include <M5Unified.h>
#include <lvgl.h>

// --- System Info ---
#define COMPANY_NAME "RHOCOM"
#define CURRENCY "N"


#define BLYNK_TEMPLATE_ID "TMPLxxxxxx"
#define BLYNK_TEMPLATE_NAME "Rhocom"
#define BLYNK_AUTH_TOKEN "xxxxxxxxxx"

// --- 2. Network Enums & Flags ---
enum NetMode { MODE_WIFI, MODE_LTE };
extern NetMode currentNetMode;  // This tells other files it exists in main.cpp
extern bool isPublicMode;

// --- Blynk & Network Triggers ---
#define VPIN_ADD_TAG    V1  // Cloud trigger to add a passenger
#define VPIN_REVENUE    V2  // Send total earnings to cloud
#define VPIN_VALIDATE   V3  // Union ID status

// --- Passenger Data Structure ---
struct PassengerTag {
    int id;
    float startLat;
    float startLon;
    float currentFare;
    uint32_t startTime;
    bool isActive;
};

// --- Global Variables (Shared across all .cpp files) ---
extern PassengerTag tags[10];
extern int activeCount;
extern bool isPublicMode;
extern float gpsSpeed;
extern int batteryLevel;

#endif