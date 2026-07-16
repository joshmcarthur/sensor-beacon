#include "Ltr390Sensor.h"

#include <Adafruit_LTR390.h>

#include "BeaconFields.h"

const __FlashStringHelper* Ltr390Sensor::name() const {
  return F("LTR390");
}

float Ltr390Sensor::uvsToUvIndex(uint32_t uvs) {
  return uvs / 2300.0f;
}

bool Ltr390Sensor::probe(TwoWire& wire) {
  _wire = &wire;
  static const uint8_t kAddr = 0x53;

  if (!i2cDevicePresent(wire, kAddr)) {
    _present = false;
    return false;
  }

  Adafruit_LTR390 ltr;
  if (!ltr.begin(&wire)) {
    _present = false;
    return false;
  }

  _addr = kAddr;
  _present = true;
  return true;
}

bool Ltr390Sensor::read(Reading& reading) {
  if (!_present || _wire == nullptr) {
    return false;
  }

  Adafruit_LTR390 ltr;
  if (!ltr.begin(_wire)) {
    return false;
  }

  ltr.setMode(LTR390_MODE_UVS);
  delay(100);
  uint32_t uvs = ltr.readUVS();
  ltr.enable(false);

  reading.has_uv = true;
  reading.uv_index = uvsToUvIndex(uvs);
  reading.ok = true;
  return true;
}

void Ltr390Sensor::appendBeaconFields(const Reading& reading, char* buf, size_t buf_len) const {
  if (!reading.has_uv) {
    return;
  }

  char field[48];
  snprintf(field, sizeof(field), "UV=%.1f", reading.uv_index);
  appendBeaconField(buf, buf_len, field);
}
