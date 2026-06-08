#include "web_server.h"
#include "vehicle.h"
#include "wifi_manager.h"

#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Arduino.h>

// ---------------------------------------------------------------------------
// web_server.cpp – HTTP routes for the VehicleLab remote-control server.
//
// Static HTML pages (drive.html, settings.html, prefs.html) are stored in
// LittleFS and served directly from flash storage.  Flash the filesystem
// separately with:   pio run -t uploadfs
// ---------------------------------------------------------------------------

static AsyncWebServer server(80);

static void json_ok(AsyncWebServerRequest* req, const String& body) {
    req->send(200, "application/json", body);
}

// ============================================================
// Route registration
// ============================================================
void web_server_begin() {

    // Mount LittleFS (true = format on first boot if partition is empty)
    if (!LittleFS.begin(true)) {
        Serial.println("[web] LittleFS mount FAILED – filesystem not available");
    } else {
        Serial.println("[web] LittleFS mounted OK");
    }

    // ---- GET /api/status ---------------------------------------------------
    // Full device status used by settings.html to populate Mode/SSID/IP fields.
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* req) {
        String body = "{\"speed\":"  + String(vehicle_get_speed())  +
                      ",\"turn\":"   + String(vehicle_get_turn())   +
                      ",\"mode\":\"" + (wifi_manager_is_sta_mode() ? "Station" : "Access Point") + "\"" +
                      ",\"ssid\":\"" + wifi_manager_current_ssid() + "\"" +
                      ",\"ip\":\""   + wifi_manager_local_ip()     + "\"}";
        json_ok(req, body);
    });

    // ---- GET /api/prefs ----------------------------------------------------
    // Vehicle preferences used by prefs.html to populate steer-limit sliders.
    server.on("/api/prefs", HTTP_GET, [](AsyncWebServerRequest* req) {
        String body = "{\"steer_min\":" + String(vehicle_get_steer_min()) +
                      ",\"steer_max\":" + String(vehicle_get_steer_max()) + "}";
        json_ok(req, body);
    });

    // ---- GET /ping ---------------------------------------------------------
    server.on("/ping", HTTP_GET, [](AsyncWebServerRequest* req) {
        json_ok(req, "{\"status\":\"ok\"}");
    });

    // ---- GET /status (legacy endpoint kept for backward compatibility) ------
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest* req) {
        String body = "{\"speed\":" + String(vehicle_get_speed()) +
                      ",\"turn\":"  + String(vehicle_get_turn())  + "}";
        json_ok(req, body);
    });

    // ---- GET /drive/<speed> ------------------------------------------------
    server.on("^/drive/(-?[0-9]+)$", HTTP_GET, [](AsyncWebServerRequest* req) {
        int speed = req->pathArg(0).toInt();
        Serial.printf("[web] GET /drive/%d\n", speed);
        vehicle_set_speed(speed);
        json_ok(req, "{\"speed\":" + String(vehicle_get_speed()) + "}");
    });

    // ---- GET /turn/<degrees> -----------------------------------------------
    server.on("^/turn/(-?[0-9]+)$", HTTP_GET, [](AsyncWebServerRequest* req) {
        int deg = req->pathArg(0).toInt();
        Serial.printf("[web] GET /turn/%d\n", deg);
        vehicle_set_turn(deg);
        json_ok(req, "{\"turn\":" + String(vehicle_get_turn()) + "}");
    });

    // ---- GET /stop ---------------------------------------------------------
    server.on("/stop", HTTP_GET, [](AsyncWebServerRequest* req) {
        Serial.println("[web] GET /stop");
        vehicle_stop();
        json_ok(req, "{\"status\":\"stopped\",\"speed\":0,\"turn\":0}");
    });

    // ---- POST /settings (save WiFi credentials) ----------------------------
    server.on("/settings", HTTP_POST, [](AsyncWebServerRequest* req) {
        String ssid, password;
        if (req->hasParam("ssid",     true)) ssid     = req->getParam("ssid",     true)->value();
        if (req->hasParam("password", true)) password = req->getParam("password", true)->value();
        req->send(200, "text/html",
            "<html><body style='font-family:sans-serif;background:#0d0f14;color:#e4e8f0;"
            "display:flex;align-items:center;justify-content:center;height:100vh;'>"
            "<p>Credentials saved. Rebooting&hellip;</p></body></html>");
        wifi_manager_save_and_reboot(ssid, password);
    });

    // ---- POST /settings/clear (reset to AP mode) ---------------------------
    server.on("/settings/clear", HTTP_POST, [](AsyncWebServerRequest* req) {
        req->send(200, "text/html",
            "<html><body style='font-family:sans-serif;background:#0d0f14;color:#e4e8f0;"
            "display:flex;align-items:center;justify-content:center;height:100vh;'>"
            "<p>WiFi credentials cleared. Rebooting into AP mode&hellip;</p></body></html>");
        wifi_manager_clear_credentials();
    });

    // ---- POST /prefs (save vehicle preferences) ----------------------------
    server.on("/prefs", HTTP_POST, [](AsyncWebServerRequest* req) {
        int min_deg = vehicle_get_steer_min();
        int max_deg = vehicle_get_steer_max();
        if (req->hasParam("steer_min", true))
            min_deg = req->getParam("steer_min", true)->value().toInt();
        if (req->hasParam("steer_max", true))
            max_deg = req->getParam("steer_max", true)->value().toInt();
        if (min_deg >= max_deg) {
            json_ok(req, "{\"ok\":false,\"error\":\"min must be less than max\"}");
            return;
        }
        vehicle_set_steer_limits(min_deg, max_deg);
        json_ok(req, "{\"ok\":true,\"steer_min\":" + String(vehicle_get_steer_min()) +
                     ",\"steer_max\":"              + String(vehicle_get_steer_max()) + "}");
    });

    // ---- 404 ---------------------------------------------------------------
    server.onNotFound([](AsyncWebServerRequest* req) {
        // Only report 404 for non-static-file paths
        if (req->method() == HTTP_GET) {
            // Let LittleFS try to serve it before giving up
            if (LittleFS.exists(req->url())) {
                req->send(LittleFS, req->url(), String(), false);
                return;
            }
        }
        req->send(404, "application/json", "{\"error\":\"not found\"}");
    });

    // ---- Static files from LittleFS (must be LAST) -------------------------
    // Serves drive.html, settings.html, prefs.html and any future assets.
    // /  →  drive.html (default)
    server.serveStatic("/", LittleFS, "/").setDefaultFile("drive.html");

    server.begin();
    Serial.println("[web] HTTP server started on port 80");
}
