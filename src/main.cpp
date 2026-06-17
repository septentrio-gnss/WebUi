/*
 * Authors:
 * - Ariel Kriss Sany
 * - Adham Ali
 *
 * Dualy WebUI Project - Modular ESP32 Firmware
 */

#include <Arduino.h>

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "AppGlobals.h"

#include "config/AppConfig.h"
#include "led/LedManager.h"
#include "web/WebServerManager.h"
#include "websocket/WebSocketManager.h"
#include "gnss/GnssManager.h"
#include "ntrip/NtripManager.h"
#include "ble/BleManager.h"

//#define BOOT_LOG(msg) Serial.printf("[BOOT +%lu ms] %s\n", millis(), msg)
#include "utils/BootProfiler.h"

TaskHandle_t Core0Task;

void Core0Loop(void * pvParameters) {
    BOOT_LOG("Core0 task started");
    Serial.println("Task on Core 0 started.");

    for (;;) {
        handleSerialParsing();
        handleConsoleFlush();
        handleBleStatusLog();

        if (currentMode == MODE_INTERNAL_NTRIP) {
            handleInternalNtrip();
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void handlePeriodicStatusUpdates() {
    if (millis() - lastStatusUpdateTime > 5000) {
        sendWifiStatus();
        lastStatusUpdateTime = millis();
    }
}

void printHeartbeat() {
    static unsigned long lastHeartbeat = 0;

    if (millis() - lastHeartbeat > 10000) {
        Serial.printf("[HEARTBEAT] Firmware running | Free heap: %u | Uptime: %lu ms\n",
                      ESP.getFreeHeap(),
                      millis());

        lastHeartbeat = millis();
    }
}

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    Serial.begin(115200);
    delay(300);

    Serial.println("\n\n===== Dualy ESP32 Booting Up =====");
    BOOT_LOG("Boot started");
    BOOT_LOG("Serial monitor ready");

    BOOT_LOG("Starting LED drivers");
    initLedDrivers();
    setBootLedPattern();
    BOOT_LOG("LED drivers initialized");

    BOOT_LOG("Creating WebSocket mutex");
    initWebSocketMutex();
    BOOT_LOG("WebSocket mutex created");

    BOOT_LOG("Mounting SPIFFS");
    initFileSystem();
    BOOT_LOG("SPIFFS mounted");

    BOOT_LOG("Loading saved credentials");
    loadCredentials();
    BOOT_LOG("Credentials loaded");

    BOOT_LOG("Initializing BLE");
    initBle();
    BOOT_LOG("BLE initialized");

    BOOT_LOG("Starting WiFi AP / STA / mDNS");
    initWiFiAndMdns();
    BOOT_LOG("WiFi AP / STA / mDNS initialized");

    BOOT_LOG("Starting HTTP server");
    initHttpServer();
    BOOT_LOG("HTTP server started");

    BOOT_LOG("Starting WebSocket server");
    initWebSocketServer();
    BOOT_LOG("WebSocket server started");

    BOOT_LOG("Initializing GNSS Serial2 and Septentrio libraries");
    initGnss();
    BOOT_LOG("GNSS/NTRIP libraries initialized");

    BOOT_LOG("Receiver stabilization delay started");
    delay(1000);
    BOOT_LOG("Receiver stabilization delay finished");

    BOOT_LOG("Requesting required SBF/NMEA receiver streams");
    startRequiredReceiverStreams();
    BOOT_LOG("Required SBF/NMEA streams requested");

    BOOT_LOG("Creating Core0 receiver task");
    xTaskCreatePinnedToCore(
        Core0Loop,
        "Core0Loop",
        10000,
        NULL,
        1,
        &Core0Task,
        0
    );
    BOOT_LOG("Core0 receiver task created");

    BOOT_LOG("Setup complete - main loop starting");
    Serial.println("===== Setup complete. Running main loop. =====");
}

void loop() {
    handleWebSocketLoop();

    updateApplicationLedStatus();
    updateLedOutputs();

    handlePeriodicStatusUpdates();

    printHeartbeat();
}