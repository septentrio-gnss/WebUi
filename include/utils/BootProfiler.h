#pragma once

#include <Arduino.h>

void bootLog(const char* message);
void bootLogValue(const char* message, unsigned long value);

#define BOOT_LOG(msg) bootLog(msg)
#define BOOT_LOG_VALUE(msg, value) bootLogValue(msg, value)