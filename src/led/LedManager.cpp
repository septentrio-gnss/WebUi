#include "led/LedManager.h"
#include "AppGlobals.h"

#define REG_RESET_CONTROL 0x00
#define REG_CHANNEL_CTRL  0x04
#define REG_TRISE_FALL    0x05
#define REG_D1_CURRENT    0x06
#define REG_D2_CURRENT    0x07
#define REG_D3_CURRENT    0x08

#define D1_ON 0x01
#define D2_ON 0x04
#define D3_ON 0x10

enum LedPattern {
    LED_OFF_PATTERN,
    LED_SOLID,
    LED_SLOW_BLINK,
    LED_FAST_BLINK
};

struct RgbLedState {
    uint8_t addr;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    LedPattern pattern;
    uint32_t intervalMs;
    bool visible;
    uint32_t lastToggleMs;
};

static RgbLedState systemLed = { LED_SYSTEM_ADDR, 0, 0, 0, LED_OFF_PATTERN, 1000, false, 0 };
static RgbLedState gnssLed   = { LED_GNSS_ADDR,   0, 0, 0, LED_OFF_PATTERN, 1000, false, 0 };

static void ktdWriteReg(uint8_t addr, uint8_t reg, uint8_t value) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(value);
    uint8_t err = Wire.endTransmission();

    if (err != 0) {
        Serial.printf("[LED] I2C write failed addr=0x%02X reg=0x%02X err=%d\n", addr, reg, err);
    }
}

static bool ktdPing(uint8_t addr) {
    Wire.beginTransmission(addr);
    return Wire.endTransmission() == 0;
}

static void ktdInit(uint8_t addr) {
    Serial.printf("[LED] Init KTD2026 at 0x%02X\n", addr);

    ktdWriteReg(addr, REG_RESET_CONTROL, 0x00);
    delay(10);

    ktdWriteReg(addr, REG_TRISE_FALL, 0x00);
    ktdWriteReg(addr, REG_CHANNEL_CTRL, 0x00);

    ktdWriteReg(addr, REG_D1_CURRENT, 0x00);
    ktdWriteReg(addr, REG_D2_CURRENT, 0x00);
    ktdWriteReg(addr, REG_D3_CURRENT, 0x00);
}

static void ktdSetRaw(uint8_t addr, uint8_t d1, uint8_t d2, uint8_t d3) {
    ktdWriteReg(addr, REG_D1_CURRENT, d1);
    ktdWriteReg(addr, REG_D2_CURRENT, d2);
    ktdWriteReg(addr, REG_D3_CURRENT, d3);

    uint8_t ctrl = 0x00;
    if (d1 > 0) ctrl |= D1_ON;
    if (d2 > 0) ctrl |= D2_ON;
    if (d3 > 0) ctrl |= D3_ON;

    ktdWriteReg(addr, REG_CHANNEL_CTRL, ctrl);
}

static void ktdOff(uint8_t addr) {
    ktdWriteReg(addr, REG_CHANNEL_CTRL, 0x00);
    ktdWriteReg(addr, REG_D1_CURRENT, 0x00);
    ktdWriteReg(addr, REG_D2_CURRENT, 0x00);
    ktdWriteReg(addr, REG_D3_CURRENT, 0x00);
}

static void setSystemLedRawColor(uint8_t r, uint8_t g, uint8_t b) {
    // U8 mapping: D3=Red, D2=Green, D1=Blue
    ktdSetRaw(LED_SYSTEM_ADDR, b, g, r);
}

static void setGnssLedRawColor(uint8_t r, uint8_t g, uint8_t b) {
    // U9 currently uses the same logical mapping.
    // If colors are inverted later, fix the mapping here only.
    ktdSetRaw(LED_GNSS_ADDR, b, g, r);
}

static void setLedState(RgbLedState &led, uint8_t r, uint8_t g, uint8_t b, LedPattern pattern, uint32_t intervalMs) {
    led.r = r;
    led.g = g;
    led.b = b;
    led.pattern = pattern;
    led.intervalMs = intervalMs;
}

static void applyLedOutput(RgbLedState &led, bool on) {
    if (!on || led.pattern == LED_OFF_PATTERN) {
        ktdOff(led.addr);
        return;
    }

    if (led.addr == LED_SYSTEM_ADDR) {
        setSystemLedRawColor(led.r, led.g, led.b);
    } else {
        setGnssLedRawColor(led.r, led.g, led.b);
    }
}

static void updateLedPattern(RgbLedState &led) {
    if (led.pattern == LED_SOLID) {
        applyLedOutput(led, true);
        return;
    }

    if (led.pattern == LED_OFF_PATTERN) {
        applyLedOutput(led, false);
        return;
    }

    uint32_t now = millis();
    if (now - led.lastToggleMs >= led.intervalMs) {
        led.visible = !led.visible;
        led.lastToggleMs = now;
        applyLedOutput(led, led.visible);
    }
}

void initLedDrivers() {
    Wire.begin(LED_I2C_SDA, LED_I2C_SCL);
    Wire.setClock(100000);

    Serial.println("[LED] Initializing RGB LED drivers...");

    if (ktdPing(LED_SYSTEM_ADDR)) {
        Serial.println("[LED] System LED driver found at 0x30");
        ktdInit(LED_SYSTEM_ADDR);
    } else {
        Serial.println("[LED] WARNING: System LED driver not found at 0x30");
    }

    if (ktdPing(LED_GNSS_ADDR)) {
        Serial.println("[LED] GNSS LED driver found at 0x31");
        ktdInit(LED_GNSS_ADDR);
    } else {
        Serial.println("[LED] WARNING: GNSS LED driver not found at 0x31");
    }
}

void setBootLedPattern() {
    setLedState(systemLed, 0, 0, 255, LED_SLOW_BLINK, 500);
    setLedState(gnssLed, 0, 0, 0, LED_OFF_PATTERN, 1000);
}

void updateApplicationLedStatus() {
    // LED 1: System / ESP32 / WebUI status
    if (systemCriticalError) {
        setLedState(systemLed, 255, 0, 0, LED_FAST_BLINK, 200);
    } else if (systemWarning) {
        setLedState(systemLed, 255, 80, 0, LED_SLOW_BLINK, 1000);
    } else {
        setLedState(systemLed, 0, 255, 0, LED_SOLID, 1000);
    }

    // LED 2: GNSS / receiver / RF status
    bool receiverAlive = (millis() - lastGnssDataMs < 3000);
    bool rtcmActive = (millis() - lastRtcmActivity < 3000);

    if (!currentJammingFiltersOn) {
        setLedState(gnssLed, 255, 0, 0, LED_FAST_BLINK, 200);
    } else if (!receiverAlive) {
        setLedState(gnssLed, 255, 0, 0, LED_SOLID, 1000);
    } else if (currentFixCode == 0) {
        setLedState(gnssLed, 255, 180, 0, LED_SLOW_BLINK, 700);
    } else if (rtcmActive || ntrip.isConnected()) {
        setLedState(gnssLed, 0, 180, 255, LED_SLOW_BLINK, 700);
    } else {
        setLedState(gnssLed, 0, 255, 0, LED_SOLID, 1000);
    }
}

void updateLedOutputs() {
    updateLedPattern(systemLed);
    updateLedPattern(gnssLed);
}
