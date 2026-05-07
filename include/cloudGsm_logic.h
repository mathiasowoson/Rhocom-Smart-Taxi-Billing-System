#ifndef CLOUD_GSM_LOGIC_H
#define CLOUD_GSM_LOGIC_H

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

extern SemaphoreHandle_t xSerialSemaphore;

// ThingSpeak Configuration
// #define SECRET_CH_ID 1234567          // Replace with your Channel ID
// #define SECRET_WRITE_APIKEY "XYZ123"  // Replace with your Write API Key
// #define SECRET_TALKBACK_ID "00000"    // Replace with your TalkBack ID
// #define SECRET_TALKBACK_KEY "AAAAA"   // Replace with your TalkBack API Key

void cloud_gsm_setup(void);
void cloud_gsm_sync(void); // This will handle both Upload and TalkBack Check

#endif