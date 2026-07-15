#include "SensorReader.h"

#include <Adafruit_BME280.h>
#include <Adafruit_OPT3001.h>
#include <Adafruit_LTR390.h>

namespace {

bool i2cDevicePresent(TwoWire& wire, uint8_t addr) {
  wire.beginTransmission(addr);
  return wire.endTransmission() == 0;
}

bool probeBme280(TwoWire& wire, uint8_t& addr_out) {
  static const uint8_t kAddrs[] = {0x76, 0x77};
  for (uint8_t addr : kAddrs) {
    Adafruit_BME280 bme;
    if (bme.begin(addr, &wire)) {
      addr_out = addr;
      return true;
    }
  }
  return false;
}

bool probeOpt3001(TwoWire& wire, uint8_t& addr_out) {
  static const uint8_t kAddrs[] = {0x44, 0x45, 0x46, 0x47};
  for (uint8_t addr : kAddrs) {
    Adafruit_OPT3001 opt;
    if (opt.begin(addr, &wire)) {
      addr_out = addr;
      return true;
    }
  }
  return false;
}

bool probeLtr390(TwoWire& wire, uint8_t& addr_out) {
  static const uint8_t kAddr = 0x53;
  if (!i2cDevicePresent(wire, kAddr)) {
    return false;
  }
  Adafruit_LTR390 ltr;
  if (ltr.begin(&wire)) {
    addr_out = kAddr;
    return true;
  }
  return false;
}

float uvsToUvIndex(uint32_t uvs) {
  return uvs / 2300.0f;
}

void appendField(char* buf, size_t buf_len, const char* field) {
  size_t used = strlen(buf);
  if (used == 0) {
    strlcpy(buf, field, buf_len);
    return;
  }
  if (used + 1 < buf_len) {
    strlcat(buf, " ", buf_len);
    strlcat(buf, field, buf_len);
  }
}

}  // namespace

bool SensorReader::begin(TwoWire& wire, uint16_t (*readBatteryMv)()) {
  _wire = &wire;
  _readBatteryMv = readBatteryMv;

  _has_bme280 = probeBme280(wire, _bme_addr);
  _has_lux = probeOpt3001(wire, _opt_addr);
  _has_uv = probeLtr390(wire, _ltr_addr);

  Serial.print(F("sensors:"));
  if (_has_bme280) Serial.print(F(" BME280"));
  if (_has_lux) Serial.print(F(" OPT3001"));
  if (_has_uv) Serial.print(F(" LTR390"));
  if (!_has_bme280 && !_has_lux && !_has_uv) Serial.print(F(" (none)"));
  Serial.println();

  return _readBatteryMv != nullptr;
}

Reading SensorReader::read() {
  Reading r;
  r.has_bme280 = _has_bme280;
  r.has_lux = _has_lux;
  r.has_uv = _has_uv;

  if (_has_bme280) {
    Adafruit_BME280 bme;
    if (bme.begin(_bme_addr, _wire)) {
      r.temp_c = bme.readTemperature();
      r.humidity_pct = bme.readHumidity();
      r.pressure_hpa = bme.readPressure() / 100.0f;
      r.ok = true;
    }
  }

  if (_has_lux) {
    Adafruit_OPT3001 opt;
    if (opt.begin(_opt_addr, _wire)) {
      r.lux = opt.readLux();
      opt.shutdown();
      r.ok = true;
    }
  }

  if (_has_uv) {
    Adafruit_LTR390 ltr;
    if (ltr.begin(_wire)) {
      ltr.setMode(LTR390_MODE_UVS);
      delay(100);
      uint32_t uvs = 0;
      if (ltr.newDataAvailable()) {
        uvs = ltr.readUVS();
      } else {
        uvs = ltr.readUVS();
      }
      r.uv_index = uvsToUvIndex(uvs);
      ltr.enable(false);
      r.ok = true;
    }
  }

  if (_readBatteryMv) {
    r.battery_v = _readBatteryMv() / 1000.0f;
    r.ok = true;
  }

  return r;
}

int formatBeaconMessage(const Reading& reading, char* buf, size_t buf_len) {
  if (buf_len == 0) {
    return 0;
  }
  buf[0] = '\0';

  char field[48];

  if (reading.has_bme280) {
    snprintf(field, sizeof(field), "T=%.1f", reading.temp_c);
    appendField(buf, buf_len, field);
    snprintf(field, sizeof(field), "H=%.0f", reading.humidity_pct);
    appendField(buf, buf_len, field);
    snprintf(field, sizeof(field), "P=%.0f", reading.pressure_hpa);
    appendField(buf, buf_len, field);
  }

  if (reading.has_lux) {
    snprintf(field, sizeof(field), "L=%.0f", reading.lux);
    appendField(buf, buf_len, field);
  }

  if (reading.has_uv) {
    snprintf(field, sizeof(field), "UV=%.1f", reading.uv_index);
    appendField(buf, buf_len, field);
  }

  snprintf(field, sizeof(field), "V=%.2f", reading.battery_v);
  appendField(buf, buf_len, field);

  return (int)strlen(buf);
}
