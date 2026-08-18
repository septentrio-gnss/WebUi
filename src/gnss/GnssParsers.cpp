#include "gnss/GnssParsers.h"
#include "AppGlobals.h"
#include "websocket/WebSocketManager.h"
#include "utils/BootProfiler.h"

void parseAndBroadcastPVT() {
    static bool firstPvtLogged = false;

    if (!firstPvtLogged) {
        BOOT_LOG("First PVTGeodetic block parsed - position data available");
        firstPvtLogged = true;
    }

    uint8_t modeByte = gnss.SBFBuffer.data[14];
    uint8_t finalFixCode = modeByte & 0x0F;

    currentFixCode = finalFixCode;
    lastGnssDataMs = millis();

    // PVTGeodetic NrSV is one unsigned byte at offset 74.
    uint8_t rawSatsInSolution = gnss.SBFBuffer.data[74];

    // 255 is the SBF "do not use" value.
    uint8_t satsInSolution =
        (rawSatsInSolution == 255) ? 0 : rawSatsInSolution;

    uint8_t pvtError = gnss.SBFBuffer.data[15];

    bool pvtValid =
        (finalFixCode != 0) &&
        (pvtError == 0);

    JsonDocument posDoc;
    posDoc["type"] = "gga_update";
    posDoc["fix"] = finalFixCode;
    posDoc["valid"] = pvtValid;
    posDoc["error_code"] = pvtError;
    posDoc["sats_in_use"] = satsInSolution;

    if (pvtValid) {
        double latitudeRad  = gnss.f8Conv(&gnss.SBFBuffer, 16);
        double longitudeRad = gnss.f8Conv(&gnss.SBFBuffer, 24);
        double altitude     = gnss.f8Conv(&gnss.SBFBuffer, 32);

        double finalLat = latitudeRad * 180.0 / PI;
        double finalLon = longitudeRad * 180.0 / PI;

        if (isfinite(finalLat) && isfinite(finalLon) &&
            finalLat >= -90.0 && finalLat <= 90.0 &&
            finalLon >= -180.0 && finalLon <= 180.0) {

            posDoc["lat"] = finalLat;
            posDoc["lon"] = finalLon;
            posDoc["alt"] = altitude;
        } else {
            posDoc["valid"] = false;

            Serial.printf("[PVT WARNING] Invalid coordinate ignored: lat=%f lon=%f fix=%u\n",
                          finalLat,
                          finalLon,
                          finalFixCode);
        }
    }

    String posOutput;
    serializeJson(posDoc, posOutput);
    broadcastJson(posOutput);

    JsonDocument velDoc;
    velDoc["type"] = "vel_update";
    velDoc["valid"] = pvtValid;

    if (pvtValid) {
        velDoc["vn"] = gnss.f4Conv(&gnss.SBFBuffer, 44);
        velDoc["ve"] = gnss.f4Conv(&gnss.SBFBuffer, 48);
        velDoc["vu"] = gnss.f4Conv(&gnss.SBFBuffer, 52);
    }

    String velOutput;
    serializeJson(velDoc, velOutput);
    broadcastJson(velOutput);

    static unsigned long lastPvtLog = 0;
    if (millis() - lastPvtLog > 1000) {
        Serial.printf(
            "[SBF] PVTGeodetic parsed | fix=%u | error=%u | valid=%s | sats=%u\n",
            finalFixCode,
            pvtError,
            pvtValid ? "YES" : "NO",
            satsInSolution
        );
        lastPvtLog = millis();
    }
}

void parseAndBroadcastVelCov() {
    JsonDocument doc;
    doc["type"] = "velcov_update";

    float covVxVx = gnss.f4Conv(&gnss.SBFBuffer, 16);
    float covVyVy = gnss.f4Conv(&gnss.SBFBuffer, 20);
    float covVzVz = gnss.f4Conv(&gnss.SBFBuffer, 24);

    doc["vn_std"] = sqrt(covVxVx);
    doc["ve_std"] = sqrt(covVyVy);
    doc["vu_std"] = sqrt(covVzVz);

    String output;
    serializeJson(doc, output);
    broadcastJson(output);
}

void parseAndBroadcastAttitude() {
    JsonDocument doc;
    doc["type"] = "att_update";
    doc["heading"] = gnss.f4Conv(&gnss.SBFBuffer, 20);
    doc["pitch"]   = gnss.f4Conv(&gnss.SBFBuffer, 24);
    doc["roll"]    = gnss.f4Conv(&gnss.SBFBuffer, 28);

    String output;
    serializeJson(doc, output);
    broadcastJson(output);
}

void parseAndBroadcastPosCov() {
    JsonDocument doc;
    doc["type"] = "poscov_update";

    float covLatLat = gnss.f4Conv(&gnss.SBFBuffer, 16);
    float covLonLon = gnss.f4Conv(&gnss.SBFBuffer, 20);
    float covHgtHgt = gnss.f4Conv(&gnss.SBFBuffer, 24);

    doc["lat_std"] = sqrt(covLatLat);
    doc["lon_std"] = sqrt(covLonLon);
    doc["alt_std"] = sqrt(covHgtHgt);

    String output;
    serializeJson(doc, output);
    broadcastJson(output);
}

void parseAndBroadcastAttCov() {
    JsonDocument doc;
    doc["type"] = "attcov_update";

    float covHeadHead   = gnss.f4Conv(&gnss.SBFBuffer, 16);
    float covPitchPitch = gnss.f4Conv(&gnss.SBFBuffer, 20);
    float covRollRoll   = gnss.f4Conv(&gnss.SBFBuffer, 24);

    doc["hdg_std"]   = sqrt(covHeadHead);
    doc["pitch_std"] = sqrt(covPitchPitch);
    doc["roll_std"]  = sqrt(covRollRoll);

    String output;
    serializeJson(doc, output);
    broadcastJson(output);
}

void parseAndBroadcastQuality() {
    static bool firstQualityLogged = false;

    if (!firstQualityLogged) {
        BOOT_LOG("First QualityInd block parsed - quality indicators available");
        firstQualityLogged = true;
    }
    uint8_t numIndicators = (uint8_t)gnss.u2Conv(&gnss.SBFBuffer, 14);

    JsonDocument doc;
    doc["type"] = "quality_update";

    int overall = -1;
    int rfPower = -1;
    int auxrf = -1;
    int signals = -1;
    int auxsig = -1;
    int cpu = -1;
    int base = -1;

    for (int i = 0; i < numIndicators; i++) {
        uint16_t indicator = gnss.u2Conv(&gnss.SBFBuffer, 16 + (i * 2));

        uint8_t indicatorType = (uint8_t)(indicator & 0x00FF);
        uint8_t indicatorValue = (uint8_t)((indicator >> 8) & 0x0F);

        int valueToSend = (indicatorValue == 15) ? -1 : indicatorValue;

        switch (indicatorType) {
            case 0:  overall = valueToSend; break;
            case 1:  signals = valueToSend; break;
            case 2:  auxsig = valueToSend; break;
            case 11: rfPower = valueToSend; break;
            case 12: auxrf = valueToSend; break;
            case 21: cpu = valueToSend; break;
            case 30: base = valueToSend; break;
            default: break;
        }
    }

    doc["overall"] = overall;
    doc["rf_power"] = rfPower;
    doc["auxrf"] = auxrf;
    doc["signals"] = signals;
    doc["auxsig"] = auxsig;
    doc["cpu"] = cpu;
    doc["base"] = base;

    String output;
    serializeJson(doc, output);
    broadcastJson(output);

    static unsigned long lastQualityLog = 0;
    if (millis() - lastQualityLog > 2000) {
        Serial.printf("[SBF] QualityInd parsed | overall=%d mainRF=%d auxRF=%d mainSig=%d auxSig=%d cpu=%d base=%d\n",
                      overall,
                      rfPower,
                      auxrf,
                      signals,
                      auxsig,
                      cpu,
                      base);

        lastQualityLog = millis();
    }
}

void parseAndBroadcastRFStatus() {
    uint8_t flags = (uint8_t)gnss.u2Conv(&gnss.SBFBuffer, 16);
    bool spoofingDetected = (flags & 0x01) || (flags & 0x02);

    uint8_t numBands = (uint8_t)gnss.u2Conv(&gnss.SBFBuffer, 14);
    uint8_t sbLength = (uint8_t)gnss.u2Conv(&gnss.SBFBuffer, 15);

    bool jammingDetected = false;

    for (int i = 0; i < numBands; i++) {
        uint16_t bandOffset = 20 + (i * sbLength);
        uint8_t info = (uint8_t)gnss.u2Conv(&gnss.SBFBuffer, bandOffset + 2);
        uint8_t mode = info & 0x0F;

        if (mode == 2 || mode == 8) {
            jammingDetected = true;
            break;
        }
    }

    bool threatDetected = spoofingDetected || jammingDetected;
    bool filtersAreOk = !threatDetected;

    if (filtersAreOk != currentJammingFiltersOn) {
        currentJammingFiltersOn = filtersAreOk;
        sendJammingStatus();
    }

    static unsigned long lastRfStatusLog = 0;
    if (millis() - lastRfStatusLog > 1000) {
        Serial.println("[SBF] RFStatus parsed");
        lastRfStatusLog = millis();
    }
}

void parseAndBroadcastReceiverStatus() {
    if (gnss.SBFBuffer.msgSize < 28) return;

    uint8_t extError = gnss.SBFBuffer.data[15];
    uint32_t rxError = gnss.u4Conv(&gnss.SBFBuffer, 24);

    bool criticalError = (extError != 0);
    bool receiverWarning = (rxError != 0) && !criticalError;

    systemCriticalError = criticalError;
    systemWarning = receiverWarning;

    JsonDocument doc;
    doc["type"] = "system_status";
    doc["error"] = criticalError;
    doc["warning"] = receiverWarning;
    doc["ext_error"] = extError;
    doc["rx_error"] = rxError;

    String output;
    serializeJson(doc, output);
    broadcastJson(output);

    if (criticalError || receiverWarning) {
        static unsigned long lastReceiverStatusLog = 0;

        if (millis() - lastReceiverStatusLog > 5000) {
            Serial.printf("[RECEIVER STATUS] Ext: %02X, Rx: %08X, level=%s\n",
                          extError,
                          rxError,
                          criticalError ? "CRITICAL" : "WARNING");

            lastReceiverStatusLog = millis();
        }
    }
}

void parseAndBroadcastSatVisibility() {
    uint8_t numSats = gnss.SBFBuffer.data[14];
    uint8_t sbLength = gnss.SBFBuffer.data[15];

    if (numSats == 0 || sbLength == 0) return;

    uint16_t baseOffset = 16;

    for (int i = 0; i < numSats; i++) {
        uint16_t satOffset = baseOffset + (i * sbLength);

        if (sbLength < 8 || (satOffset + 8 > gnss.SBFBuffer.msgSize)) break;

        uint8_t svid = gnss.SBFBuffer.data[satOffset + 0];
        float azimuth = gnss.u2Conv(&gnss.SBFBuffer, satOffset + 2) * 0.01;
        float elevation = gnss.i2Conv(&gnss.SBFBuffer, satOffset + 4) * 0.01;

        skyplotData[svid].svid = svid;
        skyplotData[svid].azimuth = azimuth;
        skyplotData[svid].elevation = elevation;
        skyplotData[svid].data_updated = true;
    }
}

void parseAndBroadcastMeasEpoch() {
    uint8_t N1 = gnss.SBFBuffer.data[14];
    uint8_t SB1Length = gnss.SBFBuffer.data[15];

    if (N1 == 0 || SB1Length == 0) return;

    uint16_t baseOffset = 20;

    for (int i = 0; i < N1; i++) {
        uint16_t offset1 = baseOffset + (i * SB1Length);

        if (offset1 + 12 > gnss.SBFBuffer.msgSize) break;

        uint8_t svid = gnss.SBFBuffer.data[offset1 + 2];
        uint8_t cn0Raw = gnss.SBFBuffer.data[offset1 + 11];

        float cn0Val = (float)cn0Raw * 0.25f + 10.0f;

        skyplotData[svid].svid = svid;
        skyplotData[svid].cn0 = cn0Val;
        skyplotData[svid].data_updated = true;
    }
}

void parseAndBroadcastChannelStatus() {
    uint8_t N = gnss.SBFBuffer.data[14];
    uint8_t SB1Length = gnss.SBFBuffer.data[15];
    uint8_t SB2Length = gnss.SBFBuffer.data[16];

    if (N == 0 || SB1Length == 0 || SB2Length == 0) return;

    uint16_t baseOffset = 20;

    for (int i = 0; i < N; i++) {
        uint16_t offset1 = baseOffset + (i * SB1Length);

        if (offset1 + 8 > gnss.SBFBuffer.msgSize) break;

        uint8_t svid = gnss.SBFBuffer.data[offset1 + 0];
        uint8_t N2 = gnss.SBFBuffer.data[offset1 + 7];

        bool satIsUsed = false;

        for (int j = 0; j < N2; j++) {
            uint16_t offset2 = offset1 + SB1Length + (j * SB2Length);

            if (offset2 + 4 > gnss.SBFBuffer.msgSize) break;

            uint16_t pvtStatusRaw = gnss.u2Conv(&gnss.SBFBuffer, offset2 + 2);

            for (int k = 0; k < 8; k++) {
                if (((pvtStatusRaw >> (k * 2)) & 0x03) == 2) {
                    satIsUsed = true;
                    break;
                }
            }

            if (satIsUsed) break;
        }

        skyplotData[svid].svid = svid;
        skyplotData[svid].in_pvt = satIsUsed;
        skyplotData[svid].data_updated = true;
    }
}

void broadcastFullSkyplot() {
    JsonDocument doc;
    doc["type"] = "gsv_update";

    JsonArray sats = doc["sats"].to<JsonArray>();

    for (auto it = skyplotData.begin(); it != skyplotData.end(); ++it) {
        SatelliteInfo& satInfo = it->second;

        if (satInfo.data_updated && satInfo.elevation > 0) {
            JsonObject sat = sats.add<JsonObject>();

            sat["prn"] = satInfo.svid;
            sat["elev"] = satInfo.elevation;
            sat["azim"] = satInfo.azimuth;
            sat["cn0"] = satInfo.cn0;
            sat["in_pvt"] = satInfo.in_pvt;

            satInfo.data_updated = false;
            satInfo.cn0 = 0;
            satInfo.in_pvt = false;
        }
    }

    if (sats.size() > 0) {
        String output;
        serializeJson(doc, output);
        broadcastJson(output);
    }
}

void parseAndBroadcastDOP() {
    static bool firstDopLogged = false;

    if (!firstDopLogged) {
        BOOT_LOG("First DOP block parsed - precision indicators available");
        firstDopLogged = true;
    }
    JsonDocument doc;
    doc["type"] = "dop_update";
    doc["pdop"] = gnss.u2Conv(&gnss.SBFBuffer, 16) * 0.01;
    doc["tdop"] = gnss.u2Conv(&gnss.SBFBuffer, 18) * 0.01;
    doc["hdop"] = gnss.u2Conv(&gnss.SBFBuffer, 20) * 0.01;
    doc["vdop"] = gnss.u2Conv(&gnss.SBFBuffer, 22) * 0.01;

    String output;
    serializeJson(doc, output);
    broadcastJson(output);
}

void parseAndBroadcastTime() {
    JsonDocument doc;
    doc["type"] = "time_update";
    doc["year"]  = (int8_t)gnss.SBFBuffer.data[14];
    doc["month"] = (int8_t)gnss.SBFBuffer.data[15];
    doc["day"]   = (int8_t)gnss.SBFBuffer.data[16];
    doc["hour"]  = (int8_t)gnss.SBFBuffer.data[17];
    doc["min"]   = (int8_t)gnss.SBFBuffer.data[18];
    doc["sec"]   = (int8_t)gnss.SBFBuffer.data[19];
    doc["dls"]   = (int8_t)gnss.SBFBuffer.data[20];

    String output;
    serializeJson(doc, output);
    broadcastJson(output);
}
