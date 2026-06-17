#pragma once

#include <Arduino.h>
#include <map>

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <ESPmDNS.h>
#include <Wire.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <freertos/semphr.h>

#include "Septentrio_Arduino_driver.h"

// =====================================================
// Pins
// =====================================================
constexpr int LED_I2C_SDA = 9;
constexpr int LED_I2C_SCL = 10;

constexpr int MOSAIC_RX_PIN = 21;
constexpr int MOSAIC_TX_PIN = 12;

// =====================================================
// LED drivers
// =====================================================
constexpr uint8_t LED_SYSTEM_ADDR = 0x30;
constexpr uint8_t LED_GNSS_ADDR   = 0x31;

// =====================================================
// BLE UUIDs
// =====================================================
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// =====================================================
// SBF IDs
// =====================================================
constexpr uint16_t SBF_ID_PVTGeodetic    = 4007;
constexpr uint16_t SBF_ID_PosCovGeodetic = 5906;
constexpr uint16_t SBF_ID_VelCovGeodetic = 5908;
constexpr uint16_t SBF_ID_AttEuler       = 5938;
constexpr uint16_t SBF_ID_AttCovEuler    = 5939;
constexpr uint16_t SBF_ID_QualityInd     = 4082;
constexpr uint16_t SBF_ID_SatVisibility  = 4012;
constexpr uint16_t SBF_ID_RFStatus       = 4092;
constexpr uint16_t SBF_ID_ChannelStatus  = 4013;
constexpr uint16_t SBF_ID_MeasEpoch      = 4027;
constexpr uint16_t SBF_ID_EndOfPVT       = 5921;
constexpr uint16_t SBF_ID_DOP            = 4001;
constexpr uint16_t SBF_ID_ReceiverTime   = 5914;
constexpr uint16_t SBF_ID_ReceiverStatus = 4014;

// =====================================================
// App configuration
// =====================================================
extern String WIFI_SSID;
extern String WIFI_PASSWORD;
extern String NTRIP_HOST;
extern String NTRIP_MOUNT;
extern String NTRIP_USER;
extern String NTRIP_PASS;

extern int NTRIP_PORT;
extern bool WIFI_STA_ENABLED;
extern bool BLUETOOTH_ENABLED;

// =====================================================
// Global objects
// =====================================================
extern AsyncWebServer server;
extern WebSocketsServer webSocket;
extern Preferences preferences;

extern SEPTENTRIO_GNSS gnss;
extern SEPTENTRIO_NTRIP ntrip;

// =====================================================
// BLE state
// =====================================================
extern BLEServer* pServer;
extern BLECharacteristic* pTxCharacteristic;

extern bool deviceConnected;
extern bool oldDeviceConnected;
extern bool bleStackInitialized;

extern volatile size_t bleDataCounter;
extern unsigned long lastBleLogTime;

// =====================================================
// Runtime modes
// =====================================================
enum OperatingMode {
    MODE_IDLE,
    MODE_INTERNAL_NTRIP
};

extern OperatingMode currentMode;

// =====================================================
// Runtime status
// =====================================================
extern bool ntripUserRequestConnect;
extern SemaphoreHandle_t webSocketMutex;

extern unsigned long lastRtcmActivity;
extern unsigned long lastConsoleSend;
extern String consoleAccumulator;

extern bool isLogging;

enum LogFormat {
    LOG_NONE,
    LOG_SBF,
    LOG_NMEA
};

extern LogFormat currentLogFormat;
extern String logFilters;

extern bool currentJammingFiltersOn;
extern uint32_t lastGnssDataMs;
extern uint8_t currentFixCode;
extern bool systemCriticalError;
extern bool systemWarning;

extern unsigned long lastStatusUpdateTime;

// =====================================================
// Skyplot
// =====================================================
struct SatelliteInfo {
    uint8_t svid = 0;
    float azimuth = 0;
    float elevation = 0;
    float cn0 = 0;
    bool in_pvt = false;
    bool data_updated = false;
};

extern std::map<uint8_t, SatelliteInfo> skyplotData;