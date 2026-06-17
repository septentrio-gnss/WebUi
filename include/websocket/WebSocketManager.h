#pragma once

#include <Arduino.h>
#include <WebSocketsServer.h>

void initWebSocketMutex();
void initWebSocketServer();
void handleWebSocketLoop();

void broadcastJson(String& output);

void broadcastNtripStatus();
void broadcastRtcm(const String& rtcmStatus);
void broadcastReplyToConsole(const String& reply);

void sendWifiStatus();
void sendLoggingStatus(uint8_t num = 255);
void sendJammingStatus();
void sendCurrentPositionModes();
void sendStatusUpdate();

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);