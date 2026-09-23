#include "ha_client.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <math.h>

#include "secrets.h"
#include "ha_entities.h"
#include "scherm_ui.h"

// ====
// INTERNE HULPFUNCTIES
// ====

// Haalt de "state" string op van een HA-entiteit.
// Geeft "" terug bij verbindingsproblemen of parsefouten.

static String ha_get_state(const char* entity_id)
{
    HTTPClient http;

    String url =
        "http://" + String(HA_URL) +
        ":" + String(HA_PORT) +
        "/api/states/" + String(entity_id);

    http.begin(url);
    http.addHeader(
        "Authorization",
        "Bearer " + String(HA_TOKEN)
    );

    int httpCode = http.GET();

    if (httpCode != 200) {

        Serial.printf(
            "[HA] HTTP %d voor %s\n",
            httpCode,
            entity_id
        );

        http.end();
        return "";
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err =
        deserializeJson(doc, payload);

    if (err) {

        Serial.printf(
            "[HA] JSON fout voor %s\n",
            entity_id
        );

        return "";
    }

    return doc["state"].as<String>();
}

// ----
// Roept een Home Assistant service aan (POST /api/services/<domain>/<service>).
// Dit is de schrijf-tegenhanger van ha_get_state(): hiermee stuur je
// commando's naar HA (lamp aan/uit, temperatuur zetten, media pauzeren, ...).
//
// extra_json: optionele extra velden ZONDER buitenste accolades,
//   bv. "\"brightness_pct\":50" — wordt toegevoegd aan de request-body.
// Geeft true terug bij HTTP 200/201, false bij een fout of geen WiFi.
// ----

bool ha_call_service(
    const char* domain,
    const char* service,
    const char* entity_id,
    const char* extra_json
)
{
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[HA] Geen WiFi verbinding, service-aanroep overgeslagen");
        return false;
    }

    HTTPClient http;

    String url =
        "http://" + String(HA_URL) +
        ":" + String(HA_PORT) +
        "/api/services/" + String(domain) +
        "/" + String(service);

    http.begin(url);
    http.addHeader("Authorization", "Bearer " + String(HA_TOKEN));
    http.addHeader("Content-Type", "application/json");

    String body = "{\"entity_id\":\"" + String(entity_id) + "\"";

    if (extra_json != nullptr && extra_json[0] != '\0') {
        body += ",";
        body += extra_json;
    }

    body += "}";

    int httpCode = http.POST(body);

    bool ok = (httpCode == 200 || httpCode == 201);

    if (!ok) {
        Serial.printf(
            "[HA] Service-aanroep %s.%s op %s mislukt (HTTP %d)\n",
            domain, service, entity_id, httpCode
        );
    } else {
        Serial.printf(
            "[HA] Service-aanroep %s.%s op %s OK\n",
            domain, service, entity_id
        );
    }

    http.end();

    return ok;
}

// ----
// Haalt een float attribuut op uit een HA-entiteit.
// Geeft 'fallback' terug bij een fout.
// ----

static float ha_get_float_attribute(
    const char* entity_id,
    const char* attribute,
    float fallback = 0.0f
)
{
    HTTPClient http;

    String url =
        "http://" + String(HA_URL) +
        ":" + String(HA_PORT) +
        "/api/states/" + String(entity_id);

    http.begin(url);
    http.addHeader(
        "Authorization",
        "Bearer " + String(HA_TOKEN)
    );

    int httpCode = http.GET();

    if (httpCode != 200) {
        http.end();
        return fallback;
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err =
        deserializeJson(doc, payload);

    if (err) {
        return fallback;
    }

    return doc["attributes"][attribute].as<float>();
}

// ----
// Haalt een string attribuut op uit een HA-entiteit.
// Geeft 'fallback' terug bij een fout.
// ----

static String ha_get_string_attribute(
    const char* entity_id,
    const char* attribute,
    const char* fallback = ""
)
{
    HTTPClient http;

    String url =
        "http://" + String(HA_URL) +
        ":" + String(HA_PORT) +
        "/api/states/" + String(entity_id);

    http.begin(url);
    http.addHeader(
        "Authorization",
        "Bearer " + String(HA_TOKEN)
    );

    int httpCode = http.GET();

    if (httpCode != 200) {
        http.end();
        return fallback;
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err =
        deserializeJson(doc, payload);

    if (err) {
        return fallback;
    }

    return doc["attributes"][attribute].as<String>();
}

// ----
// Converteert een state-string naar een float.
// Vervangt komma door punt (NL locale).
// Geeft 'fallback' terug als de state leeg of
// "unavailable" / "unknown" is.
// ----

static float state_to_float(
    const String& state,
    float fallback = 0.0f
)
{
    if (state.length() == 0       ||
        state == "unavailable"    ||
        state == "unknown"        ||
        state == "none")
    {
        return fallback;
    }

    String s = state;
    s.replace(",", ".");

    return s.toFloat();
}

// ----
// Controleert of minstens één entiteit uit de lijst
// de state "on" heeft. Stopt zodra de eerste "on" gevonden
// is (short-circuit).
// ----

static bool any_entity_on(
    const char* const* entity_ids,
    int count
)
{
    for (int i = 0; i < count; i++) {

        if (ha_get_state(entity_ids[i]) == "on") {
            return true;
        }
    }

    return false;
}

// ====
// VERLICHTING
// ====

static void sync_lights()
{
    // ---- Woonkamer ----
    {
        const char* ids[] = {
            HA_LIGHT_WOONKAMER_1,
            HA_LIGHT_WOONKAMER_2
        };

        ui_set_light_state(
            "Woonkamer",
            any_entity_on(ids, 2)
        );
    }

    // ---- Hal ----
    {
        const char* ids[] = {
            HA_LIGHT_HAL_1,
            HA_LIGHT_HAL_2
        };

        ui_set_light_state(
            "Hal",
            any_entity_on(ids, 2)
        );
    }

    // ---- Bijkeuken ----
    {
        const char* ids[] = {
            HA_LIGHT_BIJKEUKEN_1
        };

        ui_set_light_state(
            "Bijkeuken",
            any_entity_on(ids, 1)
        );
    }

    // ---- Tuinkantoor ----
    {
        const char* ids[] = {
            HA_LIGHT_TUINKANTOOR_1,
            HA_LIGHT_TUINKANTOOR_2
        };

        ui_set_light_state(
            "Tuinkantoor",
            any_entity_on(ids, 2)
        );
    }

    // ---- Steeg ----
    {
        const char* ids[] = {
            HA_LIGHT_STEEG_1,
            HA_LIGHT_STEEG_2
        };

        ui_set_light_state(
            "Steeg",
            any_entity_on(ids, 2)
        );
    }

    // ---- Tuin ----
    {
        const char* ids[] = {
            HA_LIGHT_TUIN_1,
            HA_LIGHT_TUIN_2,
            HA_LIGHT_TUIN_3,
            HA_LIGHT_TUIN_4,
            HA_LIGHT_TUIN_5,
            HA_LIGHT_TUIN_6
        };

        ui_set_light_state(
            "Tuin",
            any_entity_on(ids, 6)
        );
    }

    // Keuken / Boaz / Kai / Badkamer / Overloop / Zolder:
    // Nog geen HA-entiteiten gekoppeld — worden als statisch
    // weergegeven totdat ze in ha_entities.h worden toegevoegd.
}

// ====
// KLIMAAT
// ====

static void sync_climate()
{
    // ----
    // Actuele temperatuur:
    // current_temperature attribuut van de woonkamer thermostaat
    // ----

    float current_temp = ha_get_float_attribute(
        HA_CLIMATE_WOONKAMER,
        "current_temperature",
        20.0f
    );

    // Attribuut ontbreekt (entiteit onbeschikbaar) → 0.0
    if (current_temp <= 0.0f || current_temp > 50.0f) {

        Serial.println(
            "[HA] Huidige temp woonkamer thermostaat ongeldig, fallback 20.0 °C"
        );

        current_temp = 20.0f;
    }

    Serial.printf(
        "[HA] Woonkamer temp (thermostaat): %.1f °C\n",
        current_temp
    );

    // ----
    // Doeltemperatuur:
    // temperature attribuut van de woonkamer thermostaat
    // ----

    float target_temp = ha_get_float_attribute(
        HA_CLIMATE_WOONKAMER,
        "temperature",
        20.0f
    );

    // Sanity check: doel moet tussen 5 en 30 °C liggen
    if (target_temp < 5.0f || target_temp > 30.0f) {

        Serial.println(
            "[HA] Doeltemperatuur woonkamer thermostaat ongeldig, fallback 20.0 °C"
        );

        target_temp = 20.0f;
    }

    Serial.printf(
        "[HA] Doeltemperatuur: %.1f °C\n",
        target_temp
    );

    // ----
    // Verwarmingsstatus:
    // OTGW vlam binary sensor = brander brandt echt
    // Dit is nauwkeuriger dan target > current vergelijking
    // ----

    bool heating =
        ha_get_state(HA_OTGW_FLAME) == "on";

    Serial.printf(
        "[HA] Vlam/verwarming actief: %s\n",
        heating ? "JA" : "NEE"
    );

    // ----
    // UI bijwerken — klimaat scherm
    // ----

    ui_set_climate_state(
        "Woonkamer",
        current_temp,
        target_temp,
        heating
    );

    // ----
    // UI bijwerken — home scherm thermostaat-widget
    // (grote temperatuur, doel en slider)
    // ----

    ui_set_home_thermostat(
        current_temp,
        target_temp
    );

    // ----
    // Tuinkantoor: Vonroc verwarming
    // ----

    float tk_current = ha_get_float_attribute(
        HA_CLIMATE_TUINKANTOOR,
        "current_temperature",
        20.0f
    );

    if (tk_current <= 0.0f || tk_current > 50.0f) {
        tk_current = 20.0f;
    }

    float tk_target = ha_get_float_attribute(
        HA_CLIMATE_TUINKANTOOR,
        "temperature",
        20.0f
    );

    if (tk_target < 5.0f || tk_target > 30.0f) {
        tk_target = 20.0f;
    }

    bool tk_heating =
        ha_get_string_attribute(
            HA_CLIMATE_TUINKANTOOR,
            "hvac_action",
            ""
        ) == "heating";

    Serial.printf(
        "[HA] Tuinkantoor: %.1f -> %.1f °C, verwarmen: %s\n",
        tk_current,
        tk_target,
        tk_heating ? "JA" : "NEE"
    );

    ui_set_climate_state(
        "Tuinkantoor",
        tk_current,
        tk_target,
        tk_heating
    );

    // ----
    // UI bijwerken — topbar temperatuurwidget
    // (Philips SML002 sensor)
    // ----

    String topbar_temp_raw =
        ha_get_state(HA_SENSOR_TOPBAR_TEMP);

    float topbar_temp =
        state_to_float(topbar_temp_raw, 0.0f);

    Serial.printf(
        "[HA] Topbar temp (Philips SML002): %.1f °C\n",
        topbar_temp
    );

    ui_set_outdoor_temp(topbar_temp);

    // ----
    // UI bijwerken — oranje vlam in topbar
    // (ketel brandt: cv of warm water)
    // ----

    ui_set_boiler_flame(heating);
}

// ====
// MUZIEK
// ====

static void sync_music()
{
    bool playing =
        ha_get_state(HA_SONOS_WOONKAMER) == "playing";

    ui_set_music_state(
        "Woonkamer Sonos",
        playing
    );
}

// ====
// PUBLIEKE API
// ====

void ha_init()
{
    Serial.println("[HA] Client initialized");
}

// ----

void ha_loop()
{
    static uint32_t last_poll = 0;

    uint32_t now = millis();

    // Elke 5 seconden pollen

    if (now - last_poll < 5000) {
        return;
    }

    last_poll = now;

    // Alleen pollen als WiFi verbonden is

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[HA] Geen WiFi verbinding");
        return;
    }

    Serial.println("[HA] Polling states...");

    sync_lights();
    sync_climate();
    sync_music();
}
