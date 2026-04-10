#ifndef BLYNK_GSM_LOGIC_H
#define BLYNK_GSM_LOGIC_H

// --- Virtual Pins ---
// These are safe to keep in the header as they are just reference numbers
#define VPIN_ADD_TAG           V1
#define VPIN_TOTAL_REVENUE     V2
#define VPIN_DRIVER_NET        V4
#define VPIN_PASSENGER_REVENUE V5
#define VPIN_UNION_TOTAL       V7
#define VPIN_TODAYCHECKIN      V9
#define VPIN_FUEL_PRICE        V10
#define VPIN_KMLEFFICIENCY     V11

// Functions exposed to the rest of the project
void blynk_gsm_setup(void);
void blynk_gsm_update(void);
void blynk_gsm_sync(void);

#endif