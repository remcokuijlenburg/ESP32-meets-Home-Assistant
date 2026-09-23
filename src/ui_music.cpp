#include <lvgl.h>
#include <stdint.h>
#include <string.h>

#include "scherm_ui.h"
#include "ui_icons.h"

// ============================================================
// DATASTRUCTUUR
// ============================================================

struct SonosPlayer {
    const char* name;

    bool is_playing;

    // UI referenties
    lv_obj_t* card;
    lv_obj_t* lbl_status;
    lv_obj_t* lbl_icon;
};

// ============================================================
// DUMMY DATA
// ============================================================

#define NUM_SONOS_PLAYERS 2

static SonosPlayer players[NUM_SONOS_PLAYERS] = {
    {"Woonkamer Sonos", true,  nullptr, nullptr, nullptr},
    {"Keuken Sonos",    false, nullptr, nullptr, nullptr}
};

// ============================================================
// KAART BIJWERKEN
// ============================================================

static void update_player_card(int idx)
{
    SonosPlayer& player = players[idx];

    if (player.lbl_status != nullptr) {

        lv_label_set_text(
            player.lbl_status,
            player.is_playing
                ? "Status: Actief"
                : "Status: Gestopt"
        );
    }

    if (player.lbl_icon != nullptr) {

        lv_label_set_text(
            player.lbl_icon,
            player.is_playing ? LV_SYMBOL_PLAY : LV_SYMBOL_STOP
        );

        lv_obj_set_style_text_color(
            player.lbl_icon,
            player.is_playing
                ? lv_color_hex(0x00CC66)
                : lv_color_hex(0xAAAAAA),
            0
        );
    }

    if (player.card != nullptr) {

        lv_obj_set_style_bg_color(
            player.card,
            player.is_playing
                ? lv_color_hex(0x1A2A1A)
                : lv_color_hex(0x222222),
            0
        );
    }
}

// ============================================================
// PUBLIEKE UPDATEFUNCTIE (VOORBEREIDING HA)
// ============================================================

void ui_set_music_state(const char* player_name, bool playing)
{
    for (int i = 0; i < NUM_SONOS_PLAYERS; i++) {

        if (strcmp(players[i].name, player_name) == 0) {

            players[i].is_playing = playing;

            update_player_card(i);

            break;
        }
    }
}

// ============================================================
// CALLBACKS
// ============================================================

static void back_btn_event_cb(lv_event_t * e)
{
    if (scr_home != nullptr) {

        lv_scr_load(scr_home);
    }
}

static void stop_btn_event_cb(lv_event_t * e)
{
    int idx =
        (int)(uintptr_t)lv_event_get_user_data(e);

    players[idx].is_playing = false;

    update_player_card(idx);
}

static void play_btn_event_cb(lv_event_t * e)
{
    int idx =
        (int)(uintptr_t)lv_event_get_user_data(e);

    players[idx].is_playing = true;

    update_player_card(idx);
}

// ============================================================
// SCHERM OPBOUW
// ============================================================

void build_music_screen(lv_obj_t * parent)
{
    lv_obj_set_style_bg_color(
        parent,
        lv_color_hex(0x000000),
        0
    );

    lv_obj_clear_flag(
        parent,
        LV_OBJ_FLAG_SCROLLABLE
    );

    // --------------------------------------------------------
    // TERUGKNOP
    // --------------------------------------------------------

    lv_obj_t * btn_back =
        lv_btn_create(parent);

    lv_obj_set_size(btn_back, 90, 40);

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
        back_btn_event_cb,
        LV_EVENT_CLICKED,
        nullptr
    );

    lv_obj_t * lbl_back =
        lv_label_create(btn_back);

    lv_label_set_text(
        lbl_back,
        LV_SYMBOL_LEFT " Terug"
    );

    lv_obj_center(lbl_back);

    // --------------------------------------------------------
    // TITEL
    // --------------------------------------------------------

    lv_obj_t * lbl_title =
        lv_label_create(parent);

    lv_label_set_text(
        lbl_title,
        "Muziek"
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

    // --------------------------------------------------------
    // LIJSTCONTAINER
    // --------------------------------------------------------

    lv_obj_t * list =
        lv_obj_create(parent);

    lv_obj_set_size(list, 460, 390);

    lv_obj_align(
        list,
        LV_ALIGN_BOTTOM_MID,
        0,
        -10
    );

    lv_obj_set_style_bg_color(
        list,
        lv_color_hex(0x000000),
        0
    );

    lv_obj_set_style_border_width(list, 0, 0);

    lv_obj_set_style_pad_all(list, 0, 0);

    lv_obj_set_flex_flow(
        list,
        LV_FLEX_FLOW_COLUMN
    );

    lv_obj_set_style_pad_gap(list, 12, 0);

    // --------------------------------------------------------
    // SONOS KAARTEN
    // --------------------------------------------------------

    for (int i = 0; i < NUM_SONOS_PLAYERS; i++) {

        SonosPlayer& player = players[i];

        // Kaart

        lv_obj_t * card =
            lv_obj_create(list);

        lv_obj_set_size(card, 440, 156);

        lv_obj_set_style_border_width(card, 0, 0);

        lv_obj_set_style_radius(card, 12, 0);

        player.card = card;

        // Naam

        lv_obj_t * lbl_name =
            lv_label_create(card);

        lv_label_set_text(
            lbl_name,
            player.name
        );

        lv_obj_set_style_text_font(
            lbl_name,
            &lv_font_montserrat_18,
            0
        );

        lv_obj_set_style_text_color(
            lbl_name,
            lv_color_hex(0xFFFFFF),
            0
        );

        lv_obj_align(
            lbl_name,
            LV_ALIGN_TOP_LEFT,
            14,
            12
        );

        // Status-icoon

        player.lbl_icon =
            lv_label_create(card);

        lv_obj_set_style_text_font(
            player.lbl_icon,
            &lv_font_montserrat_20,
            0
        );

        lv_obj_align(
            player.lbl_icon,
            LV_ALIGN_TOP_RIGHT,
            -14,
            10
        );

        // Statusregel

        player.lbl_status =
            lv_label_create(card);

        lv_obj_set_style_text_color(
            player.lbl_status,
            lv_color_hex(0xCCCCCC),
            0
        );

        lv_obj_set_style_text_font(
            player.lbl_status,
            &lv_font_montserrat_16,
            0
        );

        lv_obj_align(
            player.lbl_status,
            LV_ALIGN_TOP_LEFT,
            14,
            48
        );

        // STOP knop

        lv_obj_t * btn_stop =
            lv_btn_create(card);

        lv_obj_set_size(btn_stop, 130, 44);

        lv_obj_align(
            btn_stop,
            LV_ALIGN_BOTTOM_LEFT,
            14,
            -14
        );

        lv_obj_set_style_bg_color(
            btn_stop,
            lv_color_hex(0xCC3333),
            0
        );

        lv_obj_add_event_cb(
            btn_stop,
            stop_btn_event_cb,
            LV_EVENT_CLICKED,
            (void*)(uintptr_t)i
        );

        lv_obj_t * lbl_stop =
            lv_label_create(btn_stop);

        lv_label_set_text(
            lbl_stop,
            LV_SYMBOL_STOP " Stop"
        );

        lv_obj_center(lbl_stop);

        // HERVAT knop

        lv_obj_t * btn_play =
            lv_btn_create(card);

        lv_obj_set_size(btn_play, 130, 44);

        lv_obj_align(
            btn_play,
            LV_ALIGN_BOTTOM_RIGHT,
            -14,
            -14
        );

        lv_obj_set_style_bg_color(
            btn_play,
            lv_color_hex(0x0099D8),
            0
        );

        lv_obj_add_event_cb(
            btn_play,
            play_btn_event_cb,
            LV_EVENT_CLICKED,
            (void*)(uintptr_t)i
        );

        lv_obj_t * lbl_play =
            lv_label_create(btn_play);

        lv_label_set_text(
            lbl_play,
            LV_SYMBOL_PLAY " Hervat"
        );

        lv_obj_center(lbl_play);

        // Initiële status toepassen

        update_player_card(i);
    }
}