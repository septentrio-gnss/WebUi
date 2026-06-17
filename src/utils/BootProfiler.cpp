#include "utils/BootProfiler.h"

void bootLog(const char* message) {
    Serial.printf("[BOOT +%lu ms] %s\n", millis(), message);
}

void bootLogValue(const char* message, unsigned long value) {
    Serial.printf("[BOOT +%lu ms] %s: %lu\n", millis(), message, value);
}