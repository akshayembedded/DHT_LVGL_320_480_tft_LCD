#include <Arduino.h>
#include <Wire.h>
#include "ip5306.h"


void checkBatteryLevel() {
  Wire.beginTransmission(IP5306_ADDR);
  Wire.write(0x78); // Read battery level register
  Wire.endTransmission(false);
  Wire.requestFrom(IP5306_ADDR, 1);

  if (Wire.available()) {
    uint8_t val = Wire.read();
    int percentage = 0;

    // IP5306 returns steps based on LED indicators
    switch (val & 0xF0) {
      case 0xE0: percentage = 100; break;
      case 0xC0: percentage = 75; break;
      case 0x80: percentage = 50; break;
      case 0x00: percentage = 25; break;
      default:   percentage = 0;  break;
    }

    Serial.print("Battery Level (approx): ");
    Serial.print(percentage);
    Serial.println(" %");
  }
}

void checkChargingStatus() {
  Wire.beginTransmission(IP5306_ADDR);
  Wire.write(0x70); // Register to check charging status
  Wire.endTransmission(false);
  Wire.requestFrom(IP5306_ADDR, 1);

  if (Wire.available()) {
    uint8_t val = Wire.read();
    bool charging = val & 0x08;
    Serial.print("Charging: ");
    Serial.println(charging ? "Yes" : "No");
  }
}

