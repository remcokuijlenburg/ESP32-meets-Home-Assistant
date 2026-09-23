#include <lvgl.h>
#include <stdint.h>
#include <string.h>
#include "scherm_ui.h"
#include "ha_client.h"
#include "ha_entities.h"

// ============================================================
// DATASTRUCTUUR
// ============================================================

#define MAX_ENTITIES_PER_ROOM 6

struct LightItem {
    const char* name;
    const char* entity_ids[MAX_ENTITIES_PER_ROOM];
    int entity_count;
    bool is_on;

    // UI referenties
    lv_obj_t* tile;
    lv_obj_t* lbl_name;
};

// ============================================================
// KAMERS — alleen kamers met daadwerkelijk gekoppelde
// HA-entiteiten (zie ha_entities.h). Kamers zonder entiteiten
// (Keuken, Boaz, Kai, Badkamer, Overloop, Zolder) verschijnen
// hier bewust niet totdat ze in ha_entities.h zijn toegevoegd.
// ============================================================

#define NUM_LIGHTS 6

static LightItem lights[NUM_LIGHTS] = {
    { "Woonkamer",   { HA_LIGHT_WOONKAMER_1,   HA_LIGHT_WOONKAMER_2 },                                                      2, false, nullptr, nullptr },
    { "Hal",         { HA_LIGHT_HAL_1,         HA_LIGHT_HAL_2 },                                                            2, false, nullptr, nullptr },
    { "Bijkeuken",   { HA_LIGHT_BIJKEUKEN_1 },                                                                              1, false, nullptr, nullptr },
    { "Tuinkantoor", { HA_LIGHT_TUINKANTOOR_1, HA_LIGHT_TUINKANTOOR_2 },                                                    2, false, nullptr, nullptr },
    { "Steeg",       { HA_LIGHT_STEEG_1,       HA_LIGHT_STEEG_2 },                                                          2, false, nullptr, nullptr },
    { "Tuin",        { HA_LIGHT_TUIN_1, HA_LIGHT_TUIN_2, HA_LIGHT_TUIN_3, HA_LIGHT_TUIN_4, HA_LIGHT_TUIN_5, HA_LIGHT_TUIN_6 }, 6, false, nullptr, nullptr }
};

// ============================================================
// KLEUREN
// ============================================================

static const lv_color_t COLOR_ON  = lv_color_hex(0x0099D8);
static const lv_color_t COLOR_OFF = lv_color_hex(0x222222);

// ============================================================
// HULPFUNCTIE — domein uit entity-ID halen
// (bv. "light.k_o_o" -> "light", "switch.smartplug_x" -> "switch")
// ============================================================

static void entity_domain(const char* entity_id, char* out, size_t out_size)
{
    const char* dot = strchr(entity_id, '.');
    size_t len = dot ? (size_t)(dot - entity_id) : strlen(entity_id);

    if (len >= out_size) {
        len = out_size - 1;
    }

    memcpy(out, entity_id, len);
    out[len] = '\0';
}

// ============================================================
// TEGEL BIJWERKEN
// ============================================================

static void update_light_tile(int idx)
{
    LightItem& item = lights[idx];

    if (item.tile == nullptr) {
        return;
    }

    lv_obj_set_style_bg_color(
        item.tile,
        item.is_on ? COLOR_ON : COLOR_OFF,
        0
    );

    // Toekomstige uitbreidingen kunnen hier komen:
    // - icoon veranderen
    // - aantal actieve lampen tonen
    // - animatie toevoegen
}

// ============================================================
// PUBLIEKE UPDATEFUNCTIE (VANUIT ha_client.cpp / sync_lights)
// ============================================================

void ui_set_light_state(const char* room_name, bool state)
{
    for (int i = 0; i < NUM_LIGHTS; i++) {

        if (strcmp(lights[i].name, room_name) == 0) {

            lights[i].is_on = state;

            update_light_tile(i);

            break;
        }
    }
}

// ============================================================
// TOGGLE CALLBACK
// ============================================================

static void light_btn_event_cb(lv_event_t * e)
{
    int idx =
        (int)(uintptr_t)lv_event_get_user_data(e);

    LightItem& item = lights[idx];

    bool new_state = !item.is_on;
    const char* service = new_state ? "turn_on" : "turn_off";

    // Alle lampen van deze kamer naar dezelfde eindstaat sturen
    for (int i = 0; i < item.entity_count; i++) {

        char domain[16];
        entity_domain(item.entity_ids[i], domain, sizeof(domain));

        ha_call_service(domain, service, item.entity_ids[i]);
    }

    // Optimistische UI-update — sync_lights() bevestigt dit
    // binnen 5 seconden met de echte HA-status.
    item.is_on = new_state;
    update_light_tile(idx);
}

// ============================================================
// TERUGKNOP
// ============================================================

static void back_btn_event_cb(lv_event_t * e)
{
    if (scr_home != NULL) {
        lv_scr_load(scr_home);
    }
}

// ============================================================
// SCHERM OPBOUW
// ============================================================

void build_lights_screen(lv_obj_t * parent)
{
    // --------------------------------------------------------
    // HOOFDCONTAINER
    // --------------------------------------------------------

    lv_obj_t * cont = lv_obj_create(parent);

    lv_obj_set_size(cont, 480, 480);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);

    lv_obj_set_style_bg_color(
        cont,
        lv_color_hex(0x000000),
        0
    );

    lv_obj_set_style_border_width(cont, 0, 0);

    // --------------------------------------------------------
    // TITEL
    // --------------------------------------------------------

    lv_obj_t * lbl_title = lv_label_create(cont);

    lv_label_set_text(lbl_title, "Verlichting");

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

    // --------------------------------------------------------
    // TERUGKNOP
    // --------------------------------------------------------

    lv_obj_t * btn_back = lv_btn_create(cont);

    lv_obj_set_size(btn_back, 80, 36);

    lv_obj_align(
        btn_back,
        LV_ALIGN_TOP_LEFT,
        10,
        8
    );

    lv_obj_set_style_bg_color(
        btn_back,
        lv_color_hex(0x333333),
        0
    );

    lv_obj_add_event_cb(
        btn_back,
        back_btn_event_cb,
        LV_EVENT_CLICKED,
        NULL
    );

    lv_obj_t * lbl_back = lv_label_create(btn_back);

    lv_label_set_text(
        lbl_back,
        LV_SYMBOL_LEFT " Terug"
    );

    lv_obj_center(lbl_back);

    // --------------------------------------------------------
    // GRID CONTAINER
    // --------------------------------------------------------

    lv_obj_t * grid_cont = lv_obj_create(cont);

    // Ruime marge onder de header (titel + terugknop), zodat de
    // eerste rij tegels niet meer tegen de terugknop aan zit.
    lv_obj_set_size(grid_cont, 440, 400);

    lv_obj_align(
        grid_cont,
        LV_ALIGN_TOP_MID,
        0,
        65
    );

    lv_obj_set_style_bg_color(
        grid_cont,
        lv_color_hex(0x000000),
        0
    );

    lv_obj_set_style_border_width(grid_cont, 0, 0);

    lv_obj_set_style_pad_all(grid_cont, 0, 0);

    lv_obj_set_flex_flow(
        grid_cont,
        LV_FLEX_FLOW_ROW_WRAP
    );

    lv_obj_set_style_pad_gap(grid_cont, 14, 18);

    // Tegels netjes verdelen: gecentreerd op de hoofdas (horizontaal)
    // en de rijen samen verticaal gecentreerd binnen de beschikbare ruimte.
    lv_obj_set_flex_align(
        grid_cont,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    // --------------------------------------------------------
    // TEGELS GENEREREN
    // --------------------------------------------------------

    for (int i = 0; i < NUM_LIGHTS; i++) {

        // Tegel

        lv_obj_t * tile = lv_btn_create(grid_cont);

        // Grotere tegels: 3 per rij, 2 rijen voor de 6 kamers
        lv_obj_set_size(tile, 136, 110);

        lv_obj_set_style_radius(tile, 14, 0);

        // UI referenties opslaan

        lights[i].tile = tile;

        // Event koppelen

        lv_obj_add_event_cb(
            tile,
            light_btn_event_cb,
            LV_EVENT_CLICKED,
            (void*)(uintptr_t)i
        );

        // Label

        lv_obj_t * lbl_name = lv_label_create(tile);

        lv_label_set_text(
            lbl_name,
            lights[i].name
        );

        lv_obj_set_style_text_font(
            lbl_name,
            &lv_font_montserrat_14,
            0
        );

        lv_obj_set_style_text_color(
            lbl_name,
            lv_color_hex(0xFFFFFF),
            0
        );

        lv_obj_center(lbl_name);

        lights[i].lbl_name = lbl_name;

        // Initiële status toepassen

        update_light_tile(i);
    }
}
