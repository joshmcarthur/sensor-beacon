#include "Bme280Sensor.h"

#include <Adafruit_BME280.h>

#include "BeaconFields.h"

const __FlashStringHelper* Bme280Sensor::name() const {
  return F("BME280");
}

bool Bme280Sensor::probe(TwoWire& wire) {
  _wire = &wire;
  static const uint8_t kAddrs[] = {0x76, 0x77};

  for (uint8_t addr : kAddrs) {
    Adafruit_BME280 bme;
    if (bme.begin(addr, &wire)) {
      _addr = addr;
      _present = true;
      return true;
    }
  }

  _present = false;
  return false;
}

bool Bme280Sensor::read(Reading& reading) {
  if (!_present || _wire == nullptr) {
    return false;
  }

  Adafruit_BME280 bme;
  if (!bme.begin(_addr, _wire)) {
    return false;
  }

  reading.has_bme280 = true;
  reading.temp_c = bme.readTemperature();
  reading.humidity_pct = bme.readHumidity();
  reading.pressure_hpa = bme.readPressure() / 100.0f;
  reading.ok = true;
  return true;
}

void Bme280Sensor::appendBeaconFields(const Reading& reading, char* buf, size_t buf_len) const {
  if (!reading.has_bme280) {
    return;
  }

  char field[48];
  snprintf(field, sizeof(field), "T=%.1f", reading.temp_c);
  appendBeaconField(buf, buf_len, field);
  snprintf(field, sizeof(field), "H=%.0f", reading.humidity_pct);
  appendBeaconField(buf, buf_len, field);
  snprintf(field, sizeof(field), "P=%.0f", reading.pressure_hpa);
  appendBeaconField(buf, buf_len, field);
}
