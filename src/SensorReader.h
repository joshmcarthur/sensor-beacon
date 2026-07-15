#pragma once

#include <Arduino.h>
#include <Wire.h>

struct Reading {
  float temp_c = 0.0f;
  float humidity_pct = 0.0f;
  float pressure_hpa = 0.0f;
  float lux = 0.0f;
  float uv_index = 0.0f;
  float battery_v = 0.0f;
  bool has_bme280 = false;
  bool has_lux = false;
  bool has_uv = false;
  bool ok = false;
};

class SensorReader {
public:
  bool begin(TwoWire& wire, uint16_t (*readBatteryMv)());
  Reading read();

  bool hasBme280() const { return _has_bme280; }
  bool hasLux() const { return _has_lux; }
  bool hasUv() const { return _has_uv; }

private:
  TwoWire* _wire = nullptr;
  uint16_t (*_readBatteryMv)() = nullptr;
  bool _has_bme280 = false;
  bool _has_lux = false;
  bool _has_uv = false;
  uint8_t _bme_addr = 0;
  uint8_t _opt_addr = 0;
  uint8_t _ltr_addr = 0;
};

int formatBeaconMessage(const Reading& reading, char* buf, size_t buf_len);
