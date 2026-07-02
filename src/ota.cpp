#include <ArduinoOTA.h>
#include <WiFi.h>
#include "ota.h"
#include "config.h"

// ArduinoOTA pulls in mDNS, so the device also advertises as "<OTA_HOSTNAME>.local"
// (set in config.h — give each panel a unique name if you run more than one, or
// they'll collide on the same mDNS name). Upload via PlatformIO env :tessera_ota
// (espota), targeting the device IP or that hostname. No password on the home
// LAN; to require one, add ArduinoOTA.setPassword("...") below (and --auth=...
// to upload_flags).
#ifndef OTA_HOSTNAME
#define OTA_HOSTNAME "tessera"
#endif

void ota_init() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);

  ArduinoOTA.onStart([]() {
    Serial.println("OTA: update starting");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA: done, rebooting");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA: %u%%\r", total ? (progress * 100) / total : 0);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA error[%u]: ", error);
    switch (error) {
      case OTA_AUTH_ERROR:    Serial.println("auth failed");    break;
      case OTA_BEGIN_ERROR:   Serial.println("begin failed");   break;
      case OTA_CONNECT_ERROR: Serial.println("connect failed"); break;
      case OTA_RECEIVE_ERROR: Serial.println("receive failed"); break;
      case OTA_END_ERROR:     Serial.println("end failed");     break;
      default:                Serial.println("unknown");        break;
    }
  });

  ArduinoOTA.begin();
  Serial.printf("OTA ready: %s.local (%s)\n",
                OTA_HOSTNAME, WiFi.localIP().toString().c_str());
}

void ota_loop() {
  ArduinoOTA.handle();
}
