#ifndef SCHERM_UI_H
#define SCHERM_UI_H

#include <lvgl.h>
#include <stddef.h>
#include <stdint.h>

#ifndef THERMO_STATE_DEFINED
#define THERMO_STATE_DEFINED
struct ThermoState {
    char mode[16];
    char action[16];
    bool available;
    bool dual;
    int current;
    int target;
    int low;
    int high;
};
#endif

// Globale scherm pointers
extern lv_obj_t * scr_home;
extern lv_obj_t * scr_lights;
extern lv_obj_t * scr_climate_overview;
extern lv_obj_t * scr_climate_detail;
extern lv_obj_t * scr_music;

// UI Label pointers voor Home Assistant data
extern lv_obj_t * lbl_temp_home;
extern lv_obj_t * lbl_doel_home;
extern lv_obj_t * slider_home;
extern lv_obj_t * lbl_weather;
extern lv_obj_t * lbl_flame_home;

// Custom fonts & iconen
LV_FONT_DECLARE(Custom_Icons_32);

#ifndef ICON_FIRE
#endif

// ============================================================
// UI INITIALISATIE FUNCTIES
// ============================================================

void setup_atag_style_ui();

void build_home_screen(lv_obj_t * parent);
void build_lights_screen(lv_obj_t * parent);
void build_climate_overview(lv_obj_t * parent);
void build_climate_detail(lv_obj_t * parent);
void build_music_screen(lv_obj_t * parent);

// ============================================================
// HOME ASSISTANT DATA SETTERS (temeratura, staat, enz.)
// ============================================================

void ui_set_indoor_temp(int deg);
void ui_set_home_thermostat(float current_temp, float target_temp);
void ui_set_outdoor_temp(float deg);
void ui_set_boiler_flame(bool active);

// ============================================================
// TILE UPDATE FUNCTIES (met index validatie)
// ============================================================

// Update tile state (idx: 0-8, één per tile op huidi scherm)
// Includes null check en bounds validation
void ui_update_tile_state(int idx, const char* state);

// Update tile waarde (bijv. brightness % of fan speed %)
// Includes null check en bounds validation
void ui_update_tile_value(int idx, const char* val);

// Update thermostaat-details voor detail-scherm
// Includes nullptr check
void ui_update_thermostat(ThermoState *s);

// ============================================================
// VERBINDINGSSTATUS SETTERS
// ============================================================

void ui_set_ha_connected(bool connected);
void ui_set_wifi_connected(bool connected);
void ui_set_fan_state(bool on);

// ============================================================
// UI ENTITY SETTERS (verlichting, TV, klimaat, muziek)
// ============================================================

// Verlichting
void ui_set_light_state(
    const char* room_name,
    bool state
);

// TV (aan/uit-knop op het beginscherm)
void ui_set_tv_state(bool on);

// Klimaat
// is_on: hvac_mode van de entiteit is niet "off" (kachel/thermostaat staat aan)
// heating: er wordt op dit moment daadwerkelijk warmte geleverd (vlam/element actief)
void ui_set_climate_state(
    const char* room_name,
    float current_temp,
    float target_temp,
    bool heating,
    bool is_on
);

// Muziek
// idx = index in MUSIC_PLAYERS (ha_entities.h)
// volume_pct: 0-100, of -1 als onbekend/niet ondersteund
// src0..src2: namen uit het source_list attribuut van HA (max 3), "" = leeg slot
void ui_set_music_player(
    int idx,
    bool available,
    bool playing,
    int volume_pct,
    const char* title,
    const char* artist,
    bool grouped,
    const char* src0,
    const char* src1,
    const char* src2
);

#endif // SCHERM_UI_H
