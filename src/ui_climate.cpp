#include <Arduino.h>
#include <lvgl.h>
#include <cstring>

#include "scherm_ui.h"
#include "ui_icons.h"
#include "ha_entities.h"
#include "ha_client.h"

// ==========================================
// DATASTRUCTUUR VOOR KAMERS
// ==========================================

struct ClimateRoom {
    const char* name;
    const char* entity_id;
    float current_temp;
    float target_temp;
    bool is_heating;
};

#define NUM_CLIMATE_ROOMS 2

static ClimateRoom climate_rooms[NUM_CLIMATE_ROOMS] = {
    {"Woonkamer",   HA_CLIMATE_WOONKAMER,   20.0f, 20.0f, false},
    {"Tuinkantoor", HA_CLIMATE_TUINKANTOOR, 20.0f, 20.0f, false}
};

// ==========================================
// ACTIEVE KAMER
// ==========================================

static int active_room_index = 0;

// ==========================================
// DIRECTE REFERENTIES NAAR UI OBJECTEN
// ==========================================

// Klimaat tegels
static lv_obj_t* climate_tiles[NUM_CLIMATE_ROOMS] = {
    NULL, NULL
};

// Labels op de tegels
static lv_obj_t* climate_current_labels[NUM_CLIMATE_ROOMS] = {
    NULL, NULL
};

static lv_obj_t* climate_target_labels[NUM_CLIMATE_ROOMS] = {
    NULL, NULL
};

// Verwarmingsiconen
static lv_obj_t* climate_heat_icons[NUM_CLIMATE_ROOMS] = {
    NULL, NULL
};

// ==========================================
// UI OBJECTEN DETAIL SCHERM
// ==========================================

static lv_obj_t* lbl_detail_title   = NULL;
static lv_obj_t* lbl_detail_current = NULL;
static lv_obj_t* lbl_detail_target  = NULL;
static lv_obj_t* slider_detail      = NULL;

// ==========================================
// FUNCTIEPROTOTYPES
// ==========================================

static void update_climate_tile(int room_idx);
static void load_climate_detail(int room_idx);

// ==========================================
// NAVIGATIE CALLBACKS
// ==========================================

static void overview_back_btn_cb(lv_event_t* e)
{
    if (scr_home != NULL) {
        lv_scr_load(scr_home);
    }
}

// ------------------------------------------

static void detail_back_btn_cb(lv_event_t* e)
{
    if (scr_climate_overview != NULL) {
        lv_scr_load(scr_climate_overview);
    }
}

// ==========================================
// SLIDER CALLBACK
// ==========================================

static void detail_slider_event_cb(lv_event_t* e)
{
    lv_obj_t* slider = lv_event_get_target(e);

    int value = lv_slider_get_value(slider);

    float new_target = value / 10.0f;

    // ----------------------------------------
    // Data bijwerken
    // ----------------------------------------

    climate_rooms[active_room_index].target_temp = new_target;

    climate_rooms[active_room_index].is_heating =
        (new_target >
         climate_rooms[active_room_index].current_temp);

    // ----------------------------------------
    // Detail doeltemperatuur bijwerken
    // ----------------------------------------

    if (lbl_detail_target != NULL) {

        lv_label_set_text_fmt(
            lbl_detail_target,
            "Doel: %.1f°C",
            new_target
        );
    }

    // ----------------------------------------
    // Slider kleur
    // ----------------------------------------

    lv_color_t color =
        climate_rooms[active_room_index].is_heating
            ? lv_color_hex(0xFF6600)
            : lv_color_hex(0x0099D8);

    lv_obj_set_style_bg_color(
        slider,
        color,
        LV_PART_INDICATOR
    );

    lv_obj_set_style_bg_color(
        slider,
        color,
        LV_PART_KNOB
    );

    // ----------------------------------------
    // Overzicht tegel direct bijwerken
    // ----------------------------------------

    update_climate_tile(active_room_index);
}

// ==========================================
// SLIDER LOSGELATEN — COMMANDO NAAR HOME ASSISTANT
// ==========================================
// Wordt pas bij loslaten verstuurd (niet tijdens het slepen)
// om de UI niet te blokkeren met herhaalde HTTP-aanroepen.

static void detail_slider_released_cb(lv_event_t* e)
{
    lv_obj_t* slider = lv_event_get_target(e);

    int value = lv_slider_get_value(slider);

    float new_target = value / 10.0f;

    char extra[32];

    snprintf(
        extra,
        sizeof(extra),
        "\"temperature\":%.1f",
        new_target
    );

    ha_call_service(
        "climate",
        "set_temperature",
        climate_rooms[active_room_index].entity_id,
        extra
    );
}

// ==========================================
// KAMER TILE CALLBACK
// ==========================================

static void room_tile_event_cb(lv_event_t* e)
{
    int room_idx =
        (int)(uintptr_t)lv_event_get_user_data(e);

    if (room_idx < 0 ||
        room_idx >= NUM_CLIMATE_ROOMS) {

        return;
    }

    active_room_index = room_idx;

    // Detailgegevens laden
    load_climate_detail(room_idx);

    // Naar detail scherm
    if (scr_climate_detail != NULL) {

        lv_scr_load(scr_climate_detail);
    }
}

// ==========================================
// DETAIL SCHERM LADEN
// ==========================================

static void load_climate_detail(int room_idx)
{
    if (room_idx < 0 ||
        room_idx >= NUM_CLIMATE_ROOMS) {

        return;
    }

    ClimateRoom& room =
        climate_rooms[room_idx];

    // ----------------------------------------
    // Titel
    // ----------------------------------------

    if (lbl_detail_title != NULL) {

        lv_label_set_text(
            lbl_detail_title,
            room.name
        );
    }

    // ----------------------------------------
    // Huidige temperatuur
    // ----------------------------------------

    if (lbl_detail_current != NULL) {

        lv_label_set_text_fmt(
            lbl_detail_current,
            "Huidig: %.1f°C",
            room.current_temp
        );
    }

    // ----------------------------------------
    // Doeltemperatuur
    // ----------------------------------------

    if (lbl_detail_target != NULL) {

        lv_label_set_text_fmt(
            lbl_detail_target,
            "Doel: %.1f°C",
            room.target_temp
        );
    }

    // ----------------------------------------
    // Slider
    // ----------------------------------------

    if (slider_detail != NULL) {

        lv_slider_set_value(
            slider_detail,
            (int32_t)(room.target_temp * 10.0f),
            LV_ANIM_OFF
        );

        lv_color_t color =
            room.is_heating
                ? lv_color_hex(0xFF6600)
                : lv_color_hex(0x0099D8);

        lv_obj_set_style_bg_color(
            slider_detail,
            color,
            LV_PART_INDICATOR
        );

        lv_obj_set_style_bg_color(
            slider_detail,
            color,
            LV_PART_KNOB
        );
    }
}

// ==========================================
// KLIMAAT TEGEL BIJWERKEN
// ==========================================

static void update_climate_tile(int room_idx)
{
    if (room_idx < 0 ||
        room_idx >= NUM_CLIMATE_ROOMS) {

        return;
    }

    Serial.print(
        "[UI] Update klimaat tegel index: "
    );

    Serial.println(room_idx);

    // ----------------------------------------
    // Controleer of de UI-objecten bestaan
    // ----------------------------------------

    if (climate_tiles[room_idx] == NULL) {

        Serial.println(
            "[UI] Klimaat tegel bestaat nog niet"
        );

        return;
    }

    // ----------------------------------------
    // Huidige temperatuur
    // ----------------------------------------

    if (climate_current_labels[room_idx] != NULL) {

        lv_label_set_text_fmt(
            climate_current_labels[room_idx],
            "%.1f°C",
            climate_rooms[room_idx].current_temp
        );
    }

    // ----------------------------------------
    // Doeltemperatuur
    // ----------------------------------------

    if (climate_target_labels[room_idx] != NULL) {

        lv_label_set_text_fmt(
            climate_target_labels[room_idx],
            "Doel: %.1f°C",
            climate_rooms[room_idx].target_temp
        );
    }

    // ----------------------------------------
    // Verwarmingsicoon
    // ----------------------------------------

    if (climate_rooms[room_idx].is_heating) {

        if (climate_heat_icons[room_idx] == NULL) {

            // Icoon bestaat nog niet → aanmaken

            climate_heat_icons[room_idx] =
                lv_label_create(
                    climate_tiles[room_idx]
                );

            lv_obj_set_style_text_font(
                climate_heat_icons[room_idx],
                &Custom_Icons_32,
                0
            );

            lv_label_set_text(
                climate_heat_icons[room_idx],
                ICON_FIRE
            );

            lv_obj_set_style_text_color(
                climate_heat_icons[room_idx],
                lv_color_hex(0xFF6600),
                0
            );

            lv_obj_align(
                climate_heat_icons[room_idx],
                LV_ALIGN_BOTTOM_RIGHT,
                -5,
                -5
            );

        } else {

            // Icoon bestaat al → zichtbaar maken

            lv_obj_clear_flag(
                climate_heat_icons[room_idx],
                LV_OBJ_FLAG_HIDDEN
            );
        }

    } else {

        // ------------------------------------
        // Verwarming uit
        // ------------------------------------

        if (climate_heat_icons[room_idx] != NULL) {

            lv_obj_add_flag(
                climate_heat_icons[room_idx],
                LV_OBJ_FLAG_HIDDEN
            );
        }
    }
}

// ==========================================
// HOME ASSISTANT → UI
// ==========================================

void ui_set_climate_state(
    const char* room_name,
    float current_temp,
    float target_temp,
    bool heating
)
{
    Serial.print(
        "[UI] Update climate voor: "
    );

    Serial.println(room_name);

    // ----------------------------------------
    // Zoek juiste kamer
    // ----------------------------------------

    for (int i = 0;
         i < NUM_CLIMATE_ROOMS;
         i++) {

        Serial.print(
            "[UI] Vergelijk met: "
        );

        Serial.println(
            climate_rooms[i].name
        );

        if (strcmp(
                climate_rooms[i].name,
                room_name
            ) == 0) {

            Serial.println(
                "[UI] Match gevonden"
            );

            // --------------------------------
            // Data bijwerken
            // --------------------------------

            climate_rooms[i].current_temp =
                current_temp;

            climate_rooms[i].target_temp =
                target_temp;

            climate_rooms[i].is_heating =
                heating;

            Serial.print(
                "[UI] Nieuwe huidige temperatuur: "
            );

            Serial.println(
                climate_rooms[i].current_temp,
                1
            );

            Serial.print(
                "[UI] Nieuwe doeltemperatuur: "
            );

            Serial.println(
                climate_rooms[i].target_temp,
                1
            );

            Serial.print(
                "[UI] Verwarming: "
            );

            Serial.println(
                climate_rooms[i].is_heating
                    ? "JA"
                    : "NEE"
            );

            // --------------------------------
            // Overzicht direct bijwerken
            // --------------------------------

            if (climate_tiles[i] != NULL) {

                Serial.println(
                    "[UI] Update overzicht tegel"
                );

                update_climate_tile(i);

            } else {

                Serial.println(
                    "[UI] Klimaat tegel nog niet aangemaakt"
                );
            }

            // --------------------------------
            // Detailpagina bijwerken
            // --------------------------------

            if (i == active_room_index) {

                Serial.println(
                    "[UI] Actieve kamer = bijgewerkt"
                );

                if (scr_climate_detail != NULL) {

                    Serial.println(
                        "[UI] Update klimaat detail"
                    );

                    load_climate_detail(i);

                } else {

                    Serial.println(
                        "[UI] Klimaat detail bestaat nog niet"
                    );
                }
            }

            return;
        }
    }

    // ----------------------------------------
    // Geen kamer gevonden
    // ----------------------------------------

    Serial.print(
        "[UI] GEEN MATCH gevonden voor: "
    );

    Serial.println(room_name);
}

// ==========================================
// KLIMAAT OVERZICHT OPBOUWEN
// ==========================================

void build_climate_overview(lv_obj_t* parent)
{
    // ----------------------------------------
    // Oude referenties resetten
    // ----------------------------------------

    for (int i = 0;
         i < NUM_CLIMATE_ROOMS;
         i++) {

        climate_tiles[i] = NULL;
        climate_current_labels[i] = NULL;
        climate_target_labels[i] = NULL;
        climate_heat_icons[i] = NULL;
    }

    // ----------------------------------------
    // Container
    // ----------------------------------------

    lv_obj_t* cont =
        lv_obj_create(parent);

    lv_obj_set_size(
        cont,
        480,
        480
    );

    lv_obj_align(
        cont,
        LV_ALIGN_CENTER,
        0,
        0
    );

    lv_obj_set_style_bg_color(
        cont,
        lv_color_hex(0x000000),
        0
    );

    lv_obj_set_style_border_width(
        cont,
        0,
        0
    );

    // ----------------------------------------
    // Terug knop
    // ----------------------------------------

    lv_obj_t* btn_back =
        lv_btn_create(cont);

    lv_obj_set_size(
        btn_back,
        80,
        38
    );

    lv_obj_align(
        btn_back,
        LV_ALIGN_TOP_LEFT,
        10,
        10
    );

    lv_obj_set_style_bg_color(
        btn_back,
        lv_color_hex(0x333333),
        0
    );

    lv_obj_add_event_cb(
        btn_back,
        overview_back_btn_cb,
        LV_EVENT_CLICKED,
        NULL
    );

    lv_obj_t* lbl_back =
        lv_label_create(btn_back);

    lv_label_set_text(
        lbl_back,
        LV_SYMBOL_LEFT " Terug"
    );

    lv_obj_center(lbl_back);

    // ----------------------------------------
    // Titel
    // ----------------------------------------

    lv_obj_t* lbl_title =
        lv_label_create(cont);

    lv_label_set_text(
        lbl_title,
        "Klimaat Overzicht"
    );

    lv_obj_set_style_text_font(
        lbl_title,
        &lv_font_montserrat_20,
        0
    );

    lv_obj_set_style_text_color(
        lbl_title,
        lv_color_hex(0xFFFFFF),
        0
    );

    lv_obj_align(
        lbl_title,
        LV_ALIGN_TOP_MID,
        0,
        15
    );

    // ----------------------------------------
    // Grid
    // ----------------------------------------

    lv_obj_t* grid =
        lv_obj_create(cont);

    lv_obj_set_size(
        grid,
        460,
        400
    );

    lv_obj_align(
        grid,
        LV_ALIGN_BOTTOM_MID,
        0,
        -5
    );

    lv_obj_set_style_bg_color(
        grid,
        lv_color_hex(0x000000),
        0
    );

    lv_obj_set_style_border_width(
        grid,
        0,
        0
    );

    lv_obj_set_flex_flow(
        grid,
        LV_FLEX_FLOW_ROW_WRAP
    );

    lv_obj_set_style_pad_all(
        grid,
        5,
        0
    );

    lv_obj_set_style_pad_gap(
        grid,
        10,
        10
    );

    // ----------------------------------------
    // Kamertegels
    // ----------------------------------------

    for (int i = 0;
         i < NUM_CLIMATE_ROOMS;
         i++) {

        lv_obj_t* tile =
            lv_btn_create(grid);

        climate_tiles[i] = tile;

        lv_obj_set_size(
            tile,
            220,
            160
        );

        lv_obj_set_style_bg_color(
            tile,
            lv_color_hex(0x222222),
            0
        );

        lv_obj_add_event_cb(
            tile,
            room_tile_event_cb,
            LV_EVENT_CLICKED,
            (void*)(uintptr_t)i
        );

        // ------------------------------------
        // Kamernaam
        // ------------------------------------

        lv_obj_t* lbl_name =
            lv_label_create(tile);

        lv_label_set_text(
            lbl_name,
            climate_rooms[i].name
        );

        lv_obj_align(
            lbl_name,
            LV_ALIGN_TOP_LEFT,
            5,
            5
        );

        lv_obj_set_style_text_color(
            lbl_name,
            lv_color_hex(0xAAAAAA),
            0
        );

        // ------------------------------------
        // Huidige temperatuur
        // ------------------------------------

        lv_obj_t* lbl_cur =
            lv_label_create(tile);

        climate_current_labels[i] =
            lbl_cur;

        lv_label_set_text_fmt(
            lbl_cur,
            "%.1f°C",
            climate_rooms[i].current_temp
        );

        lv_obj_set_style_text_font(
            lbl_cur,
            &lv_font_montserrat_32,
            0
        );

        lv_obj_set_style_text_color(
            lbl_cur,
            lv_color_hex(0xFFFFFF),
            0
        );

        lv_obj_align(
            lbl_cur,
            LV_ALIGN_CENTER,
            0,
            -10
        );

        // ------------------------------------
        // Doeltemperatuur
        // ------------------------------------

        lv_obj_t* lbl_tgt =
            lv_label_create(tile);

        climate_target_labels[i] =
            lbl_tgt;

        lv_label_set_text_fmt(
            lbl_tgt,
            "Doel: %.1f°C",
            climate_rooms[i].target_temp
        );

        lv_obj_set_style_text_color(
            lbl_tgt,
            lv_color_hex(0xCCCCCC),
            0
        );

        lv_obj_align(
            lbl_tgt,
            LV_ALIGN_BOTTOM_LEFT,
            5,
            -5
        );

        // ------------------------------------
        // Verwarmingsicoon
        // ------------------------------------

        if (climate_rooms[i].is_heating) {

            lv_obj_t* lbl_heat =
                lv_label_create(tile);

            climate_heat_icons[i] =
                lbl_heat;

            lv_obj_set_style_text_font(
                lbl_heat,
                &Custom_Icons_32,
                0
            );

            lv_label_set_text(
                lbl_heat,
                ICON_FIRE
            );

            lv_obj_set_style_text_color(
                lbl_heat,
                lv_color_hex(0xFF6600),
                0
            );

            lv_obj_align(
                lbl_heat,
                LV_ALIGN_BOTTOM_RIGHT,
                -5,
                -5
            );
        }
    }
}

// ==========================================
// KLIMAAT DETAIL OPBOUWEN
// ==========================================

void build_climate_detail(lv_obj_t* parent)
{
    // ----------------------------------------
    // Container
    // ----------------------------------------

    lv_obj_t* cont =
        lv_obj_create(parent);

    lv_obj_set_size(
        cont,
        480,
        480
    );

    lv_obj_align(
        cont,
        LV_ALIGN_CENTER,
        0,
        0
    );

    lv_obj_set_style_bg_color(
        cont,
        lv_color_hex(0x000000),
        0
    );

    lv_obj_set_style_border_width(
        cont,
        0,
        0
    );

    // ----------------------------------------
    // Terug knop
    // ----------------------------------------

    lv_obj_t* btn_back =
        lv_btn_create(cont);

    lv_obj_set_size(
        btn_back,
        80,
        38
    );

    lv_obj_align(
        btn_back,
        LV_ALIGN_TOP_LEFT,
        10,
        10
    );

    lv_obj_set_style_bg_color(
        btn_back,
        lv_color_hex(0x333333),
        0
    );

    lv_obj_add_event_cb(
        btn_back,
        detail_back_btn_cb,
        LV_EVENT_CLICKED,
        NULL
    );

    lv_obj_t* lbl_back =
        lv_label_create(btn_back);

    lv_label_set_text(
        lbl_back,
        LV_SYMBOL_LEFT " Terug"
    );

    lv_obj_center(lbl_back);

    // ----------------------------------------
    // Titel
    // ----------------------------------------

    lbl_detail_title =
        lv_label_create(cont);

    lv_label_set_text(
        lbl_detail_title,
        "Kamer"
    );

    lv_obj_set_style_text_font(
        lbl_detail_title,
        &lv_font_montserrat_24,
        0
    );

    lv_obj_set_style_text_color(
        lbl_detail_title,
        lv_color_hex(0xFFFFFF),
        0
    );

    lv_obj_align(
        lbl_detail_title,
        LV_ALIGN_TOP_MID,
        0,
        15
    );

    // ----------------------------------------
    // Huidige temperatuur
    // ----------------------------------------

    lbl_detail_current =
        lv_label_create(cont);

    lv_label_set_text(
        lbl_detail_current,
        "Huidig: --.-°C"
    );

    lv_obj_set_style_text_color(
        lbl_detail_current,
        lv_color_hex(0xAAAAAA),
        0
    );

    lv_obj_set_style_text_font(
        lbl_detail_current,
        &lv_font_montserrat_24,
        0
    );

    lv_obj_align(
        lbl_detail_current,
        LV_ALIGN_CENTER,
        0,
        -60
    );

    // ----------------------------------------
    // Doeltemperatuur
    // ----------------------------------------

    lbl_detail_target =
        lv_label_create(cont);

    lv_label_set_text(
        lbl_detail_target,
        "Doel: --.-°C"
    );

    lv_obj_set_style_text_font(
        lbl_detail_target,
        &lv_font_montserrat_32,
        0
    );

    lv_obj_set_style_text_color(
        lbl_detail_target,
        lv_color_hex(0xFFFFFF),
        0
    );

    lv_obj_align(
        lbl_detail_target,
        LV_ALIGN_CENTER,
        0,
        10
    );

    // ----------------------------------------
    // Slider
    // ----------------------------------------

    slider_detail =
        lv_slider_create(cont);

    lv_obj_set_size(
        slider_detail,
        380,
        50
    );

    lv_obj_align(
        slider_detail,
        LV_ALIGN_BOTTOM_MID,
        0,
        -40
    );

    lv_slider_set_range(
        slider_detail,
        150,
        300
    );

    lv_slider_set_value(
        slider_detail,
        200,
        LV_ANIM_OFF
    );

    lv_obj_add_event_cb(
        slider_detail,
        detail_slider_event_cb,
        LV_EVENT_VALUE_CHANGED,
        NULL
    );

    lv_obj_add_event_cb(
        slider_detail,
        detail_slider_released_cb,
        LV_EVENT_RELEASED,
        NULL
    );
}