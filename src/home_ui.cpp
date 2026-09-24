#include <lvgl.h>
#include "scherm_ui.h"
#include "ui_icons.h"
#include "sntp_clock.h"
#include "ha_client.h"
#include "ha_entities.h"

// ==========================================
// TV-KNOP STATUS
// ==========================================

static lv_obj_t* btn_tv    = nullptr;
static bool       tv_is_on = false;

static const lv_color_t TV_COLOR_OFF = lv_color_hex(0x333333);
static const lv_color_t TV_COLOR_ON  = lv_color_hex(0x0099D8);

static void update_tv_button()
{
    if (btn_tv == nullptr) {
        return;
    }

    lv_obj_set_style_bg_color(
        btn_tv,
        tv_is_on ? TV_COLOR_ON : TV_COLOR_OFF,
        0
    );
}

void ui_set_tv_state(bool on)
{
    tv_is_on = on;
    update_tv_button();
}

// ==========================================
// NAVIGATIE CALLBACKS
// ==========================================

static void nav_lights_cb(lv_event_t * e)
{
    if (scr_lights != NULL) {
        lv_scr_load(scr_lights);
    }
}

static void nav_climate_cb(lv_event_t * e)
{
    if (scr_climate_overview != NULL) {
        lv_scr_load(scr_climate_overview);
    }
}
static void nav_music_cb(lv_event_t * e)
{
    if (scr_music != NULL) {
        lv_scr_load(scr_music);
    }
}

static void tv_btn_event_cb(lv_event_t * e)
{
    if (!tv_is_on) {

        // Aanzetten via de HA-automatisering (regelt WOL/inputs),
        // dezelfde die ook aan de Hue-knop hangt.
        ha_call_service(
            "automation",
            "trigger",
            HA_AUTOMATION_TV_AAN,
            "\"skip_condition\":true"
        );

    } else {

        // Uitzetten: beide media_players rechtstreeks uitzetten
        ha_call_service("media_player", "turn_off", HA_TV_LG);
        ha_call_service("media_player", "turn_off", HA_TV_KPN);
    }

    // Optimistische UI-update — sync_tv() bevestigt dit binnen
    // 5 seconden met de echte HA-status.
    tv_is_on = !tv_is_on;
    update_tv_button();
}

// ==========================================
// SCHERM OPBOUW (HOMESCREEN)
// ==========================================

void build_home_screen(lv_obj_t * parent)
{
    // Achtergrond
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x000000), 0);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    // =====================================================
    // TOP BAR
    // =====================================================

    lv_obj_t * top_box = lv_obj_create(parent);

    lv_obj_set_size(top_box, 460, 60);
    lv_obj_align(top_box, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_set_style_bg_color(top_box, lv_color_hex(0x111111), 0);
    lv_obj_set_style_border_width(top_box, 0, 0);
    lv_obj_set_style_radius(top_box, 8, 0);

    lv_obj_t * lbl_date = lv_label_create(top_box);

   lv_label_set_text(lbl_date, "Laden...");
sntp_clock_set_label(lbl_date);

    lv_obj_set_style_text_color(
        lbl_date,
        lv_color_hex(0xCCCCCC),
        0
    );

    lv_obj_align(
        lbl_date,
        LV_ALIGN_LEFT_MID,
        10,
        0
    );

    lv_obj_t * lbl_temp_topbar = lv_label_create(top_box);

    lv_label_set_text(
        lbl_temp_topbar,
        LV_SYMBOL_IMAGE " --.-°C"
    );

    lv_obj_set_style_text_color(
        lbl_temp_topbar,
        lv_color_hex(0xCCCCCC),
        0
    );

    lv_obj_align(
        lbl_temp_topbar,
        LV_ALIGN_RIGHT_MID,
        -10,
        0
    );

    // Koppel aan globale pointer zodat HA sync
    // (ui_set_outdoor_temp) dit label kan bijwerken
    lbl_weather = lbl_temp_topbar;

    // Oranje vlam: zichtbaar zolang de ketel brandt
    // (HA sync via ui_set_boiler_flame)
    lv_obj_t * lbl_flame = lv_label_create(top_box);

    lv_obj_set_style_text_font(
        lbl_flame,
        &Custom_Icons_32,
        0
    );

    lv_label_set_text(lbl_flame, ICON_FIRE);

    lv_obj_set_style_text_color(
        lbl_flame,
        lv_color_hex(0xFF6600),
        0
    );

    lv_obj_align(
        lbl_flame,
        LV_ALIGN_RIGHT_MID,
        -95,
        0
    );

    lv_obj_add_flag(lbl_flame, LV_OBJ_FLAG_HIDDEN);

    lbl_flame_home = lbl_flame;

    // =====================================================
    // MIDDEN (KLIMAAT WIDGET)
    // =====================================================

    lv_obj_t * mid_box = lv_obj_create(parent);

    lv_obj_set_size(mid_box, 460, 280);
    lv_obj_align(mid_box, LV_ALIGN_CENTER, 0, -10);

    lv_obj_set_style_bg_color(mid_box, lv_color_hex(0x111111), 0);
    lv_obj_set_style_border_width(mid_box, 0, 0);
    lv_obj_set_style_radius(mid_box, 8, 0);

    // Grote temperatuur

    lv_obj_t * lbl_temp = lv_label_create(mid_box);

    lv_label_set_text(lbl_temp, "20.5°");

    // Koppel aan globale pointer zodat Home Assistant
    // sync (ui_set_home_thermostat) dit label kan bijwerken
    lbl_temp_home = lbl_temp;

    lv_obj_set_style_text_font(
        lbl_temp,
        &lv_font_montserrat_48,
        0
    );

    lv_obj_set_style_text_color(
        lbl_temp,
        lv_color_hex(0xFFFFFF),
        0
    );

    lv_obj_align(
        lbl_temp,
        LV_ALIGN_TOP_MID,
        0,
        20
    );

    // Status

    lv_obj_t * lbl_schema = lv_label_create(mid_box);

    lv_label_set_text(
        lbl_schema,
        LV_SYMBOL_CHARGE " Schema"
    );

    lv_obj_set_style_text_color(
        lbl_schema,
        lv_color_hex(0xAAAAAA),
        0
    );

    lv_obj_align(
        lbl_schema,
        LV_ALIGN_CENTER,
        -100,
        20
    );

    // Doel

    lv_obj_t * lbl_doel = lv_label_create(mid_box);

    lv_label_set_text(
        lbl_doel,
        "Doel: 21.0°C"
    );

    // Koppel aan globale pointer voor HA sync
    lbl_doel_home = lbl_doel;

    lv_obj_set_style_text_color(
        lbl_doel,
        lv_color_hex(0xFFFFFF),
        0
    );

    lv_obj_align(
        lbl_doel,
        LV_ALIGN_CENTER,
        80,
        20
    );

    // Slider

    lv_obj_t * slider_temp = lv_slider_create(mid_box);

    lv_obj_set_size(slider_temp, 400, 25);

    lv_obj_align(
        slider_temp,
        LV_ALIGN_BOTTOM_MID,
        0,
        -25
    );

    lv_slider_set_range(slider_temp, 150, 300);

    lv_slider_set_value(
        slider_temp,
        210,
        LV_ANIM_OFF
    );

    lv_obj_set_style_bg_color(
        slider_temp,
        lv_color_hex(0x0099D8),
        LV_PART_INDICATOR
    );

    lv_obj_set_style_bg_color(
        slider_temp,
        lv_color_hex(0x0099D8),
        LV_PART_KNOB
    );

    // Koppel aan globale pointer voor HA sync
    slider_home = slider_temp;

    // =====================================================
    // ONDERSTE NAVIGATIEBALK
    // =====================================================

    lv_obj_t * nav_bar = lv_obj_create(parent);

    lv_obj_set_size(nav_bar, 460, 80);

    lv_obj_align(
        nav_bar,
        LV_ALIGN_BOTTOM_MID,
        0,
        -10
    );

    lv_obj_set_style_bg_color(
        nav_bar,
        lv_color_hex(0x000000),
        0
    );

    lv_obj_set_style_border_width(nav_bar, 0, 0);
    lv_obj_set_style_pad_all(nav_bar, 0, 0);

    // Flex-layout

    lv_obj_set_flex_flow(
        nav_bar,
        LV_FLEX_FLOW_ROW
    );

    lv_obj_set_flex_align(
        nav_bar,
        LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    lv_color_t btn_kleur = lv_color_hex(0x007ACC);

    // =====================================================
    // KNOP 1 — VERLICHTING
    // =====================================================

    lv_obj_t * btn_l = lv_btn_create(nav_bar);

    lv_obj_set_size(btn_l, 105, 70);

    lv_obj_set_style_bg_color(
        btn_l,
        btn_kleur,
        0
    );

    lv_obj_add_event_cb(
        btn_l,
        nav_lights_cb,
        LV_EVENT_CLICKED,
        NULL
    );

    lv_obj_t * lbl_l = lv_label_create(btn_l);

    lv_obj_set_style_text_font(
        lbl_l,
        &Custom_Icons_32,
        0
    );

    lv_label_set_text(lbl_l, ICON_LIGHTS);

    lv_obj_center(lbl_l);

    // =====================================================
    // KNOP 2 — TV
    // =====================================================

    lv_obj_t * btn_m = lv_btn_create(nav_bar);

    lv_obj_set_size(btn_m, 105, 70);

    btn_tv = btn_m;

    lv_obj_add_event_cb(
        btn_m,
        tv_btn_event_cb,
        LV_EVENT_CLICKED,
        NULL
    );

    update_tv_button();

    lv_obj_t * lbl_m = lv_label_create(btn_m);

    lv_obj_set_style_text_font(
        lbl_m,
        &Custom_Icons_32,
        0
    );

    lv_label_set_text(lbl_m, ICON_TV);

    lv_obj_center(lbl_m);

    // =====================================================
    // KNOP 3 — MUZIEK
    // =====================================================

    lv_obj_t * btn_mu = lv_btn_create(nav_bar);
    lv_obj_add_event_cb(
    btn_mu,
    nav_music_cb,
    LV_EVENT_CLICKED,
    NULL
    );

    lv_obj_set_size(btn_mu, 105, 70);

    lv_obj_set_style_bg_color(
        btn_mu,
        btn_kleur,
        0
    );

    lv_obj_t * lbl_mu = lv_label_create(btn_mu);

    lv_obj_set_style_text_font(
        lbl_mu,
        &Custom_Icons_32,
        0
    );

    lv_label_set_text(lbl_mu, ICON_MUSIC);

    lv_obj_center(lbl_mu);

    // =====================================================
    // KNOP 4 — KLIMAAT
    // =====================================================

    lv_obj_t * btn_c = lv_btn_create(nav_bar);

    lv_obj_set_size(btn_c, 105, 70);

    lv_obj_set_style_bg_color(
        btn_c,
        btn_kleur,
        0
    );

    lv_obj_add_event_cb(
        btn_c,
        nav_climate_cb,
        LV_EVENT_CLICKED,
        NULL
    );

    lv_obj_t * lbl_c = lv_label_create(btn_c);

    lv_obj_set_style_text_font(
        lbl_c,
        &Custom_Icons_32,
        0
    );

    lv_label_set_text(lbl_c, ICON_CLIMATE);

    lv_obj_center(lbl_c);
}