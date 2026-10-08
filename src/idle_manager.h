#pragma once

#include <stdint.h>

// Idle timeout in milliseconds (10 seconds)
#define IDLE_TIMEOUT_MS 10000

// Initialize idle manager
void idle_manager_init();

// Call this on every touch event
void idle_manager_touch_detected();

// Call this periodically (e.g., every loop iteration)
void idle_manager_update();

// Get current state
bool idle_manager_is_display_on();
bool idle_manager_is_wake_touch();

// Reset wake-touch flag after handling
void idle_manager_clear_wake_touch();