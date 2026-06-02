#include <Arduino.h>
#include "wifi_manager.h"
#include "web_server.h"
#include "vehicle.h"

// ---------------------------------------------------------------------------
// main.cpp – VehicleLab ESP32 Remote-Control Firmware
//
// Boot sequence:
//   1. Serial init
//   2. Vehicle actuator init (PWM channels / GPIO)
//   3. WiFi: STA if credentials saved, else AP mode
//   4. HTTP server start
// ---------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n\n=== VehicleLab booting ===");

    vehicle_init();
    wifi_manager_begin();
    web_server_begin();

    Serial.println("=== Ready ===");
}

void loop() {
    // ESPAsyncWebServer is fully asynchronous – nothing needed here.
    // Add sensor reads, telemetry, or watchdog logic as required.
    delay(10);
}
