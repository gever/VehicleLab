#pragma once

// ---------------------------------------------------------------------------
// wifi_manager.h – AP / STA mode WiFi initialisation and NVS credential
// storage using the Arduino Preferences library.
// ---------------------------------------------------------------------------

#include <Arduino.h>

// Boot-time WiFi setup.  Loads credentials from NVS; starts STA mode if
// credentials exist (with AP fallback on failure), otherwise starts AP mode.
void wifi_manager_begin();

// Persist new credentials to NVS and reboot.
void wifi_manager_save_and_reboot(const String& ssid, const String& password);

// Clear saved credentials from NVS (reverts to AP mode on next boot).
void wifi_manager_clear_credentials();

// Returns true when running in STA mode (connected to a router).
bool wifi_manager_is_sta_mode();

// Human-readable connection info for the /settings page.
String wifi_manager_current_ssid();
String wifi_manager_local_ip();
