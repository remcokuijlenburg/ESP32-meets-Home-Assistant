#include "idle_manager.h"
#include "display.h"
#include <Arduino.h>

static uint32_t last_touch_time = 0;
static bool display_on = true;
static bool wake_touch = false;

void idle_manager_init() {
    last_touch_time = millis();
    display_on = true;
    wake_touch = false;
    display_set_backlight(BRIGHT_FULL);
    Serial.println("[IDLE] Idle manager initialized");
}

void idle_manager_touch_detected() {
    uint32_t now = millis();

    if (!display_on) {
        // Display was off, this is a wake-touch
        display_on = true;
        wake_touch = true;
        display_set_backlight(BRIGHT_FULL);
        Serial.println("[IDLE] Display woke up (wake-touch, next tap will interact)");
    } else {
        // Display was already on, normal interaction
        wake_touch = false;
    }

    last_touch_time = now;
}

void idle_manager_update() {
    uint32_t now = millis();
    uint32_t elapsed = now - last_touch_time;

    if (display_on && elapsed >= IDLE_TIMEOUT_MS) {
        display_on = false;
        wake_touch = false;
        display_set_backlight(0);  // Turn off
        Serial.printf("[IDLE] Display turned off (inactive for %lu ms)\n", elapsed);
    }
}

bool idle_manager_is_display_on() {
    return display_on;
}

bool idle_manager_is_wake_touch() {
    return wake_touch;
}

void idle_manager_clear_wake_touch() {
    wake_touch = false;
}