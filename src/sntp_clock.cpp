#include "sntp_clock.h"

#include <Arduino.h>
#include <time.h>

// ====
// CONFIGURATIE
// ====

#define NTP_SERVER      "pool.ntp.org"
#define TZ_AMSTERDAM    "CET-1CEST,M3.5.0,M10.5.0/3"   // NL zomer/wintertijd

// ====
// INTERNE STATE
// ====

static lv_obj_t* _clock_label = nullptr;
static uint32_t  _last_update = 0;
static const uint32_t UPDATE_INTERVAL_MS = 30000;  // elke 30 seconden

// Dag- en maandnamen in het Nederlands
static const char* dag_namen[] = {
    "Zondag", "Maandag", "Dinsdag", "Woensdag",
    "Donderdag", "Vrijdag", "Zaterdag"
};

static const char* maand_namen[] = {
    "Januari", "Februari", "Maart", "April",
    "Mei", "Juni", "Juli", "Augustus",
    "September", "Oktober", "November", "December"
};

// ====
// PUBLIEKE FUNCTIES
// ====

void sntp_clock_init()
{
    configTzTime(TZ_AMSTERDAM, NTP_SERVER);

    Serial.println("[SNTP] NTP sync gestart...");

    // Wacht max 5 seconden op eerste sync
    struct tm timeinfo;
    uint32_t start = millis();

    while (!getLocalTime(&timeinfo, 1000)) {
        if (millis() - start > 5000) {
            Serial.println("[SNTP] Timeout — klok nog niet gesynchroniseerd");
            return;
        }
    }

    Serial.println("[SNTP] Tijd gesynchroniseerd");
}

void sntp_clock_set_label(lv_obj_t* label)
{
    _clock_label = label;
}

void sntp_clock_tick()
{
    if (_clock_label == nullptr) return;

    uint32_t now = millis();

    if (now - _last_update < UPDATE_INTERVAL_MS && _last_update != 0) {
        return;
    }

    _last_update = now;

    struct tm timeinfo;

    if (!getLocalTime(&timeinfo, 100)) {
        lv_label_set_text(_clock_label, "Tijd onbekend");
        return;
    }

    char buf[48];

    snprintf(
        buf,
        sizeof(buf),
        "%s %d %s %02d:%02d",
        dag_namen[timeinfo.tm_wday],
        timeinfo.tm_mday,
        maand_namen[timeinfo.tm_mon],
        timeinfo.tm_hour,
        timeinfo.tm_min
    );

    lv_label_set_text(_clock_label, buf);
}