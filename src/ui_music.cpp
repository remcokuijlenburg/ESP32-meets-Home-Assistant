#include <Arduino.h>
#include <lvgl.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <TJpg_Decoder.h>

#include "scherm_ui.h"
#include "ui_icons.h"
#include "ha_entities.h"
#include "ha_client.h"

// ============================================================
// LAYOUT CONSTANTEN
// ============================================================

#define CARD_WIDTH   440
#define CARD_HEIGHT  320
#define ART_SIZE     72

// ============================================================
// DATASTRUCTUUR
// ============================================================

struct SonosPlayer {
    // Configuratie (uit ha_entities.h / MUSIC_PLAYERS)
    const char* name;
    const char* entity_id;

    // Status (bijgewerkt vanuit ha_client.cpp)
    bool available;
    bool is_playing;
    int  volume_pct;        // 0-100, -1 = onbekend
    bool grouped;
    char title[48];
    char artist[48];
    char sources[3][24];

    // UI referenties
    lv_obj_t* card;
    lv_obj_t* lbl_title;
    lv_obj_t* lbl_artist;
    lv_obj_t* lbl_status_icon;
    lv_obj_t* lbl_playpause_icon;
    lv_obj_t* slider_volume;
    lv_obj_t* lbl_volume_pct;
    lv_obj_t* btn_group;
    lv_obj_t* lbl_group;
    lv_obj_t* btn_preset[3];
    lv_obj_t* lbl_preset[3];
    lv_obj_t* canvas_art;
    lv_obj_t* lbl_art_fallback;
    lv_color_t* art_buf;
};

static SonosPlayer players[NUM_MUSIC_PLAYERS];

// Context voor de preset-knoppen (welke speler / welke slot)
struct PresetCtx {
    int idx;
    int slot;
};

static PresetCtx preset_ctx[NUM_MUSIC_PLAYERS][3];

// ============================================================
// ALBUMHOES DECODEREN (TJpg_Decoder -> LVGL canvas buffer)
// ============================================================

static lv_color_t* g_decode_target = nullptr;

static bool jpg_output_cb(
    int16_t x,
    int16_t y,
    uint16_t w,
    uint16_t h,
    uint16_t* bitmap
)
{
    if (g_decode_target == nullptr) {
        return false;
    }

    for (uint16_t row = 0; row < h; row++) {

        int16_t py = y + row;

        if (py < 0 || py >= ART_SIZE) {
            continue;
        }

        for (uint16_t col = 0; col < w; col++) {

            int16_t px = x + col;

            if (px < 0 || px >= ART_SIZE) {
                continue;
            }

            g_decode_target[py * ART_SIZE + px].full =
                bitmap[row * w + col];
        }
    }

    return true;
}

// ============================================================
// KAART BIJWERKEN
// ============================================================

static void update_player_card(int idx)
{
    SonosPlayer& p = players[idx];

    // Titel / artiest

    if (p.lbl_title != nullptr) {

        if (!p.available) {
            lv_label_set_text(p.lbl_title, "Niet beschikbaar");
        } else if (p.title[0] != '\0') {
            lv_label_set_text(p.lbl_title, p.title);
        } else {
            lv_label_set_text(p.lbl_title, "Geen titel");
        }
    }

    if (p.lbl_artist != nullptr) {
        lv_label_set_text(
            p.lbl_artist,
            p.artist[0] != '\0' ? p.artist : ""
        );
    }

    // Status-icoon (rechtsboven op de kaart)

    if (p.lbl_status_icon != nullptr) {

        lv_label_set_text(
            p.lbl_status_icon,
            p.is_playing ? LV_SYMBOL_PLAY : LV_SYMBOL_PAUSE
        );

        lv_obj_set_style_text_color(
            p.lbl_status_icon,
            p.is_playing
                ? lv_color_hex(0x00CC66)
                : lv_color_hex(0xAAAAAA),
            0
        );
    }

    // Play/pauze-knop icoon

    if (p.lbl_playpause_icon != nullptr) {
        lv_label_set_text(
            p.lbl_playpause_icon,
            p.is_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY
        );
    }

    // Kaartkleur

    if (p.card != nullptr) {
        lv_obj_set_style_bg_color(
            p.card,
            p.is_playing
                ? lv_color_hex(0x1A2A1A)
                : lv_color_hex(0x222222),
            0
        );
    }

    // Volume

    if (p.slider_volume != nullptr) {
        lv_slider_set_value(
            p.slider_volume,
            p.volume_pct >= 0 ? p.volume_pct : 0,
            LV_ANIM_OFF
        );
    }

    if (p.lbl_volume_pct != nullptr) {

        if (p.volume_pct >= 0) {
            lv_label_set_text_fmt(p.lbl_volume_pct, "%d%%", p.volume_pct);
        } else {
            lv_label_set_text(p.lbl_volume_pct, "--%");
        }
    }

    // Groeperen / ontkoppelen (alleen op niet-eerste spelers)

    if (p.lbl_group != nullptr) {
        lv_label_set_text(
            p.lbl_group,
            p.grouped ? LV_SYMBOL_CLOSE " Ontkoppel" : LV_SYMBOL_SHUFFLE " Groepeer"
        );
    }

    // Presets (bronnen uit HA source_list)

    for (int slot = 0; slot < 3; slot++) {

        if (p.btn_preset[slot] == nullptr) {
            continue;
        }

        if (p.sources[slot][0] == '\0') {
            lv_obj_add_flag(p.btn_preset[slot], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(p.btn_preset[slot], LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(p.lbl_preset[slot], p.sources[slot]);
        }
    }
}

// ============================================================
// PUBLIEKE UPDATEFUNCTIES (AANGEROEPEN VANUIT ha_client.cpp)
// ============================================================

static void copy_truncated(char* dst, size_t dst_size, const char* src)
{
    if (src == nullptr) {
        dst[0] = '\0';
        return;
    }

    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
}

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
)
{
    if (idx < 0 || idx >= NUM_MUSIC_PLAYERS) {
        return;
    }

    SonosPlayer& p = players[idx];

    p.available  = available;
    p.is_playing = playing;
    p.volume_pct = volume_pct;
    p.grouped    = grouped;

    copy_truncated(p.title, sizeof(p.title), title);
    copy_truncated(p.artist, sizeof(p.artist), artist);

    copy_truncated(p.sources[0], sizeof(p.sources[0]), src0);
    copy_truncated(p.sources[1], sizeof(p.sources[1]), src1);
    copy_truncated(p.sources[2], sizeof(p.sources[2]), src2);

    update_player_card(idx);
}

void ui_set_music_picture(
    int idx,
    const uint8_t* jpeg_data,
    size_t len
)
{
    if (idx < 0 || idx >= NUM_MUSIC_PLAYERS) {
        return;
    }

    SonosPlayer& p = players[idx];

    if (p.canvas_art == nullptr || p.art_buf == nullptr) {
        return;
    }

    if (jpeg_data == nullptr || len == 0) {
        // Geen albumhoes: canvas donker maken, fallback-icoon tonen
        lv_canvas_fill_bg(p.canvas_art, lv_color_hex(0x333333), LV_OPA_COVER);

        if (p.lbl_art_fallback != nullptr) {
            lv_obj_clear_flag(p.lbl_art_fallback, LV_OBJ_FLAG_HIDDEN);
        }

        lv_obj_invalidate(p.canvas_art);
        return;
    }

    lv_canvas_fill_bg(p.canvas_art, lv_color_hex(0x333333), LV_OPA_COVER);

    g_decode_target = p.art_buf;

    TJpgDec.setJpgScale(4);
    TJpgDec.setSwapBytes(false);
    TJpgDec.setCallback(jpg_output_cb);

    bool ok = (TJpgDec.drawJpg(0, 0, jpeg_data, len) == 0);

    g_decode_target = nullptr;

    if (ok && p.lbl_art_fallback != nullptr) {
        lv_obj_add_flag(p.lbl_art_fallback, LV_OBJ_FLAG_HIDDEN);
    } else if (!ok && p.lbl_art_fallback != nullptr) {
        lv_obj_clear_flag(p.lbl_art_fallback, LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_invalidate(p.canvas_art);
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

static void playpause_btn_event_cb(lv_event_t * e)
{
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);

    SonosPlayer& p = players[idx];

    const char* service = p.is_playing ? "media_pause" : "media_play";

    ha_call_service("media_player", service, p.entity_id);

    // Optimistische UI-update — sync_music() bevestigt dit binnen 5 sec.
    p.is_playing = !p.is_playing;
    update_player_card(idx);
}

static void prev_btn_event_cb(lv_event_t * e)
{
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);

    ha_call_service(
        "media_player",
        "media_previous_track",
        players[idx].entity_id
    );
}

static void next_btn_event_cb(lv_event_t * e)
{
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);

    ha_call_service(
        "media_player",
        "media_next_track",
        players[idx].entity_id
    );
}

static void volume_slider_released_cb(lv_event_t * e)
{
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);

    lv_obj_t* slider = lv_event_get_target(e);

    int value = lv_slider_get_value(slider);

    char extra[32];

    snprintf(
        extra,
        sizeof(extra),
        "\"volume_level\":%.2f",
        value / 100.0f
    );

    ha_call_service(
        "media_player",
        "volume_set",
        players[idx].entity_id,
        extra
    );

    players[idx].volume_pct = value;

    if (players[idx].lbl_volume_pct != nullptr) {
        lv_label_set_text_fmt(players[idx].lbl_volume_pct, "%d%%", value);
    }
}

static void group_btn_event_cb(lv_event_t * e)
{
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);

    SonosPlayer& p = players[idx];

    if (p.grouped) {

        // Ontkoppelen: de speler zelf uit zijn groep halen
        ha_call_service("media_player", "unjoin", p.entity_id);

    } else {

        // Groeperen met de eerste speaker in de lijst (de "leider")
        char extra[64];

        snprintf(
            extra,
            sizeof(extra),
            "\"group_members\":[\"%s\"]",
            p.entity_id
        );

        ha_call_service(
            "media_player",
            "join",
            MUSIC_PLAYERS[0].entity_id,
            extra
        );
    }

    // sync_music() bevestigt de nieuwe groepsstatus binnen 5 seconden
}

static void preset_btn_event_cb(lv_event_t * e)
{
    PresetCtx* ctx = (PresetCtx*)lv_event_get_user_data(e);

    const char* source_name = players[ctx->idx].sources[ctx->slot];

    if (source_name[0] == '\0') {
        return;
    }

    char extra[48];

    snprintf(
        extra,
        sizeof(extra),
        "\"source\":\"%s\"",
        source_name
    );

    ha_call_service(
        "media_player",
        "select_source",
        players[ctx->idx].entity_id,
        extra
    );
}

// ============================================================
// SCHERM OPBOUW
// ============================================================

void build_music_screen(lv_obj_t * parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x000000), 0);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    // --------------------------------------------------------
    // TERUGKNOP
    // --------------------------------------------------------

    lv_obj_t * btn_back = lv_btn_create(parent);
    lv_obj_set_size(btn_back, 90, 40);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x333333), 0);
    lv_obj_add_event_cb(btn_back, back_btn_event_cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t * lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, LV_SYMBOL_LEFT " Terug");
    lv_obj_center(lbl_back);

    // --------------------------------------------------------
    // TITEL
    // --------------------------------------------------------

    lv_obj_t * lbl_title = lv_label_create(parent);
    lv_label_set_text(lbl_title, "Muziek");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 15);

    // --------------------------------------------------------
    // LIJSTCONTAINER (scrollt verticaal als de kaarten niet passen)
    // --------------------------------------------------------

    lv_obj_t * list = lv_obj_create(parent);
    lv_obj_set_size(list, 460, 390);
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(list, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 0, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(list, 12, 0);

    // --------------------------------------------------------
    // SONOS KAARTEN
    // --------------------------------------------------------

    for (int i = 0; i < NUM_MUSIC_PLAYERS; i++) {

        SonosPlayer& p = players[i];

        p.name        = MUSIC_PLAYERS[i].name;
        p.entity_id   = MUSIC_PLAYERS[i].entity_id;
        p.available   = false;
        p.is_playing  = false;
        p.volume_pct  = -1;
        p.grouped     = false;
        p.title[0]    = '\0';
        p.artist[0]   = '\0';
        p.sources[0][0] = '\0';
        p.sources[1][0] = '\0';
        p.sources[2][0] = '\0';

        // Kaart

        lv_obj_t * card = lv_obj_create(list);
        lv_obj_set_size(card, CARD_WIDTH, CARD_HEIGHT);
        lv_obj_set_style_border_width(card, 0, 0);
        lv_obj_set_style_radius(card, 12, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        p.card = card;

        // Albumhoes (canvas) + fallback-icoon

        lv_obj_t * canvas = lv_canvas_create(card);

        size_t buf_bytes = LV_CANVAS_BUF_SIZE_TRUE_COLOR(ART_SIZE, ART_SIZE);

        p.art_buf = (lv_color_t*)heap_caps_malloc(
            buf_bytes,
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
        );

        if (p.art_buf != nullptr) {

            lv_canvas_set_buffer(
                canvas,
                p.art_buf,
                ART_SIZE,
                ART_SIZE,
                LV_IMG_CF_TRUE_COLOR
            );

            lv_canvas_fill_bg(canvas, lv_color_hex(0x333333), LV_OPA_COVER);
        }

        lv_obj_align(canvas, LV_ALIGN_TOP_LEFT, 14, 14);
        p.canvas_art = canvas;

        lv_obj_t * lbl_fallback = lv_label_create(card);
        lv_label_set_text(lbl_fallback, LV_SYMBOL_AUDIO);
        lv_obj_set_style_text_font(lbl_fallback, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(lbl_fallback, lv_color_hex(0x888888), 0);
        lv_obj_align(lbl_fallback, LV_ALIGN_TOP_LEFT, 14 + ART_SIZE / 2 - 8, 14 + ART_SIZE / 2 - 10);
        p.lbl_art_fallback = lbl_fallback;

        // Naam (kamer)

        lv_obj_t * lbl_name = lv_label_create(card);
        lv_label_set_text(lbl_name, p.name);
        lv_obj_set_style_text_font(lbl_name, &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(lbl_name, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(lbl_name, LV_ALIGN_TOP_LEFT, 14 + ART_SIZE + 14, 12);

        // Status-icoon rechtsboven

        p.lbl_status_icon = lv_label_create(card);
        lv_obj_set_style_text_font(p.lbl_status_icon, &lv_font_montserrat_20, 0);
        lv_obj_align(p.lbl_status_icon, LV_ALIGN_TOP_RIGHT, -14, 12);

        // Titel / artiest

        p.lbl_title = lv_label_create(card);
        lv_label_set_long_mode(p.lbl_title, LV_LABEL_LONG_DOT);
        lv_obj_set_width(p.lbl_title, 250);
        lv_obj_set_style_text_font(p.lbl_title, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(p.lbl_title, lv_color_hex(0xEEEEEE), 0);
        lv_obj_align(p.lbl_title, LV_ALIGN_TOP_LEFT, 14 + ART_SIZE + 14, 40);

        p.lbl_artist = lv_label_create(card);
        lv_label_set_long_mode(p.lbl_artist, LV_LABEL_LONG_DOT);
        lv_obj_set_width(p.lbl_artist, 250);
        lv_obj_set_style_text_font(p.lbl_artist, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(p.lbl_artist, lv_color_hex(0xAAAAAA), 0);
        lv_obj_align(p.lbl_artist, LV_ALIGN_TOP_LEFT, 14 + ART_SIZE + 14, 64);

        // Transport: vorige / afspelen-pauzeren / volgende

        lv_obj_t * btn_prev = lv_btn_create(card);
        lv_obj_set_size(btn_prev, 56, 44);
        lv_obj_align(btn_prev, LV_ALIGN_TOP_MID, -76, 102);
        lv_obj_set_style_bg_color(btn_prev, lv_color_hex(0x333333), 0);
        lv_obj_add_event_cb(btn_prev, prev_btn_event_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
        lv_obj_t * lbl_prev = lv_label_create(btn_prev);
        lv_label_set_text(lbl_prev, LV_SYMBOL_PREV);
        lv_obj_center(lbl_prev);

        lv_obj_t * btn_playpause = lv_btn_create(card);
        lv_obj_set_size(btn_playpause, 64, 44);
        lv_obj_align(btn_playpause, LV_ALIGN_TOP_MID, 0, 102);
        lv_obj_set_style_bg_color(btn_playpause, lv_color_hex(0x0099D8), 0);
        lv_obj_add_event_cb(btn_playpause, playpause_btn_event_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
        p.lbl_playpause_icon = lv_label_create(btn_playpause);
        lv_label_set_text(p.lbl_playpause_icon, LV_SYMBOL_PLAY);
        lv_obj_center(p.lbl_playpause_icon);

        lv_obj_t * btn_next = lv_btn_create(card);
        lv_obj_set_size(btn_next, 56, 44);
        lv_obj_align(btn_next, LV_ALIGN_TOP_MID, 76, 102);
        lv_obj_set_style_bg_color(btn_next, lv_color_hex(0x333333), 0);
        lv_obj_add_event_cb(btn_next, next_btn_event_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
        lv_obj_t * lbl_next = lv_label_create(btn_next);
        lv_label_set_text(lbl_next, LV_SYMBOL_NEXT);
        lv_obj_center(lbl_next);

        // Volume

        lv_obj_t * lbl_volume_caption = lv_label_create(card);
        lv_label_set_text(lbl_volume_caption, LV_SYMBOL_VOLUME_MID " Volume");
        lv_obj_set_style_text_font(lbl_volume_caption, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl_volume_caption, lv_color_hex(0xAAAAAA), 0);
        lv_obj_align(lbl_volume_caption, LV_ALIGN_TOP_LEFT, 14, 158);

        p.lbl_volume_pct = lv_label_create(card);
        lv_obj_set_style_text_color(p.lbl_volume_pct, lv_color_hex(0xAAAAAA), 0);
        lv_obj_align(p.lbl_volume_pct, LV_ALIGN_TOP_RIGHT, -14, 158);

        p.slider_volume = lv_slider_create(card);
        lv_obj_set_size(p.slider_volume, CARD_WIDTH - 28, 20);
        lv_obj_align(p.slider_volume, LV_ALIGN_TOP_LEFT, 14, 180);
        lv_slider_set_range(p.slider_volume, 0, 100);
        lv_obj_set_style_bg_color(p.slider_volume, lv_color_hex(0x0099D8), LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(p.slider_volume, lv_color_hex(0x0099D8), LV_PART_KNOB);
        lv_obj_add_event_cb(p.slider_volume, volume_slider_released_cb, LV_EVENT_RELEASED, (void*)(uintptr_t)i);

        // Presets (bronnen uit HA source_list, max 3)

        for (int slot = 0; slot < 3; slot++) {

            preset_ctx[i][slot].idx = i;
            preset_ctx[i][slot].slot = slot;

            lv_obj_t * btn_preset = lv_btn_create(card);
            lv_obj_set_size(btn_preset, 130, 36);
            lv_obj_align(btn_preset, LV_ALIGN_TOP_LEFT, 14 + slot * 141, 222);
            lv_obj_set_style_bg_color(btn_preset, lv_color_hex(0x2A2A2A), 0);
            lv_obj_add_flag(btn_preset, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_event_cb(
                btn_preset,
                preset_btn_event_cb,
                LV_EVENT_CLICKED,
                &preset_ctx[i][slot]
            );

            lv_obj_t * lbl_preset = lv_label_create(btn_preset);
            lv_label_set_long_mode(lbl_preset, LV_LABEL_LONG_DOT);
            lv_obj_set_width(lbl_preset, 114);
            lv_obj_set_style_text_align(lbl_preset, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_center(lbl_preset);

            p.btn_preset[slot] = btn_preset;
            p.lbl_preset[slot] = lbl_preset;
        }

        // Groeperen / ontkoppelen (niet zinvol op de eerste/leidende speler)

        if (i > 0) {

            lv_obj_t * btn_group = lv_btn_create(card);
            lv_obj_set_size(btn_group, 200, 40);
            lv_obj_align(btn_group, LV_ALIGN_TOP_LEFT, 14, 266);
            lv_obj_set_style_bg_color(btn_group, lv_color_hex(0x555555), 0);
            lv_obj_add_event_cb(btn_group, group_btn_event_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)i);

            lv_obj_t * lbl_group = lv_label_create(btn_group);
            lv_obj_center(lbl_group);

            p.btn_group = btn_group;
            p.lbl_group = lbl_group;
        } else {
            p.btn_group = nullptr;
            p.lbl_group = nullptr;
        }

        // Initiële status toepassen (alles nog onbekend totdat ha_client sync't)

        update_player_card(i);
        ui_set_music_picture(i, nullptr, 0);
    }
}
