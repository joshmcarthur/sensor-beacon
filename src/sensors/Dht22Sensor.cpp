#include "Dht22Sensor.h"

#include "BeaconFields.h"

#ifdef DHT22_PIN

#include <DHT.h>

namespace {

// Adafruit DHT library type constant — DHT22/AM2302 only (not DHT11; different protocol timing).
static constexpr uint8_t kDhtSensorType = DHT22;
DHT dht(DHT22_PIN, kDhtSensorType);

bool isValidSample(float temp_c, float humidity_pct) {
  if (isnan(temp_c) || isnan(humidity_pct)) {
    return false;
  }
  if (temp_c < -40.0f || temp_c > 85.0f) {
    return false;
  }
  if (humidity_pct < 0.0f || humidity_pct > 100.0f) {
    return false;
  }
  return true;
}

bool readSample(float& temp_c, float& humidity_pct) {
  humidity_pct = dht.readHumidity();
  temp_c = dht.readTemperature();
  return isValidSample(temp_c, humidity_pct);
}

}  // namespace

const __FlashStringHelper* Dht22Sensor::name() const {
  return F("DHT22");
}

bool Dht22Sensor::probe(TwoWire& wire) {
  (void)wire;
  dht.begin();

  float temp_c = 0.0f;
  float humidity_pct = 0.0f;
  if (!readSample(temp_c, humidity_pct)) {
    _present = false;
    return false;
  }

  _present = true;
  return true;
}

bool Dht22Sensor::read(Reading& reading) {
  if (!_present) {
    return false;
  }

  float temp_c = 0.0f;
  float humidity_pct = 0.0f;
  if (!readSample(temp_c, humidity_pct)) {
    return false;
  }

  reading.has_dht22 = true;
  reading.temp_c = temp_c;
  reading.humidity_pct = humidity_pct;
  reading.ok = true;
  return true;
}

void Dht22Sensor::appendBeaconFields(const Reading& reading, char* buf, size_t buf_len) const {
  if (!reading.has_dht22) {
    return;
  }

  char field[48];
  snprintf(field, sizeof(field), "T=%.1f", reading.temp_c);
  appendBeaconField(buf, buf_len, field);
  snprintf(field, sizeof(field), "H=%.0f", reading.humidity_pct);
  appendBeaconField(buf, buf_len, field);
}

#else  // DHT22_PIN

const __FlashStringHelper* Dht22Sensor::name() const {
  return F("DHT22");
}

bool Dht22Sensor::probe(TwoWire& wire) {
  (void)wire;
  _present = false;
  return false;
}

bool Dht22Sensor::read(Reading& reading) {
  (void)reading;
  return false;
}

void Dht22Sensor::appendBeaconFields(const Reading& reading, char* buf, size_t buf_len) const {
  (void)reading;
  (void)buf;
  (void)buf_len;
}

#endif  // DHT22_PIN
