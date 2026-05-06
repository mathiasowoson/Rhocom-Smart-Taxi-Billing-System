#include "screens/ui_union.h"
#include "billing_logic.h" 
#include "cloudGsm_logic.h"
#include "ui_manager.h"

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
    {10, 125, "1"}, {115, 125, "2"}, {220, 125, "3"},
    {10, 155, "4"}, {115, 155, "5"}, {220, 155, "6"},
    {10, 185, "7"}, {115, 185, "8"}, {220, 185, "9"},
    {10, 215, "CLR"}, {115, 215, "0"}, {220, 215, "OK"} 
};

void ui_union_init(void) {
    is_keyboard_open = false;
    entered_id = "";
    
    M5.Display.startWrite();
    M5.Display.fillScreen(TFT_BLACK);
    
    // Header and Static Title
    ui_create_header();
    M5.Display.setFont(NULL);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.drawCenterString("UNION VALIDATION", 160, 45);

    // 1. Clear the scrollable list area
    M5.Display.fillRect(0, 80, 275, 160, TFT_BLACK);

    // 2. List of Unions (WITH SCROLLING)
    const char* labels[] = {"Park Fee (N200)", "Maintenance", "Checkpoint (N100)", "Emergency Levy", "Local Tax", "State Levy"};
    M5.Display.setFont(&fonts::FreeSans9pt7b); // Reset font to prevent "Giant Text" bug
    
    for(int i = 0; i < 6; i++) {
        int draw_y = 85 + (i * 38) - ui_scroll_offset;
        
        // Only draw if within the window
        if (draw_y >= 80 && draw_y < 230) {
            draw_scrollable_button(10, draw_y, 260, 32, labels[i], 0x1A1A);
        }
    }

    // 3. Scroll Controls (Blue Arrows)
    M5.Display.setFont(&fonts::FreeSans12pt7b);
    M5.Display.fillRoundRect(280, 85, 35, 60, 4, TFT_BLUE);
    M5.Display.drawCenterString("^", 297, 105);
    
    M5.Display.fillRoundRect(280, 160, 35, 60, 4, TFT_BLUE);
    M5.Display.drawCenterString("V", 297, 180);

    // 4. Back Button
    M5.Display.fillRoundRect(5, 40, 50, 35, 4, TFT_DARKGREY);
    M5.Display.drawCenterString("<", 30, 50);
    M5.Display.endWrite();
}

void draw_numeric_kb() {
    M5.Display.startWrite();
    // Modal background (dimmed)
    M5.Display.fillRect(0, 0, 320, 240, M5.Display.color565(15, 15, 15));
    
    // Display Box for the ID
    M5.Display.drawRect(20, 85, 280, 35, TFT_WHITE);
    M5.Display.setTextColor(TFT_GREEN);
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.drawString("ID: " + entered_id, 30, 95);

    // Draw the compact keys
    for(int i=0; i<12; i++) {
        uint32_t btnCol = (i == 11) ? TFT_GREEN : (i == 9 ? TFT_RED : 0x3333);
        M5.Display.fillRoundRect(keypad[i].x, keypad[i].y, 95, 28, 4, btnCol);
        M5.Display.setTextColor(TFT_WHITE);
        M5.Display.drawCenterString(keypad[i].val, keypad[i].x + 47, keypad[i].y + 5);
    }
    M5.Display.endWrite();
}

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

void ui_union_handle_touch(m5::touch_detail_t &t) {
    if (!t.wasPressed()) return;

    // --- ZONE 1: KEYBOARD OVERLAY (HIGHEST PRIORITY) ---
    if (is_keyboard_open) {
        for(int i = 0; i < 12; i++) {
            if (t.x > keypad[i].x && t.x < (keypad[i].x + 95) && 
                t.y > keypad[i].y && t.y < (keypad[i].y + 30)) {
                
                M5.Speaker.tone(2000, 20);
                String val = keypad[i].val;

                if (val == "OK") {
                    int res = validate_union_id_status(entered_id, current_selected_union);
                    if(res == 1) show_status_msg("PAID SUCCESS!", TFT_GREEN);
                    else if(res == 2) show_status_msg("WRONG UNION", 0xEAA0); 
                    else show_status_msg("INVALID ID", TFT_RED);
                } 
                else if (val == "CLR") { entered_id = ""; draw_numeric_kb(); } 
                else if (entered_id.length() < 6) { entered_id += val; draw_numeric_kb(); }
                return; 
            }
        }
        return; // Block background touches when KB is open
    }

    // --- ZONE 2: STATIC BACK BUTTON ---
    if (t.x > 5 && t.x < 60 && t.y > 40 && t.y < 75) {
        M5.Speaker.tone(1000, 50);
        ui_goto_page(UI_PAGE_DASHBOARD);
        return;
    }

    // --- ZONE 3: SCROLL BUTTONS ---
    if (t.x > 275) {
        if (t.y > 85 && t.y < 145) { // Up
            ui_scroll_offset -= 38;
            if (ui_scroll_offset < 0) ui_scroll_offset = 0;
            ui_union_init(); 
        } 
        else if (t.y > 160 && t.y < 220) { // Down
            ui_scroll_offset += 38;
            ui_union_init();
        }
        return;
    }

    // --- ZONE 4: LIST ITEMS ---
    if (t.x < 270 && t.y > 80) {
        int virtual_y = t.y + ui_scroll_offset;
        int idx = (virtual_y - 85) / 38;
        
        const char* types[] = {"Park Fee", "Maintenance", "Checkpoint", "Emergency", "Tax", "Levy"};
        if (idx >= 0 && idx < 6) {
            current_selected_union = types[idx];
            is_keyboard_open = true;
            M5.Speaker.tone(1500, 50);
            draw_numeric_kb(); 
        }
    }
}