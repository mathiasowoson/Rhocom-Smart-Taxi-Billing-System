#include "screens/ui_union.h"
#include "billing_logic.h" 
#include "cloudGsm_logic.h"

// --- 1. LOCAL DATABASE (Now inside UI for localized validation) ---
struct UnionMember {
    String id;
    String branch;
    String unionType;
    float fee;
};

static UnionMember unionDb[] = {
    {"100", "Ikeja", "Park Fee", 200.0},
    {"101", "Oshodi", "Park Fee", 200.0},
    {"200", "Lekki", "Maintenance", 500.0},
    {"300", "Ajah", "Checkpoint", 100.0},
    {"400", "Yaba", "Emergency", 150.0}
};
const int dbSize = sizeof(unionDb) / sizeof(unionDb[0]);

// --- 2. UI STATE & KEYPAD MAP ---
static String current_selected_union = "";
static String entered_id = "";
static bool is_keyboard_open = false;

// Button Map for Keypad: {x, y, w, h, label}
struct Key { int x; int y; const char* val; };
static Key keypad[12] = {
    {10, 130, "1"}, {115, 130, "2"}, {220, 130, "3"},
    {10, 165, "4"}, {115, 165, "5"}, {220, 165, "6"},
    {10, 200, "7"}, {115, 200, "8"}, {220, 200, "9"},
    {10, 235, "CLR"}, {115, 235, "0"}, {220, 235, "OK"} 
};

// --- 3. VALIDATION LOGIC ---
int validate_union_id_status(String inputId, String selectedUnion) {
    for (int i = 0; i < dbSize; i++) {
        if (unionDb[i].id == inputId) {
            if (selectedUnion.indexOf(unionDb[i].unionType) != -1) {
                dailyUnionTotal += unionDb[i].fee; // Global from billing_logic
                validCheckinsToday++;
                return 1; // Success
            } else {
                return 2; // Wrong Union
            }
        }
    }
    return 0; // Not Found
}

void show_status_msg(const char * msg, uint32_t color) {
    M5.Display.fillRoundRect(40, 100, 240, 60, 8, color);
    M5.Display.setTextColor(TFT_WHITE);
   M5.Display.setTextSize(1);              // Force size back to small
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.drawCenterString(msg, 160, 122);
    delay(1500);
    ui_union_init(); 
}

void draw_numeric_kb() {
    M5.Display.startWrite();
    M5.Display.fillRect(0, 80, 320, 160, TFT_BLACK);
    
    // Display Box
    M5.Display.drawRect(20, 85, 280, 35, TFT_WHITE);
    M5.Display.setTextColor(TFT_GREEN);
    M5.Display.setTextSize(1);              // Force size back to small
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.drawString("ID: " + entered_id, 30, 95);

    // Keys
    M5.Display.setTextColor(TFT_WHITE);
    for(int i=0; i<12; i++) {
        uint32_t btnCol = (i == 11) ? TFT_GREEN : (i == 9 ? TFT_RED : 0x3333);
        M5.Display.fillRoundRect(keypad[i].x, keypad[i].y, 95, 30, 4, btnCol);
        M5.Display.drawCenterString(keypad[i].val, keypad[i].x + 47, keypad[i].y + 7);
    }
    M5.Display.endWrite();
}

void ui_union_init(void) {
    is_keyboard_open = false;
    entered_id = "";
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextSize(1);              // Force size back to small
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    ui_create_header();

    M5.Display.drawCenterString("UNION VALIDATION", 160, 45);
    M5.Display.fillRoundRect(5, 40, 50, 35, 4, TFT_DARKGREY);
    M5.Display.drawCenterString("<", 30, 50);

    // List of Unions
    const char* labels[] = {"Park Fee (N200)", "Maintenance", "Checkpoint (N100)", "Emergency Levy"};
    for(int i=0; i<4; i++) {
        int y = 85 + (i * 38);
        M5.Display.fillRoundRect(10, y, 300, 32, 4, 0x1A1A);
        M5.Display.drawString(labels[i], 20, y + 8);
    }
}

void ui_union_handle_touch(m5::touch_detail_t &t) {
    // 1. Initial Guard: Only process when first pressed to prevent double-triggers
    if (!t.wasPressed()) return;

    // 2. State-Based Logic: Is the user typing or looking at the list?
    if (!is_keyboard_open) {
        
        // --- WIDGET: BACK BUTTON ---
        // Using your new navigation system instead of a raw callback
        if (t.x > 10 && t.x < 70 && t.y > 40 && t.y < 70) {
            M5.Speaker.tone(1000, 50); // Audio feedback
            ui_goto_page(UI_PAGE_DASHBOARD);
            return; // Exit early so we don't trigger list selection by mistake
        }

        // --- WIDGET: UNION LIST SELECTION ---
        if (t.y > 85 && t.y < 240) {
            int idx = (t.y - 85) / 38;
            const char* types[] = {"Park Fee", "Maintenance", "Checkpoint", "Emergency"};
            
            if (idx >= 0 && idx < 4) {
                current_selected_union = types[idx];
                is_keyboard_open = true;
                M5.Speaker.tone(1500, 50);
                draw_numeric_kb(); // This "opens" the keyboard overlay
            }
        }
    } 
    else {
        // --- WIDGET: NUMERIC KEYBOARD LOGIC ---
        // This only runs when is_keyboard_open is true
        for(int i = 0; i < 12; i++) {
            if (t.x > keypad[i].x && t.x < (keypad[i].x + 95) && 
                t.y > keypad[i].y && t.y < (keypad[i].y + 30)) {
                
                M5.Speaker.tone(2000, 20); // Quick click sound
                String val = keypad[i].val;

                if (val == "OK") {
                    int res = validate_union_id_status(entered_id, current_selected_union);
                    if(res == 1) show_status_msg("PAID SUCCESS!", TFT_GREEN);
                    else if(res == 2) show_status_msg("WRONG UNION", 0xEAA0); 
                    else show_status_msg("INVALID ID", TFT_RED);
                    
                    // Optional: Close keyboard after success? 
                    // is_keyboard_open = false; 
                    // ui_union_init(); 
                } 
                else if (val == "CLR") {
                    entered_id = "";
                } 
                else if (entered_id.length() < 6) {
                    entered_id += val;
                }
                
                // Refresh the display to show the new digits
                if (is_keyboard_open) draw_numeric_kb(); 
                break; 
            }
        }
    }
}