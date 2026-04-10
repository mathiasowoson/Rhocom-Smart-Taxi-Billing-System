#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#include <M5Unified.h>
#include <lvgl.h>

// --- System Info ---
#define COMPANY_NAME "RHOCOM"
#define CURRENCY "N"

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
extern bool isPublicMode;


#endif