#include "websocket/WebSocketManager.h"
#include "AppGlobals.h"

#include "config/AppConfig.h"
#include "gnss/GnssManager.h"
#include "ntrip/NtripManager.h"
#include "ble/BleManager.h"
#include "logging/LoggingManager.h"
#include "utils/BootProfiler.h"

void initWebSocketMutex() {
    webSocketMutex = xSemaphoreCreateMutex();

    if (webSocketMutex == nullptr) {
        Serial.println("CRITICAL: WebSocket mutex creation failed! Halting.");
        while (1) delay(1000);
    }
}

void initWebSocketServer() {
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
}

void handleWebSocketLoop() {
    if (webSocketMutex && xSemaphoreTake(webSocketMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        webSocket.loop();
        xSemaphoreGive(webSocketMutex);
    }
}

void broadcastJson(String &output) {
    static bool firstBroadcastLogged = false;

    if (!webSocketMutex) return;

    if (!firstBroadcastLogged) {
        BOOT_LOG("First WebSocket JSON broadcast attempted");
        firstBroadcastLogged = true;
    }

    if (xSemaphoreTake(webSocketMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        webSocket.broadcastTXT(output);
        xSemaphoreGive(webSocketMutex);
    }
}

void broadcastNtripStatus() {
    JsonDocument doc;
    doc["type"] = "status";
    doc["connected"] = ntrip.isConnected();

    String output;
    serializeJson(doc, output);
    broadcastJson(output);
}

void broadcastRtcm(const String& rtcmStatus) {
    JsonDocument doc;
    doc["type"] = "rtcm";
    doc["data"] = rtcmStatus;

    String output;
    serializeJson(doc, output);
    broadcastJson(output);
}

void broadcastReplyToConsole(const String& reply) {
    if (reply.length() == 0) return;

    JsonDocument doc;
    doc["type"] = "raw_receiver_data";
    doc["data"] = reply;

    String output;
    serializeJson(doc, output);
    broadcastJson(output);
}

void sendWifiStatus() {
    JsonDocument doc;
    doc["type"] = "wifi_status";
    doc["connected"] = (WiFi.status() == WL_CONNECTED);

    String output;
    serializeJson(doc, output);
    broadcastJson(output);
}

void sendLoggingStatus(uint8_t num) {
    JsonDocument doc;
    doc["type"] = "logging_status";
    doc["enabled"] = isLogging;

    if (isLogging) {
        doc["format"] = (currentLogFormat == LOG_SBF) ? "SBF" : "NMEA";
    } else {
        doc["format"] = "NONE";
    }

    String output;
    serializeJson(doc, output);

    if (!webSocketMutex) return;

    if (xSemaphoreTake(webSocketMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (num == 255) webSocket.broadcastTXT(output);
        else webSocket.sendTXT(num, output);

        xSemaphoreGive(webSocketMutex);
    }
}

void sendJammingStatus() {
    JsonDocument doc;
    doc["type"] = "jamming_status";
    doc["filters_on"] = currentJammingFiltersOn;

    String output;
    serializeJson(doc, output);
    broadcastJson(output);
}

void sendCurrentPositionModes() {
    JsonDocument doc;
    doc["type"] = "current_pos_modes";

    JsonArray modes = doc["modes"].to<JsonArray>();
    modes.add("RTK");
    modes.add("StandAlone");

    String output;
    serializeJson(doc, output);
    broadcastJson(output);
}

void sendStatusUpdate() {
    JsonDocument doc;
    doc["type"] = "full_status";
    doc["wifi_connected"] = (WiFi.status() == WL_CONNECTED);
    doc["ntrip_connected"] = ntrip.isConnected();
    doc["bt_enabled"] = BLUETOOTH_ENABLED;
    doc["bt_connected"] = deviceConnected;
    doc["logging"] = isLogging;
    doc["jamming_ok"] = currentJammingFiltersOn;
    doc["rtcm_active"] = (millis() - lastRtcmActivity < 3000);
    doc["error"] = systemCriticalError;
    doc["warning"] = systemWarning;

    String output;
    serializeJson(doc, output);
    broadcastJson(output);
}

static void sendCurrentConfig(uint8_t num) {
    JsonDocument cfgDoc;

    cfgDoc["type"] = "current_config";
    cfgDoc["wifi_ssid"] = WIFI_SSID;
    cfgDoc["wifi_sta_enabled"] = WIFI_STA_ENABLED;
    cfgDoc["ntrip_host"] = NTRIP_HOST;
    cfgDoc["ntrip_port"] = NTRIP_PORT;
    cfgDoc["ntrip_mount"] = NTRIP_MOUNT;
    cfgDoc["ntrip_user"] = NTRIP_USER;
    cfgDoc["bt_enabled"] = BLUETOOTH_ENABLED;

    String configJson;
    serializeJson(cfgDoc, configJson);
    webSocket.sendTXT(num, configJson);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] WebSocket Disconnected!\n", num);
            Serial.println("INFO: WebSocket disconnected. Receiver streams kept active.");
            break;

        case WStype_CONNECTED: {
            static bool firstWebSocketClientLogged = false;

            if (!firstWebSocketClientLogged) {
                BOOT_LOG("First WebSocket client connected");
                firstWebSocketClientLogged = true;
            }
            IPAddress ip = webSocket.remoteIP(num);
            Serial.printf("[%u] WS Connection from %s\n", num, ip.toString().c_str());

            // startRequiredReceiverStreams(); 
            // Removed from WebSocket connection to avoid reconfiguring receiver streams
            // while GNSS startup is handled by the Core0 background task.

            broadcastNtripStatus();
            sendWifiStatus();
            sendLoggingStatus(num);
            sendJammingStatus();
            sendCurrentPositionModes();
            sendStatusUpdate();
            break;
        }

        case WStype_TEXT: {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload, length);

            if (error) {
                if (length < 64 && length > 0) {
                    char cmdBuf[length + 1];
                    memcpy(cmdBuf, payload, length);
                    cmdBuf[length] = '\0';

                    Serial.printf("[%u] Rx CMD: %s\n", num, cmdBuf);

                    if (strcmp(cmdBuf, "NTRIP_CONNECT") == 0) {
                        currentMode = MODE_INTERNAL_NTRIP;
                        ntripUserRequestConnect = true;
                    } else if (strcmp(cmdBuf, "NTRIP_DISCONNECT") == 0) {
                        ntripUserRequestConnect = false;
                        ntrip.disconnect();
                        broadcastNtripStatus();
                    } else if (strcmp(cmdBuf, "get_config") == 0) {
                        sendCurrentConfig(num);
                    } else if (strcmp(cmdBuf, "get_current_pos_modes") == 0) {
                        sendCommandToReceiver("getPVTMode");
                        sendCurrentPositionModes();
                    } else {
                        Serial.printf("[%u] Unknown CMD: %s\n", num, cmdBuf);
                    }
                } else {
                    Serial.printf("[%u] Invalid CMD (len %d)\n", num, length);
                }

                return;
            }

            const char* msgType = doc["type"];
            if (!msgType) {
                Serial.printf("[%u] JSON missing 'type'\n", num);
                return;
            }

            Serial.printf("[%u] Rx JSON: %s\n", num, msgType);

            if (strcmp(msgType, "fetch_mountpoints") == 0) {
                if (!doc["host"].isNull() && !doc["port"].isNull()) {
                    fetchAndSendMountpoints(doc["host"], doc["port"]);
                }
            }

            else if (strcmp(msgType, "wifi_config") == 0) {
                WIFI_SSID = doc["ssid"].as<String>();
                WIFI_PASSWORD = doc["pass"].as<String>();
                WIFI_STA_ENABLED = doc["enabled"].as<bool>();

                saveCredentials();

                Serial.println("WiFi saved. Rebooting...");
                delay(1000);
                ESP.restart();
            }

            else if (strcmp(msgType, "ntrip_config") == 0) {
                NTRIP_HOST = doc["host"].as<String>();
                NTRIP_PORT = doc["port"].as<int>();
                NTRIP_MOUNT = doc["mount"].as<String>();
                NTRIP_USER = doc["user"].as<String>();
                NTRIP_PASS = doc["pass"].as<String>();

                saveCredentials();

                Serial.println("NTRIP saved.");

                JsonDocument ackDoc;
                ackDoc["type"] = "config_ack";
                ackDoc["message"] = "NTRIP configuration saved!";

                String ack;
                serializeJson(ackDoc, ack);
                webSocket.sendTXT(num, ack);
            }

            else if (strcmp(msgType, "command") == 0) {
                String command = doc["data"];

                if (command.length() > 0 && command.length() < 100) {
                    Serial.printf("Sending cmd: %s\n", command.c_str());
                    sendCommandToReceiver(command);
                }
            }

            else if (strcmp(msgType, "start_log") == 0) {
                handleStartLog(doc);
            }

            else if (strcmp(msgType, "stop_log_sbf") == 0) {
                handleStopLogSbf();
            }

            else if (strcmp(msgType, "stop_log_nmea") == 0) {
                handleStopLogNmea();
            }

            else if (strcmp(msgType, "set_positioning_modes") == 0) {
                JsonArray modes = doc["modes"];
                String command = "setPVTMode, Rover, ";

                bool firstMode = true;

                for (JsonVariant v : modes) {
                    if (!firstMode) command += "+";
                    command += v.as<String>();
                    firstMode = false;
                }

                if (firstMode) command += "StandAlone";

                Serial.printf("Setting modes: %s\n", command.c_str());
                sendCommandToReceiver(command);

                delay(200);
                sendCommandToReceiver("getPVTMode");
            }

            else if (strcmp(msgType, "get_current_pos_modes") == 0) {
                sendCommandToReceiver("getPVTMode");
                sendCurrentPositionModes();
            }

            else if (strcmp(msgType, "save_bt") == 0) {
                bool wantEnabled = doc["enabled"];

                Serial.printf("DEBUG: save_bt received. Request: %s\n", wantEnabled ? "ON" : "OFF");

                if (wantEnabled && ntrip.isConnected()) {
                    ntrip.disconnect();
                    ntripUserRequestConnect = false;
                    Serial.println("DEBUG: Disconnecting NTRIP for BLE priority");
                }

                updateBluetoothState(wantEnabled);

                JsonDocument ack;
                ack["type"] = "config_ack";
                ack["message"] = wantEnabled ? "Bluetooth ON (Visible)" : "Bluetooth OFF (Hidden)";

                String out;
                serializeJson(ack, out);
                webSocket.sendTXT(num, out);
            }

            else {
                Serial.printf("[%u] Unknown JSON type: %s\n", num, msgType);
            }

            break;
        }

        default:
            break;
    }
}
