#pragma once

#include <Arduino.h>

void handleInternalNtrip();
void fetchAndSendMountpoints(
    uint8_t clientNumber,
    const String& host,
    int port,
    const String& user,
    const String& password
);