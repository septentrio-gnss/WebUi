#pragma once

#include <Arduino.h>

void handleInternalNtrip();
void fetchAndSendMountpoints(const String& host, int port);