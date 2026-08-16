#include "SensorReader.h"

#include "sensors/BeaconFields.h"

SensorReader::SensorReader() {
  _channels[_channel_count++] = &_bme280;
  _channels[_channel_count++] = &_dht22;
  _channels[_channel_count++] = &_opt3001;
  _channels[_channel_count++] = &_bh1750;
  _channels[_channel_count++] = &_ltr390;
}

bool SensorReader::begin(TwoWire& wire, uint16_t (*readBatteryMv)()) {
  _readBatteryMv = readBatteryMv;

  bool any_present = false;
  Serial.print(F("sensors:"));
  for (size_t i = 0; i < _channel_count; i++) {
    if (_channels[i]->probe(wire)) {
      Serial.print(F(" "));
      Serial.print(_channels[i]->name());
      any_present = true;
    }
  }
  if (!any_present) {
    Serial.print(F(" (none)"));
  }
  Serial.println();

  return _readBatteryMv != nullptr;
}

Reading SensorReader::read() {
  Reading reading;

  for (size_t i = 0; i < _channel_count; i++) {
    if (_channels[i]->present()) {
      _channels[i]->read(reading);
    }
  }

  if (_readBatteryMv) {
    reading.battery_v = _readBatteryMv() / 1000.0f;
    reading.has_battery = true;
    reading.ok = true;
  }

  return reading;
}

int SensorReader::formatBeaconMessage(const Reading& reading, char* buf, size_t buf_len,
                                      uint32_t seq) const {
  if (buf_len == 0) {
    return 0;
  }
  buf[0] = '\0';

  char field[48];
  snprintf(field, sizeof(field), "seq=%lu", (unsigned long)seq);
  appendBeaconField(buf, buf_len, field);

  for (size_t i = 0; i < _channel_count; i++) {
    _channels[i]->appendBeaconFields(reading, buf, buf_len);
  }

  if (reading.has_battery) {
    snprintf(field, sizeof(field), "V=%.2f", reading.battery_v);
    appendBeaconField(buf, buf_len, field);
  }

  return (int)strlen(buf);
}
