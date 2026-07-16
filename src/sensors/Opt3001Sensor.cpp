#include "Opt3001Sensor.h"

#include <ClosedCube_OPT3001.h>

#include "BeaconFields.h"

const __FlashStringHelper* Opt3001Sensor::name() const {
  return F("OPT3001");
}

bool Opt3001Sensor::probe(TwoWire& wire) {
  static const uint8_t kAddrs[] = {0x44, 0x45, 0x46, 0x47};
  constexpr uint16_t kManufacturerId = 0x5449;  // "TI"
  constexpr uint16_t kDeviceId = 0x3001;

  for (uint8_t addr : kAddrs) {
    if (!i2cDevicePresent(wire, addr)) {
      continue;
    }

    ClosedCube_OPT3001 opt;
    opt.begin(addr);
    if (opt.readManufacturerID() != kManufacturerId) {
      continue;
    }
    if (opt.readDeviceID() != kDeviceId) {
      continue;
    }

    _addr = addr;
    _present = true;
    return true;
  }

  _present = false;
  return false;
}

bool Opt3001Sensor::read(Reading& reading) {
  if (!_present) {
    return false;
  }

  ClosedCube_OPT3001 opt;
  opt.begin(_addr);
  OPT3001 sample = opt.readResult();
  if (sample.error != NO_ERROR) {
    return false;
  }

  reading.has_lux = true;
  reading.lux = sample.lux;
  reading.ok = true;
  return true;
}

void Opt3001Sensor::appendBeaconFields(const Reading& reading, char* buf, size_t buf_len) const {
  if (!reading.has_lux) {
    return;
  }

  char field[48];
  snprintf(field, sizeof(field), "L=%.0f", reading.lux);
  appendBeaconField(buf, buf_len, field);
}
