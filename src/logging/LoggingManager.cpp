#include "logging/LoggingManager.h"
#include "AppGlobals.h"
#include "gnss/GnssManager.h"
#include "websocket/WebSocketManager.h"

void handleStartLog(JsonDocument& doc) {
    String format = doc["format"];
    logFilters = doc["filters"].as<String>();

    logFilters.replace(" ", "");

    String filterString = logFilters;
    filterString.replace(",", "+");

    if (filterString.length() == 0) {
        filterString = "all";
    }

    String cmdStream = "";

    if (format.equalsIgnoreCase("SBF")) {
        cmdStream = "sso,Stream7,DSK1," + filterString + ",sec1";
        currentLogFormat = LOG_SBF;
    } else {
        cmdStream = "sno,Stream8,DSK1," + filterString + ",sec1";
        currentLogFormat = LOG_NMEA;
    }

    isLogging = true;

    Serial.println("---------------------------------");
    Serial.println("[LOG DEBUG] STARTING LOG...");
    Serial.printf("[LOG DEBUG] > G5 CMD: %s\n", cmdStream.c_str());
    Serial.println("---------------------------------");

    sendCommandToReceiver(cmdStream);
    sendLoggingStatus();
}

void handleStopLogSbf() {
    String cmdStopSBF = "sso,Stream7,DSK1, ,off";

    Serial.println("---------------------------------");
    Serial.println("[LOG DEBUG] STOPPING SBF LOG...");
    Serial.printf("[LOG DEBUG] > G5 CMD: %s\n", cmdStopSBF.c_str());
    Serial.println("---------------------------------");

    sendCommandToReceiver(cmdStopSBF);

    isLogging = false;
    currentLogFormat = LOG_NONE;
    sendLoggingStatus();
}

void handleStopLogNmea() {
    String cmdStopNMEA = "sno,Stream8,DSK1, ,off";

    Serial.println("---------------------------------");
    Serial.println("[LOG DEBUG] STOPPING NMEA LOG...");
    Serial.printf("[LOG DEBUG] > G5 CMD: %s\n", cmdStopNMEA.c_str());
    Serial.println("---------------------------------");

    sendCommandToReceiver(cmdStopNMEA);

    isLogging = false;
    currentLogFormat = LOG_NONE;
    sendLoggingStatus();
}
