#pragma once

void ha_init();
void ha_loop();

// Roept een Home Assistant service aan, bv. ha_call_service("light", "turn_on", "light.k_o_o").
// extra_json: optionele extra velden ZONDER buitenste accolades,
//   bv. "\"brightness_pct\":50" — wordt toegevoegd aan de request-body.
// Geeft true terug bij HTTP 200/201, false bij een fout of geen WiFi.
bool ha_call_service(
    const char* domain,
    const char* service,
    const char* entity_id,
    const char* extra_json = nullptr
);
