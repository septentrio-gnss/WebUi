#include "gnss/GnssManager.h"
#include "gnss/GnssParsers.h"

#include "AppGlobals.h"
#include "websocket/WebSocketManager.h"
#include "ntrip/NtripManager.h"

#include "utils/BootProfiler.h"

void initGnss() {
    Serial.printf("[GNSS] Serial2 RX GPIO = %d, TX GPIO = %d\n", MOSAIC_RX_PIN, MOSAIC_TX_PIN);

    Serial2.begin(115200, SERIAL_8N1, MOSAIC_RX_PIN, MOSAIC_TX_PIN);

    gnss.begin(Serial2);
    ntrip.begin(Serial2);
}

void sendCommandToReceiver(const String& command) {
    if (!gnssSerialReady) {
        Serial.printf(
            "[GNSS WARNING] Command ignored because GNSS Serial2 is not ready yet: %s\n",
            command.c_str()
        );
        return;
    }

    const unsigned long RTCM_RECENT_WINDOW_MS = 5000;

    bool rtcmWasRecentlyActive =
        (lastRtcmActivity != 0) &&
        ((millis() - lastRtcmActivity) < RTCM_RECENT_WINDOW_MS);

    if (rtcmWasRecentlyActive) {
        Serial.println("INFO: RTCM recently active. Forcing receiver into command mode.");

        Serial2.print("SSSSSSSSSS");
        Serial2.flush();

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    Serial2.print(command);
    Serial2.print("\r\n");

    // Wait until the complete command has physically left the UART.
    Serial2.flush();

    Serial.printf("Sent to Receiver: %s\n", command.c_str());
}

void startRequiredReceiverStreams() {
    Serial.println("INFO: Starting RxControl-like SBF/NMEA streams on COM2...");

    const TickType_t commandDelay = pdMS_TO_TICKS(250);

    // First disable existing streams.
    sendCommandToReceiver("sso,Stream1,COM2,,off");
    vTaskDelay(commandDelay);

    sendCommandToReceiver("sso,Stream2,COM2,,off");
    vTaskDelay(commandDelay);

    sendCommandToReceiver("sso,Stream3,COM2,,off");
    vTaskDelay(commandDelay);

    sendCommandToReceiver("sno,Stream4,COM2,,off");
    vTaskDelay(commandDelay);

    // Give the receiver additional time after disabling all streams.
    vTaskDelay(pdMS_TO_TICKS(500));

    // Enable required SBF streams.
    sendCommandToReceiver(
        "sso,Stream1,COM2,"
        "PVTGeodetic+QualityInd+RFStatus+ReceiverStatus+DOP+ReceiverTime,"
        "sec1"
    );
    vTaskDelay(commandDelay);

    sendCommandToReceiver(
        "sso,Stream2,COM2,"
        "SatVisibility+ChannelStatus+MeasEpoch+EndOfPVT,"
        "sec1"
    );
    vTaskDelay(commandDelay);

    sendCommandToReceiver(
        "sso,Stream3,COM2,"
        "PosCovGeodetic+VelCovGeodetic,"
        "sec1"
    );
    vTaskDelay(commandDelay);

    // Enable GGA.
    sendCommandToReceiver("sno,Stream4,COM2,GGA,sec1");
    vTaskDelay(commandDelay);

    Serial.println("INFO: Receiver stream configuration commands completed.");
}

void handleSerialParsing() {
    static String lineBuffer = "";
    static bool lineIsCorrupted = false;

    static uint32_t gnssBytesThisSecond = 0;
    static uint32_t sbfSyncThisSecond = 0;
    static uint32_t sbfParsedThisSecond = 0;
    static uint32_t nmeaParsedThisSecond = 0;
    static uint32_t ggaParsedThisSecond = 0;
    static uint32_t asciiCharsThisSecond = 0;
    static uint32_t consoleLinesThisSecond = 0;

    static uint8_t previousByte = 0;
    static unsigned long lastGnssDebug = 0;

    while (Serial2.available()) {
        static bool firstSerial2ByteLogged = false;
        
        uint8_t c = Serial2.read();

        if (!firstSerial2ByteLogged) {
            BOOT_LOG("First byte received from GNSS receiver on Serial2");
            firstSerial2ByteLogged = true;
        }


        gnssBytesThisSecond++;

        if (previousByte == '$' && c == '@') {
            sbfSyncThisSecond++;
        }

        previousByte = c;

        if (c >= 32 && c <= 126) {
            asciiCharsThisSecond++;
        }

        bool sbfReady = gnss.readSbf(c);
        bool nmeaReady = gnss.readNmea(c);

        if (sbfReady) {
            static bool firstSbfLogged = false;
            sbfParsedThisSecond++;

            if (!firstSbfLogged) {
                BOOT_LOG("First SBF message parsed");
                firstSbfLogged = true;
            }

            switch (gnss.SBFBuffer.block_id) {
                case SBF_ID_PVTGeodetic:
                    parseAndBroadcastPVT();
                    break;

                case SBF_ID_PosCovGeodetic:
                    parseAndBroadcastPosCov();
                    break;

                case SBF_ID_VelCovGeodetic:
                    parseAndBroadcastVelCov();
                    break;

                case SBF_ID_AttEuler:
                    parseAndBroadcastAttitude();
                    break;

                case SBF_ID_AttCovEuler:
                    parseAndBroadcastAttCov();
                    break;

                case SBF_ID_QualityInd:
                    parseAndBroadcastQuality();
                    break;

                case SBF_ID_SatVisibility:
                    parseAndBroadcastSatVisibility();
                    break;

                case SBF_ID_RFStatus:
                    parseAndBroadcastRFStatus();
                    break;

                case SBF_ID_MeasEpoch:
                    parseAndBroadcastMeasEpoch();
                    break;

                case SBF_ID_ChannelStatus:
                    parseAndBroadcastChannelStatus();
                    break;

                case SBF_ID_EndOfPVT:
                    broadcastFullSkyplot();
                    break;

                case SBF_ID_DOP:
                    parseAndBroadcastDOP();
                    break;

                case SBF_ID_ReceiverTime:
                    parseAndBroadcastTime();
                    break;

                case SBF_ID_ReceiverStatus:
                    parseAndBroadcastReceiverStatus();
                    break;

                default:
                    break;
            }
        }

        if (nmeaReady) {
            static bool firstNmeaLogged = false;

            nmeaParsedThisSecond++;

            if (!firstNmeaLogged) {
                BOOT_LOG("First NMEA message parsed");
                firstNmeaLogged = true;
            }


            String sentence = String(gnss.NMEABuffer.data);

            if (strcmp(gnss.NMEABuffer.messageID, "GGA") == 0) {
                static bool firstGgaLogged = false;

                ggaParsedThisSecond++;

                if (!firstGgaLogged) {
                    BOOT_LOG("First GGA sentence parsed");
                    firstGgaLogged = true;
                }
            }

            if (deviceConnected && BLUETOOTH_ENABLED && pTxCharacteristic) {
                pTxCharacteristic->setValue((uint8_t*)sentence.c_str(), sentence.length());
                pTxCharacteristic->notify();
            }

            if (strcmp(gnss.NMEABuffer.messageID, "GGA") == 0 && ntrip.isConnected()) {
                ntrip.updateGGA(sentence);
            }
        }

        if (c == '\n') {
            if (!lineIsCorrupted && lineBuffer.length() > 0) {
                lineBuffer.trim();

                if (lineBuffer.length() > 0) {
                    consoleLinesThisSecond++;
                    broadcastReplyToConsole(lineBuffer);
                }
            }

            lineBuffer = "";
            lineIsCorrupted = false;
        } else if (c == '\r') {
            continue;
        } else {
            if (isprint(c)) {
                if (!lineIsCorrupted && lineBuffer.length() < 200) {
                    lineBuffer += (char)c;
                }
            } else {
                lineIsCorrupted = true;
            }
        }
    }

    if (millis() - lastGnssDebug >= 5000) {
        Serial.printf(
            "[GNSS] bytes/s=%lu | SBF sync/s=%lu | SBF parsed/s=%lu | NMEA/s=%lu | GGA/s=%lu | ASCII/s=%lu | console lines/s=%lu\n",
            gnssBytesThisSecond,
            sbfSyncThisSecond,
            sbfParsedThisSecond,
            nmeaParsedThisSecond,
            ggaParsedThisSecond,
            asciiCharsThisSecond,
            consoleLinesThisSecond
        );

        gnssBytesThisSecond = 0;
        sbfSyncThisSecond = 0;
        sbfParsedThisSecond = 0;
        nmeaParsedThisSecond = 0;
        ggaParsedThisSecond = 0;
        asciiCharsThisSecond = 0;
        consoleLinesThisSecond = 0;

        lastGnssDebug = millis();
    }
}

void handleConsoleFlush() {
    if (millis() - lastConsoleSend > 100) {
        if (consoleAccumulator.length() > 0) {
            broadcastReplyToConsole(consoleAccumulator);
            consoleAccumulator = "";
        }

        lastConsoleSend = millis();
    }
}
