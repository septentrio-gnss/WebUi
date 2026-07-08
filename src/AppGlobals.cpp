#include "AppGlobals.h"

String WIFI_SSID = "";
String WIFI_PASSWORD = "";
String NTRIP_HOST = "";
String NTRIP_MOUNT = "";
String NTRIP_USER = "";
String NTRIP_PASS = "";

int NTRIP_PORT = 2101;
bool NTRIP_AUTO_CONNECT = true;
bool WIFI_STA_ENABLED = false;
bool BLUETOOTH_ENABLED = false;

AsyncWebServer server(80);
WebSocketsServer webSocket(81);
Preferences preferences;

SEPTENTRIO_GNSS gnss;
SEPTENTRIO_NTRIP ntrip;

BLEServer* pServer = nullptr;
BLECharacteristic* pTxCharacteristic = nullptr;

bool deviceConnected = false;
bool oldDeviceConnected = false;
bool bleStackInitialized = false;

volatile size_t bleDataCounter = 0;
unsigned long lastBleLogTime = 0;

OperatingMode currentMode = MODE_INTERNAL_NTRIP;

bool ntripUserRequestConnect = false;
SemaphoreHandle_t webSocketMutex = nullptr;

unsigned long lastRtcmActivity = 0;
unsigned long lastConsoleSend = 0;
String consoleAccumulator = "";

bool isLogging = false;
LogFormat currentLogFormat = LOG_NONE;
String logFilters = "";

bool currentJammingFiltersOn = true;
uint32_t lastGnssDataMs = 0;
uint8_t currentFixCode = 0;
bool systemCriticalError = false;
bool systemWarning = false;

unsigned long lastStatusUpdateTime = 0;

std::map<uint8_t, SatelliteInfo> skyplotData;

volatile bool gnssSerialReady = false;
volatile bool receiverStreamsReady = false;

volatile bool bleInitStarted = false;
volatile bool bleReady = false;