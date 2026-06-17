#include <Arduino.h>
#include <Wire.h>

#define I2C_SDA 9
#define I2C_SCL 10

#define LED_DRIVER_1 0x30  // U8 - KTD2026EWE-TR
#define LED_DRIVER_2 0x31  // U9 - KTD2026BEWE-TR

// KTD2026 registers
#define REG_RESET_CONTROL 0x00
#define REG_CHANNEL_CTRL  0x04
#define REG_TRISE_FALL    0x05
#define REG_D1_CURRENT    0x06
#define REG_D2_CURRENT    0x07
#define REG_D3_CURRENT    0x08

// Channel control bits:
// D1 uses bits 1:0
// D2 uses bits 3:2
// D3 uses bits 5:4
// 00 = off, 01 = always on
#define D1_ON 0x01
#define D2_ON 0x04
#define D3_ON 0x10
#define ALL_ON (D1_ON | D2_ON | D3_ON)

void writeReg(uint8_t addr, uint8_t reg, uint8_t value) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(value);
    uint8_t err = Wire.endTransmission();

    Serial.print("Write 0x");
    Serial.print(addr, HEX);
    Serial.print(" reg 0x");
    Serial.print(reg, HEX);
    Serial.print(" = 0x");
    Serial.print(value, HEX);
    Serial.print(" -> err ");
    Serial.println(err);
}

bool ping(uint8_t addr) {
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();

    Serial.print("Ping 0x");
    Serial.print(addr, HEX);
    Serial.print(" -> ");
    Serial.println(err == 0 ? "ACK" : "NO ACK");

    return err == 0;
}

void initDriver(uint8_t addr) {
    Serial.print("Initializing driver 0x");
    Serial.println(addr, HEX);

    // Wake / normal operation
    writeReg(addr, REG_RESET_CONTROL, 0x00);
    delay(10);

    // No fade/ramp for now
    writeReg(addr, REG_TRISE_FALL, 0x00);
    delay(10);

    // Start with all outputs off
    writeReg(addr, REG_CHANNEL_CTRL, 0x00);

    // Set a safe low current first
    // Datasheet/Linux driver uses register values for output current.
    // 0x10 is a conservative visible starting value.
    writeReg(addr, REG_D1_CURRENT, 0x10);
    writeReg(addr, REG_D2_CURRENT, 0x10);
    writeReg(addr, REG_D3_CURRENT, 0x10);
}

void allOff(uint8_t addr) {
    writeReg(addr, REG_CHANNEL_CTRL, 0x00);
}

void setChannels(uint8_t addr, bool d1, bool d2, bool d3, uint8_t current = 0x20) {
    writeReg(addr, REG_D1_CURRENT, current);
    writeReg(addr, REG_D2_CURRENT, current);
    writeReg(addr, REG_D3_CURRENT, current);

    uint8_t ctrl = 0x00;
    if (d1) ctrl |= D1_ON;
    if (d2) ctrl |= D2_ON;
    if (d3) ctrl |= D3_ON;

    writeReg(addr, REG_CHANNEL_CTRL, ctrl);
}

void testDriver(uint8_t addr) {
    if (!ping(addr)) {
        return;
    }

    initDriver(addr);

    Serial.print("Testing D1 on driver 0x");
    Serial.println(addr, HEX);
    setChannels(addr, true, false, false, 0x20);
    delay(1500);

    Serial.print("Testing D2 on driver 0x");
    Serial.println(addr, HEX);
    setChannels(addr, false, true, false, 0x20);
    delay(1500);

    Serial.print("Testing D3 on driver 0x");
    Serial.println(addr, HEX);
    setChannels(addr, false, false, true, 0x20);
    delay(1500);

    Serial.print("Testing all channels on driver 0x");
    Serial.println(addr, HEX);
    setChannels(addr, true, true, true, 0x15);
    delay(1500);

    Serial.print("OFF driver 0x");
    Serial.println(addr, HEX);
    allOff(addr);
    delay(1000);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("KTD2026 RGB LED ON/OFF test");
    Serial.println("SDA = GPIO9, SCL = GPIO10");

    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(100000);

    ping(LED_DRIVER_1);
    ping(LED_DRIVER_2);

    initDriver(LED_DRIVER_1);
    initDriver(LED_DRIVER_2);

    allOff(LED_DRIVER_1);
    allOff(LED_DRIVER_2);
}

void loop() {
    Serial.println("Testing LED driver U8 / address 0x30");
    testDriver(LED_DRIVER_1);

    Serial.println("Testing LED driver U9 / address 0x31");
    testDriver(LED_DRIVER_2);

    delay(2000);
}