/*
 * Authors:
 * - Adham Ali
 * - Ariel Kriss Sany
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
#include "BoardFeatures.h"

//#define BOOT_LOG(msg) Serial.printf("[BOOT +%lu ms] %s\n", millis(), msg)
#include "utils/BootProfiler.h"

TaskHandle_t Core0Task;
TaskHandle_t BleInitTaskHandle = NULL;

void Core0Loop(void * pvParameters) {
    BOOT_LOG("Core0 receiver task started");
    Serial.println("Task on Core 0 started.");

    // ============================================================
    // GNSS BACKGROUND STARTUP
    // This no longer blocks the WebUI startup path.
    // ============================================================

    BOOT_LOG("GNSS background startup started");

    BOOT_LOG("Initializing GNSS Serial2 and Septentrio libraries");
    initGnss();
    gnssSerialReady = true;
    BOOT_LOG("GNSS/NTRIP libraries initialized");

    BOOT_LOG("Receiver stabilization delay started in background");
    vTaskDelay(pdMS_TO_TICKS(1000));
    BOOT_LOG("Receiver stabilization delay finished in background");

    BOOT_LOG("Requesting required SBF/NMEA receiver streams in background");
    startRequiredReceiverStreams();
    receiverStreamsReady = true;
    BOOT_LOG("Required SBF/NMEA streams requested in background");

    BOOT_LOG("GNSS background startup complete");

    // ============================================================
    // RUNTIME LOOP
    // ============================================================

    for (;;) {
        handleSerialParsing();
        handleConsoleFlush();
        if (bleReady) { //This avoids calling BLE-related logic before BLE is initialized.
            handleBleStatusLog();
        }
        if (currentMode == MODE_INTERNAL_NTRIP && receiverStreamsReady) {
            static bool firstNtripRuntimeLogged = false;

            if (!firstNtripRuntimeLogged) {
                BOOT_LOG("NTRIP runtime loop enabled in background");
                firstNtripRuntimeLogged = true;
            }

            unsigned long ntripStart = millis();

            handleInternalNtrip();

            unsigned long ntripDuration = millis() - ntripStart;

            if (ntripDuration > 100) {
                Serial.printf("[NTRIP WARNING] handleInternalNtrip() took %lu ms\n", ntripDuration);
            }
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

void BleInitTask(void *pvParameters) {
    BOOT_LOG("BLE init background task started");

    /*
     * Small delay to let setup() finish and let the WebUI loop become active.
     * BLE is useful, but it is not required to display the WebUI.
     */
    vTaskDelay(pdMS_TO_TICKS(500));

    bleInitStarted = true;

    BOOT_LOG("BLE initialization started in background");
    initBle();
    bleReady = true;
    BOOT_LOG("BLE initialization finished in background");

    vTaskDelete(NULL);
}

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    Serial.begin(115200);
    delay(300);

    Serial.println("\n\n===== WebUI ESP32 Booting Up =====");
    BOOT_LOG("Boot started");
    BOOT_LOG("Serial monitor ready");

    // ============================================================
    // WEBUI CRITICAL PATH
    // ============================================================

    BOOT_LOG("WEBUI CRITICAL PATH START");

    BOOT_LOG("Creating WebSocket mutex");
    initWebSocketMutex();
    BOOT_LOG("WebSocket mutex created");

    BOOT_LOG("Mounting SPIFFS");
    initFileSystem();
    BOOT_LOG("SPIFFS mounted");

    BOOT_LOG("Loading saved credentials");
    loadCredentials();
    BOOT_LOG("Credentials loaded");

    BOOT_LOG("Starting WiFi AP / STA / mDNS");
    initWiFiAndMdns();
    BOOT_LOG("WiFi AP / STA / mDNS initialized");

    BOOT_LOG("Starting HTTP server");
    initHttpServer();
    BOOT_LOG("HTTP server started");

    BOOT_LOG("Starting WebSocket server");
    initWebSocketServer();
    BOOT_LOG("WebSocket server started");

    BOOT_LOG("WEBUI CRITICAL PATH READY");

    BOOT_LOG("Creating delayed STA task");
    xTaskCreatePinnedToCore(
       delayedStaTask,
       "DelayedStaTask",
       6000,
       NULL,
       1,
       NULL,
       1
    );
    BOOT_LOG("Delayed STA task created");

    // ============================================================
    // BACKGROUND RECEIVER TASK
    // GNSS startup is now moved out of setup().
    // ============================================================

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

    // ============================================================
    // POST-WEBUI SERVICES
    // These are still after WebUI.
    // BLE can be moved to its own task in Task 6.
    // ============================================================

    BOOT_LOG("POST-WEBUI SERVICES START");

    #if WEBUI_HAS_KTD2026

        BOOT_LOG("Starting KTD2026 LED drivers");

        initLedDrivers();
        setBootLedPattern();

        BOOT_LOG("KTD2026 LED drivers initialized");

    #else

        BOOT_LOG("KTD2026 LED drivers not available on this ESP32 target");

    #endif

    BOOT_LOG("Creating BLE init background task");

    xTaskCreatePinnedToCore(
        BleInitTask,
        "BleInitTask",
        8192,
        NULL,
        1,
        &BleInitTaskHandle,
        1
    );

    BOOT_LOG("BLE init background task created");

    BOOT_LOG("POST-WEBUI SERVICES DONE");

    BOOT_LOG("Setup complete - main loop starting");
    Serial.println("===== Setup complete. Running main loop. =====");
}

void loop() {
    handleWebSocketLoop();

#if WEBUI_HAS_KTD2026
    updateApplicationLedStatus();
    updateLedOutputs();
#endif

    handlePeriodicStatusUpdates();
    printHeartbeat();
}