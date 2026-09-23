#include "scherm_ui.h"
#include <Arduino.h>

// ==========================================
// GLOBALE SCHERM OBJECTEN DEFINIËREN
// ==========================================
lv_obj_t * scr_home = nullptr;
lv_obj_t * scr_lights = nullptr;
lv_obj_t * scr_climate_overview = nullptr;
lv_obj_t * scr_climate_detail = nullptr;
lv_obj_t * scr_music = nullptr;

// Globale UI label pointers voor Home Assistant data
lv_obj_t * lbl_temp_home = nullptr;
lv_obj_t * lbl_doel_home = nullptr;
lv_obj_t * slider_home = nullptr;
lv_obj_t * lbl_weather = nullptr;
lv_obj_t * lbl_flame_home = nullptr;

// ==========================================
// UI INITIALISATIE
// ==========================================
void setup_atag_style_ui() {
    // 1. Schermen direct en robuust aanmaken
    scr_home = lv_obj_create(NULL);
    scr_lights = lv_obj_create(NULL);
    scr_climate_overview = lv_obj_create(NULL);
    scr_climate_detail = lv_obj_create(NULL);
    scr_music = lv_obj_create(NULL);

    // 2. Bouw alle schermen op (elementen toevoegen)
    build_home_screen(scr_home);
    build_lights_screen(scr_lights);
    build_climate_overview(scr_climate_overview);
    build_climate_detail(scr_climate_detail);
    build_music_screen(scr_music);


    // 3. Start op het homescreen
    lv_scr_load(scr_home);
}

// ==========================================
// HOME ASSISTANT DATA SETTERS
// ==========================================
void ui_set_indoor_temp(int deg) {
    if (lbl_temp_home) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d °C", deg);
        lv_label_set_text(lbl_temp_home, buf);
    }
}

void ui_set_outdoor_temp(float deg) {
    if (lbl_weather) {
        char buf[24];
        snprintf(buf, sizeof(buf), LV_SYMBOL_IMAGE " %.1f°C", deg);
        lv_label_set_text(lbl_weather, buf);
    }
}

// Toont/verbergt de oranje vlam in de topbar op basis van
// de OTGW-vlamsensor (ketel brandt voor cv of warm water).
void ui_set_boiler_flame(bool active) {
    if (!lbl_flame_home) {
        return;
    }

    if (active) {
        lv_obj_clear_flag(lbl_flame_home, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(lbl_flame_home, LV_OBJ_FLAG_HIDDEN);
    }
}

// Werkt de thermostaat-widget op de voorpagina bij:
// grote temperatuur, doeltemperatuur en de slider (waarde ×10).
void ui_set_home_thermostat(float current_temp, float target_temp) {
    if (lbl_temp_home) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f°", current_temp);
        lv_label_set_text(lbl_temp_home, buf);
    }

    if (lbl_doel_home) {
        char buf[24];
        snprintf(buf, sizeof(buf), "Doel: %.1f°C", target_temp);
        lv_label_set_text(lbl_doel_home, buf);
    }

    if (slider_home) {
        lv_slider_set_value(
            slider_home,
            (int32_t)(target_temp * 10.0f),
            LV_ANIM_OFF
        );
    }
}

void ui_update_tile_state(int idx, const char* state) {
    // Logica voor tile status update
}

void ui_update_tile_value(int idx, const char* val) {
    // Logica voor tile waarde update
}

void ui_update_thermostat(ThermoState *s) {
    // Logica voor thermostaat update
}

void ui_set_ha_connected(bool connected) {
    Serial.printf("HA Connection status: %s\n", connected ? "Connected" : "Disconnected");
}

void ui_set_wifi_connected(bool connected) {
    Serial.printf("WiFi Connection status: %s\n", connected ? "Connected" : "Disconnected");
}

void ui_set_fan_state(bool on) {
    Serial.printf("Fan state: %s\n", on ? "ON" : "OFF");
}