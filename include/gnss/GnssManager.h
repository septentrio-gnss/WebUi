#pragma once

#include <Arduino.h>

void initGnss();
void sendCommandToReceiver(const String& command);
void startRequiredReceiverStreams();
void handleSerialParsing();
void handleConsoleFlush();