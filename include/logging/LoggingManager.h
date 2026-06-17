#pragma once

#include <ArduinoJson.h>

void handleStartLog(JsonDocument& doc);
void handleStopLogSbf();
void handleStopLogNmea();