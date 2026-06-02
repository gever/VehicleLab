#include "wifi_manager.h"
#include <Preferences.h>
#include <WiFi.h>

// ---------------------------------------------------------------------------
// wifi_manager.cpp
// ---------------------------------------------------------------------------

static const char* AP_SSID     = "VehicleLab-AP";
static const char* AP_PASSWORD = "12345678";       // min 8 chars for WPA2
static const int   STA_TIMEOUT_MS = 30000;         // 30 s connection timeout

static bool   s_sta_mode = false;
static String s_ssid;
static String s_password;

static Preferences prefs;

// ---------- helpers ----------------------------------------------------------

static void load_credentials() {
    prefs.begin("wifi", /*readOnly=*/true);
    s_ssid     = prefs.getString("ssid",     "");
    s_password = prefs.getString("password", "");
    prefs.end();
}

static void start_ap_mode() {
    Serial.println("[wifi] Starting AP mode...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    Serial.printf("[wifi] AP ready  SSID: %s  IP: %s\n",
                  AP_SSID, WiFi.softAPIP().toString().c_str());
    s_sta_mode = false;
}

static bool try_sta_mode(const String& ssid, const String& password) {
    Serial.printf("[wifi] Connecting to \"%s\"...\n", ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long deadline = millis() + STA_TIMEOUT_MS;
    while (WiFi.status() != WL_CONNECTED && millis() < deadline) {
        delay(500);
        Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[wifi] Connected!  IP: %s\n",
                      WiFi.localIP().toString().c_str());
        s_sta_mode = true;
        return true;
    }

    Serial.println("[wifi] Connection failed – falling back to AP mode.");
    return false;
}

// ---------- public -----------------------------------------------------------

void wifi_manager_begin() {
    WiFi.setHostname("vehiclelab");
    load_credentials();

    if (s_ssid.length() == 0) {
        start_ap_mode();
        return;
    }

    if (!try_sta_mode(s_ssid, s_password)) {
        start_ap_mode();
    }
}

void wifi_manager_save_and_reboot(const String& ssid, const String& password) {
    prefs.begin("wifi", /*readOnly=*/false);
    prefs.putString("ssid",     ssid);
    prefs.putString("password", password);
    prefs.end();
    Serial.printf("[wifi] Credentials saved (SSID: \"%s\"). Rebooting...\n",
                  ssid.c_str());
    delay(500);
    ESP.restart();
}

void wifi_manager_clear_credentials() {
    prefs.begin("wifi", /*readOnly=*/false);
    prefs.remove("ssid");
    prefs.remove("password");
    prefs.end();
    Serial.println("[wifi] Credentials cleared. Rebooting...");
    delay(500);
    ESP.restart();
}

bool   wifi_manager_is_sta_mode()   { return s_sta_mode; }
String wifi_manager_current_ssid()  { return s_sta_mode ? s_ssid : String(AP_SSID); }
String wifi_manager_local_ip() {
    return s_sta_mode ? WiFi.localIP().toString()
                      : WiFi.softAPIP().toString();
}
