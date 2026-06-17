#include "config/AppConfig.h"
#include "AppGlobals.h"

void saveCredentials() {
    preferences.begin("wifi-creds", false);
    preferences.putBool("sta_enabled", WIFI_STA_ENABLED);
    preferences.putString("ssid", WIFI_SSID);
    preferences.putString("pass", WIFI_PASSWORD);
    preferences.end();

    preferences.begin("bt-creds", false);
    preferences.putBool("bt_enabled", BLUETOOTH_ENABLED);
    preferences.end();

    preferences.begin("ntrip-creds", false);
    preferences.putString("host", NTRIP_HOST);
    preferences.putInt("port", NTRIP_PORT);
    preferences.putString("mount", NTRIP_MOUNT);
    preferences.putString("user", NTRIP_USER);
    preferences.putString("pass", NTRIP_PASS);
    preferences.end();
}

void loadCredentials() {
    preferences.begin("wifi-creds", true);
    WIFI_SSID = preferences.getString("ssid", "");
    WIFI_PASSWORD = preferences.getString("pass", "");
    WIFI_STA_ENABLED = preferences.getBool("sta_enabled", false);
    preferences.end();

    preferences.begin("bt-creds", true);
    BLUETOOTH_ENABLED = preferences.getBool("bt_enabled", false);
    preferences.end();

    preferences.begin("ntrip-creds", true);
    NTRIP_HOST = preferences.getString("host", "");
    NTRIP_PORT = preferences.getInt("port", 2101);
    NTRIP_MOUNT = preferences.getString("mount", "");
    NTRIP_USER = preferences.getString("user", "");
    NTRIP_PASS = preferences.getString("pass", "");
    preferences.end();

    Serial.println("Loaded credentials:");
    Serial.println("  WIFI SSID: " + String(WIFI_SSID.length() > 0 ? WIFI_SSID : "<Not Set>"));
    Serial.println("  WIFI STA Enabled: " + String(WIFI_STA_ENABLED ? "Yes" : "No"));
    Serial.println("  NTRIP Host: " + String(NTRIP_HOST.length() > 0 ? NTRIP_HOST : "<Not Set>"));
}
