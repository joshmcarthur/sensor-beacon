#include "Bh1750Sensor.h"

#include "BeaconFields.h"

namespace {

constexpr uint8_t kCmdPowerOn = 0x01;
constexpr uint8_t kCmdReset = 0x07;
constexpr uint8_t kCmdOneTimeHighRes = 0x20;
constexpr uint32_t kMeasureDelayMs = 180;

bool bh1750Write(TwoWire& wire, uint8_t addr, uint8_t cmd) {
  wire.beginTransmission(addr);
  wire.write(cmd);
  return wire.endTransmission() == 0;
}

bool bh1750ReadRaw(TwoWire& wire, uint8_t addr, uint16_t& raw) {
  if (wire.requestFrom(addr, (uint8_t)2) != 2) {
    return false;
  }

  raw = ((uint16_t)wire.read() << 8) | wire.read();
  return true;
}

}  // namespace

const __FlashStringHelper* Bh1750Sensor::name() const {
  return F("BH1750");
}

bool Bh1750Sensor::measureLux(float& lux) const {
  if (_wire == nullptr) {
    return false;
  }

  if (!bh1750Write(*_wire, _addr, kCmdPowerOn)) {
    return false;
  }
  if (!bh1750Write(*_wire, _addr, kCmdReset)) {
    return false;
  }
  if (!bh1750Write(*_wire, _addr, kCmdOneTimeHighRes)) {
    return false;
  }

  delay(kMeasureDelayMs);

  uint16_t raw = 0;
  if (!bh1750ReadRaw(*_wire, _addr, raw)) {
    return false;
  }

  lux = raw / 1.2f;
  return true;
}

bool Bh1750Sensor::probe(TwoWire& wire) {
  _wire = &wire;
  static const uint8_t kAddrs[] = {0x23, 0x5C};

  for (uint8_t addr : kAddrs) {
    if (!i2cDevicePresent(wire, addr)) {
      continue;
    }

    _addr = addr;
    float lux = 0.0f;
    if (!measureLux(lux)) {
      continue;
    }

    _present = true;
    return true;
  }

  _present = false;
  return false;
}

bool Bh1750Sensor::read(Reading& reading) {
  if (!_present) {
    return false;
  }

  float lux = 0.0f;
  if (!measureLux(lux)) {
    return false;
  }

  reading.has_bh1750 = true;
  reading.lux = lux;
  reading.ok = true;
  return true;
}

void Bh1750Sensor::appendBeaconFields(const Reading& reading, char* buf, size_t buf_len) const {
  if (!reading.has_bh1750) {
    return;
  }

  char field[48];
  snprintf(field, sizeof(field), "L=%.0f", reading.lux);
  appendBeaconField(buf, buf_len, field);
}
