#pragma once

// ====
// VERLICHTING — WOONKAMER
// ====

#define HA_LIGHT_WOONKAMER_1    "light.k_o_o"
#define HA_LIGHT_WOONKAMER_2    "light.smartplug_remto"

// ====
// VERLICHTING — HAL
// ====

#define HA_LIGHT_HAL_1          "light.lamp_hal"
#define HA_LIGHT_HAL_2          "light.lamp_voordeur"

// ====
// VERLICHTING — BIJKEUKEN
// ====

#define HA_LIGHT_BIJKEUKEN_1    "light.signify_netherlanbijkeuken_lamp"

// ====
// VERLICHTING — TUINKANTOOR / SCHUUR
// ====

#define HA_LIGHT_TUINKANTOOR_1  "light.schuurlamp_fietsendeel"
#define HA_LIGHT_TUINKANTOOR_2  "light.philips_1"

// ====
// VERLICHTING — STEEG
// ====

#define HA_LIGHT_STEEG_1        "light.steeg_sproeier"
#define HA_LIGHT_STEEG_2        "light.steeg_bijkeukendeur"

// ====
// VERLICHTING — TUIN
// ====

#define HA_LIGHT_TUIN_1         "light.tuinwand_rechts_boven_2"
#define HA_LIGHT_TUIN_2         "light.signify_netherlands_b_v_lwg005"
#define HA_LIGHT_TUIN_3         "light.signify_netherlands_b_v_lwg005_2"
#define HA_LIGHT_TUIN_4         "light.tuinwand_rechts_onder"
#define HA_LIGHT_TUIN_5         "light.containerlamp"
#define HA_LIGHT_TUIN_6         "light.smartplug_buitenslinger"

// ====
// KLIMAAT — TEMPERATUURSENSOREN
// ====

// Fysieke Zigbee temperatuursensor woonkamer
#define HA_SENSOR_WOONKAMER_TEMP \
    "sensor.temperatuur_woonkamer_temperatuur"

// OTGW: kamertemperatuur zoals de thermostaat die meet
#define HA_OTGW_ROOM_TEMP \
    "sensor.opentherm_gateway_otgw_otgw_room_temperature"

// OTGW: setpoint / doeltemperatuur
#define HA_OTGW_ROOM_SETPOINT \
    "sensor.opentherm_gateway_otgw_otgw_room_setpoint"

// OTGW: vlam actief (brander brandt = echt aan het verwarmen)
#define HA_OTGW_FLAME \
    "binary_sensor.opentherm_gateway_otgw_otgw_flame"

// OTGW: centrale verwarming ingeschakeld
#define HA_OTGW_CENTRAL_HEATING \
    "binary_sensor.opentherm_gateway_otgw_otgw_central_heating"

// ====
// KLIMAAT — CLIMATE ENTITEITEN
// ====

// Woonkamer thermostaat (bron voor de voorpagina)
#define HA_CLIMATE_WOONKAMER \
    "climate.woonkamer_thermostaat"

// OpenTherm thermostat (voor set-commando's / attributen fallback)
#define HA_CLIMATE_CENTRAAL \
    "climate.opentherm_gateway_otgw_otgw_thermostat"

// Tuinkantoor verwarming (Vonroc)
#define HA_CLIMATE_TUINKANTOOR \
    "climate.vonroc_verwarming"

// ====
// SONOS
// ====

#define HA_SONOS_WOONKAMER      "media_player.woonkamer"
#define HA_SONOS_KANTOOR        "media_player.kantoor"

// ====
// TV
// ====

#define HA_TV_LG                "media_player.tv"
#define HA_TV_KPN               "media_player.kpn_tv"
