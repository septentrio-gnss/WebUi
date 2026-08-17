#include "ntrip/NtripManager.h"
#include "AppGlobals.h"
#include "websocket/WebSocketManager.h"
#include "utils/BootProfiler.h"
#include <WiFi.h>
#include <mbedtls/base64.h>
#include <vector>

static String makeBasicAuthorization(
    const String& user,
    const String& password
) {
    if (user.isEmpty()) {
        return "";
    }

    const String credentials = user + ":" + password;

    const size_t outputCapacity =
        4 * ((credentials.length() + 2) / 3) + 1;

    std::vector<unsigned char> encoded(outputCapacity);

    size_t encodedLength = 0;

    const int result = mbedtls_base64_encode(
        encoded.data(),
        encoded.size(),
        &encodedLength,
        reinterpret_cast<const unsigned char*>(
            credentials.c_str()
        ),
        credentials.length()
    );

    if (result != 0) {
        return "";
    }

    encoded[encodedLength] = '\0';

    return String(
        reinterpret_cast<const char*>(encoded.data())
    );
}


static void sendMountpointError(
    uint8_t clientNumber,
    const String& message
) {
    JsonDocument doc;

    doc["type"] = "mountpoints_list";
    doc["success"] = false;
    doc["message"] = message;
    doc["data"].to<JsonArray>();

    String output;
    serializeJson(doc, output);

    // Called from the WebSocket event callback:
    // do not call broadcastJson() here.
    webSocket.sendTXT(clientNumber, output);
}


void fetchAndSendMountpoints(
    uint8_t clientNumber,
    const String& host,
    int port,
    const String& user,
    const String& password
) {
    if (WiFi.status() != WL_CONNECTED) {
        sendMountpointError(
            clientNumber,
            "No Internet Wi-Fi connection."
        );
        return;
    }

    WiFiClient client;
    client.setTimeout(2000);

    Serial.printf(
        "[NTRIP] Fetching sourcetable from %s:%d\n",
        host.c_str(),
        port
    );

    if (!client.connect(host.c_str(), port)) {
        Serial.println(
            "[NTRIP] Sourcetable TCP connection failed."
        );

        sendMountpointError(
            clientNumber,
            "Could not connect to the NTRIP caster."
        );

        return;
    }

    const String authorization =
        makeBasicAuthorization(user, password);

    String request;

    request += "GET / HTTP/1.0\r\n";
    request += "Host: " + host + "\r\n";
    request += "User-Agent: NTRIP WebUI-ESP32/1.0\r\n";
    request += "Ntrip-Version: Ntrip/2.0\r\n";

    if (!authorization.isEmpty()) {
        request +=
            "Authorization: Basic " +
            authorization +
            "\r\n";
    }

    request += "Connection: close\r\n";
    request += "\r\n";

    client.print(request);

    const unsigned long responseStart = millis();

    while (client.available() == 0) {
        if (millis() - responseStart > 7000) {
            client.stop();

            Serial.println(
                "[NTRIP] Sourcetable response timeout."
            );

            sendMountpointError(
                clientNumber,
                "NTRIP caster response timed out."
            );

            return;
        }

        delay(10);
    }

    String statusLine = client.readStringUntil('\n');
    statusLine.trim();

    Serial.printf(
        "[NTRIP] Sourcetable response: %s\n",
        statusLine.c_str()
    );

    const bool responseAccepted =
        statusLine.startsWith("SOURCETABLE 200") ||
        statusLine.startsWith("HTTP/1.0 200") ||
        statusLine.startsWith("HTTP/1.1 200");

    if (!responseAccepted) {
        client.stop();

        if (statusLine.indexOf("401") >= 0) {
            sendMountpointError(
                clientNumber,
                "Authentication failed. Check the FLEPOS user and password."
            );
        } else if (statusLine.indexOf("403") >= 0) {
            sendMountpointError(
                clientNumber,
                "Access to the FLEPOS sourcetable was refused."
            );
        } else {
            sendMountpointError(
                clientNumber,
                "Caster rejected the sourcetable request: " +
                statusLine
            );
        }

        return;
    }

    JsonDocument doc;

    doc["type"] = "mountpoints_list";
    doc["success"] = true;

    JsonArray mountpoints =
        doc["data"].to<JsonArray>();

    unsigned long lastNetworkActivity = millis();

    while (
        client.connected() ||
        client.available() > 0
    ) {
        if (client.available() == 0) {
            if (
                millis() - lastNetworkActivity > 5000
            ) {
                break;
            }

            delay(10);
            continue;
        }

        String line = client.readStringUntil('\n');
        line.trim();

        lastNetworkActivity = millis();

        /*
         * Do not depend on a blank line separating headers
         * from the sourcetable. Parse every STR record found.
         */
        if (line.startsWith("STR;")) {
            const int firstSeparator =
                line.indexOf(';');

            const int secondSeparator =
                line.indexOf(
                    ';',
                    firstSeparator + 1
                );

            if (
                firstSeparator >= 0 &&
                secondSeparator > firstSeparator + 1
            ) {
                const String mountpoint =
                    line.substring(
                        firstSeparator + 1,
                        secondSeparator
                    );

                mountpoints.add(mountpoint);
            }
        }

        if (line == "ENDSOURCETABLE") {
            break;
        }
    }

    client.stop();

    const size_t count = mountpoints.size();

    Serial.printf(
        "[NTRIP] Found %u mountpoints.\n",
        static_cast<unsigned int>(count)
    );

    if (count == 0) {
        doc["success"] = false;
        doc["message"] =
            "The caster returned no accessible mountpoints.";
    } else {
        doc["message"] =
            String(count) +
            " mountpoints loaded.";
    }

    String output;
    serializeJson(doc, output);

    /*
     * Important: do not call broadcastJson() here.
     * The WebSocket mutex is already held by webSocket.loop().
     */
    webSocket.sendTXT(clientNumber, output);
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