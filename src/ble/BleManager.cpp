#include "ble/BleManager.h"
#include "AppGlobals.h"
#include "config/AppConfig.h"
#include "websocket/WebSocketManager.h"
#include "utils/BootProfiler.h"

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* server) override {
        deviceConnected = true;
        Serial.println("BLE: Client Connected");
        sendStatusUpdate();
    }

    void onDisconnect(BLEServer* server) override {
        deviceConnected = false;
        Serial.println("BLE: Client Disconnected");

        delay(500);
        server->getAdvertising()->start();

        sendStatusUpdate();
    }
};

class MyCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *characteristic) override {
        std::string rxValue = characteristic->getValue();

        if (rxValue.length() > 0) {
            for (int i = 0; i < rxValue.length(); i++) {
                Serial2.write(rxValue[i]);
            }

            lastRtcmActivity = millis();
            bleDataCounter += rxValue.length();
        }
    }
};

void initBle() {
    BOOT_LOG("BLE initialization started");
    BLEDevice::init("Dualy-GNSS");

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *service = pServer->createService(SERVICE_UUID);

    pTxCharacteristic = service->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY
    );

    pTxCharacteristic->addDescriptor(new BLE2902());

    BLECharacteristic *rxCharacteristic = service->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
    );

    rxCharacteristic->setCallbacks(new MyCallbacks());

    service->start();

    BLEAdvertising *advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(SERVICE_UUID);
    advertising->setScanResponse(true);
    advertising->setMinPreferred(0x06);
    advertising->setMinPreferred(0x12);

    bleStackInitialized = true;

    if (BLUETOOTH_ENABLED) {
        advertising->start();
        Serial.println("BLE Advertising Started at Boot");
    } else {
        Serial.println("BLE Initialized but hidden (Standby)");
    }
    BOOT_LOG("BLE initialization finished");
}

void updateBluetoothState(bool enable) {
    BLEAdvertising *advertising = BLEDevice::getAdvertising();

    if (!advertising) {
        Serial.println("CRITICAL ERROR: BLE advertising is NULL.");
        return;
    }

    Serial.printf("DEBUG: updateBluetoothState called. Enable=%d, Current=%d\n", enable, BLUETOOTH_ENABLED);

    if (enable) {
        if (!BLUETOOTH_ENABLED) {
            advertising->start();
            BLUETOOTH_ENABLED = true;
            Serial.println(">>> BLE Visibility ENABLED <<<");
        } else {
            advertising->stop();
            delay(10);
            advertising->start();
            Serial.println(">>> BLE already ON, restarted advertising <<<");
        }
    } else {
        if (BLUETOOTH_ENABLED) {
            advertising->stop();
            BLUETOOTH_ENABLED = false;
            Serial.println(">>> BLE Visibility DISABLED <<<");
        } else {
            Serial.println("DEBUG: BLE already off.");
        }
    }

    saveCredentials();
    sendStatusUpdate();
}

void handleBleStatusLog() {
    if (bleDataCounter > 0 && (millis() - lastBleLogTime > 1000)) {
        String logMsg = "[BLE Relay] Received " + String(bleDataCounter) + " bytes RTCM";

        broadcastRtcm(logMsg);

        bleDataCounter = 0;
        lastBleLogTime = millis();
    }
}
