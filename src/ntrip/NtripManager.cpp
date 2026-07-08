#include "ntrip/NtripManager.h"
#include "AppGlobals.h"
#include "websocket/WebSocketManager.h"
#include "utils/BootProfiler.h"
#include <WiFi.h>

void fetchAndSendMountpoints(const String& host, int port) {
    WiFiClient client;

    Serial.printf("Fetching sourcetable from %s:%d\n", host.c_str(), port);

    auto sendEmptyList = []() {
        JsonDocument doc;
        doc["type"] = "mountpoints_list";
        doc["data"].to<JsonArray>();

        String output;
        serializeJson(doc, output);
        broadcastJson(output);
    };

    if (!client.connect(host.c_str(), port)) {
        Serial.println("-> Connection failed.");
        sendEmptyList();
        return;
    }

    client.print("GET / HTTP/1.0\r\nUser-Agent: Dualy-ESP32-Fetcher/1.0\r\nHost: " + host + "\r\nConnection: close\r\n\r\n");

    unsigned long timeout = millis();

    while (client.available() == 0) {
        if (millis() - timeout > 7000) {
            Serial.println("-> Sourcetable timeout.");
            client.stop();
            sendEmptyList();
            return;
        }

        delay(10);
    }

    JsonDocument doc;
    doc["type"] = "mountpoints_list";

    JsonArray mountpoints = doc["data"].to<JsonArray>();
    bool headersEnded = false;

    while (client.connected() || client.available()) {
        String line = client.readStringUntil('\n');
        line.trim();

        if (!headersEnded) {
            if (line.length() == 0) headersEnded = true;
            continue;
        }

        if (line.startsWith("STR;")) {
            int firstSemi = line.indexOf(';');
            int secondSemi = line.indexOf(';', firstSemi + 1);

            if (secondSemi > firstSemi) {
                if (mountpoints.add(line.substring(firstSemi + 1, secondSemi)) == false) {
                    Serial.println("WARN: Mountpoint JSON document full. Stopping parse.");
                    break;
                }
            }
        } else if (line == "ENDSOURCETABLE") {
            break;
        }
    }

    Serial.printf("-> Found %d mountpoints.\n", mountpoints.size());

    String output;
    serializeJson(doc, output);
    broadcastJson(output);

    client.stop();
}

void handleInternalNtrip() {
    static unsigned long lastConnectAttempt = 0;
    static bool wasConnected = false;

    if (!gnssSerialReady || !receiverStreamsReady) {
        return;
    }

    if (ntripUserRequestConnect && !ntrip.isConnected()) {
        if (WiFi.status() != WL_CONNECTED) {
            static unsigned long lastNoWifiLog = 0;

            if (millis() - lastNoWifiLog > 5000) {
                Serial.println("[NTRIP] Waiting for WiFi STA connection before NTRIP connect...");
                lastNoWifiLog = millis();
            }

            return;
        }

        if (millis() - lastConnectAttempt < 10000) {
            return;
        }

        lastConnectAttempt = millis();

        static bool firstNtripAttemptLogged = false;

        if (!firstNtripAttemptLogged) {
            BOOT_LOG("First NTRIP connection attempt");
            firstNtripAttemptLogged = true;
        }

        Serial.printf("[NTRIP] Connecting to %s:%d / %s...\n",
                      NTRIP_HOST.c_str(),
                      NTRIP_PORT,
                      NTRIP_MOUNT.c_str());

        if (!ntrip.connect(NTRIP_HOST, NTRIP_PORT, NTRIP_MOUNT, NTRIP_USER, NTRIP_PASS)) {
            Serial.println("[NTRIP] Connect failed. Keeping auto-connect request active; retry in 10 s.");
            broadcastNtripStatus();
            return;
        }

        Serial.println("[NTRIP] Connected.");
        wasConnected = true;
        broadcastNtripStatus();
    }

    if (ntrip.isConnected()) {
        size_t rtcmBytes = ntrip.loop();

        if (rtcmBytes > 0) {
            static bool firstRtcmLogged = false;

            if (!firstRtcmLogged) {
                BOOT_LOG("First RTCM bytes received from NTRIP");
                firstRtcmLogged = true;
            }

            broadcastRtcm("[RTCM via WiFi] " + String(rtcmBytes) + " bytes Rx.");
            lastRtcmActivity = millis();
        }
    }

    if (wasConnected && ntripUserRequestConnect && !ntrip.isConnected()) {
        Serial.println("[NTRIP] Socket closed. Auto-connect request remains active; retry will continue.");
        wasConnected = false;
        broadcastNtripStatus();
    }
}