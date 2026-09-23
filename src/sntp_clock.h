#pragma once

#include <lvgl.h>

// Initialiseer SNTP (aanroepen na WiFi verbonden)
void sntp_clock_init();

// Koppel het datum/tijd label dat bijgewerkt moet worden
void sntp_clock_set_label(lv_obj_t* label);

// Aanroepen vanuit ha_loop() of loop() — werkt label bij indien nodig
void sntp_clock_tick();