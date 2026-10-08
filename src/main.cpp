#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <lvgl.h>
#include <TAMC_GT911.h>

#include "display.h"
#include "scherm_ui.h"
#include "ha_client.h"
#include "sntp_clock.h"
#include "config.h"
#include "secrets.h"
#include "ha_entities.h"
#include "ota.h"
#include "idle_manager.h"

// ====
// DISPLAY / TOUCH CONFIGURATIE
// ====

#define LCD_WIDTH   480
#define LCD_HEIGHT  480

// GT911 Touch instantie
TAMC_GT911 ts(
    PIN_TOUCH_SDA,
    PIN_TOUCH_SCL,
    PIN_TOUCH_INT,
    PIN_TOUCH_RST,
    LCD_WIDTH,
    LCD_HEIGHT
);

// ====
// TOUCH CALLBACK VOOR LVGL
// ====

void my_touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    ts.read();

    if (ts.isTouched && ts.touches > 0) {

        idle_manager_touch_detected();

        if (idle_manager_is_wake_touch()) {
            data->state = LV_INDEV_STATE_REL;
            idle_manager_clear_wake_touch();
            return;
        }

        // Touch-coördinaten omzetten naar de schermoriëntatie
        int x = ts.points[0].y;
        int y = 479 - ts.points[0].x;

        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PR;

    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

// ====
// SETUP
// ====

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("====");
    Serial.println("Tessera starting...");
    Serial.println("====");

    // ----
    // 1. LVGL initialiseren
    // ----

    lv_init();

    Serial.println("LVGL initialized");

    // ----
    // 2. DISPLAY initialiseren
    // ----

    display_init();

    Serial.println("Display initialized");

    // ----
    // 3. I2C / GT911 touch initialiseren
    // ----

    Wire.begin(PIN_TOUCH_SDA, PIN_TOUCH_SCL);

    ts.begin();
    ts.setRotation(ROTATION_NORMAL);

    Serial.println("Touch (GT911) initialized");

    // ----
    // 4. Touch registreren bij LVGL
    // ----

    static lv_indev_drv_t indev_drv;

    lv_indev_drv_init(&indev_drv);

    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;

    lv_indev_drv_register(&indev_drv);

    Serial.println("Touch registered with LVGL");

    // ----
    // 5. WiFi verbinden
    // ----

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("WiFi connecting");

    uint32_t wifi_start = millis();
    const uint32_t WIFI_TIMEOUT_MS = 15000;

    while (WiFi.status() != WL_CONNECTED) {

        // LVGL draaiende houden tijdens wachten
        uint32_t now = millis();
        static uint32_t last_tick = 0;
        lv_tick_inc(now - last_tick);
        last_tick = now;
        lv_timer_handler();

        delay(250);
        Serial.print(".");

        if (millis() - wifi_start > WIFI_TIMEOUT_MS) {
            Serial.println();
            Serial.println("[WiFi] Timeout! Doorgaan zonder WiFi.");
            ui_set_wifi_connected(false);
            break;
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.print("[WiFi] Verbonden, IP: ");
        Serial.println(WiFi.localIP());
        ui_set_wifi_connected(true);
    }

    // ----
    // 5a. OTA (WiFi-flashen) initialiseren
    // ----

    if (WiFi.status() == WL_CONNECTED) {
        ota_init();
        Serial.println("OTA initialized");
    }

    // ----
    // 5b. SNTP klok synchroniseren
    // ----

    if (WiFi.status() == WL_CONNECTED) {
        sntp_clock_init();
    }

    // ----
    // 6. UI opbouwen
    // ----

    setup_atag_style_ui();

    Serial.println("UI setup complete");

    // ----
    // 7. Home Assistant
    // ----

    ha_init();

    Serial.println("Home Assistant initialized");

    idle_manager_init();
    Serial.println("Idle manager initialized");

    Serial.println("====");
    Serial.println("Setup complete");
    Serial.println("====");
}

// ====
// WIFI HERSTEL
// ====

static void maintain_wifi()
{
    static uint32_t last_attempt = 0;
    static bool was_connected = false;
    const uint32_t RECONNECT_INTERVAL_MS = 10000;

    bool connected = (WiFi.status() == WL_CONNECTED);
    if (connected) {
        if (!was_connected) {
            Serial.print("[WiFi] Verbonden, IP: ");
            Serial.println(WiFi.localIP());
            ui_set_wifi_connected(true);
            sntp_clock_init();
        }
        was_connected = true;
        return;
    }

    if (was_connected) {
        Serial.println("[WiFi] Verbinding verloren");
        ui_set_wifi_connected(false);
    }
    was_connected = false;

    uint32_t now = millis();
    if (now - last_attempt >= RECONNECT_INTERVAL_MS) {
        last_attempt = now;
        Serial.println("[WiFi] Opnieuw verbinden...");
        WiFi.reconnect();
    }
}

// ====
// LOOP
// ====

void loop()
{
    // ----
    // LVGL tijd bijwerken
    // ----

    static uint32_t loop_last_tick = 0;

    uint32_t now = millis();

    lv_tick_inc(now - loop_last_tick);
    loop_last_tick = now;

    // ----
    // LVGL verwerken
    // ----

    lv_timer_handler();
    idle_manager_update();

    maintain_wifi();

    // ----
    // Home Assistant verwerken
    // ----

    ha_loop();

    // ----
    // OTA (WiFi-flash) verwerken
    // ----

    ota_loop();

    // ----
    // Klok bijwerken
    // ----

    sntp_clock_tick();

    delay(5);
}