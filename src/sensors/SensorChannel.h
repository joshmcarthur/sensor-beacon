#pragma once

#include <Arduino.h>
#include <Wire.h>

#include "../SensorReading.h"

bool i2cDevicePresent(TwoWire& wire, uint8_t addr);

class SensorChannel {
public:
  virtual ~SensorChannel() = default;

  virtual const __FlashStringHelper* name() const = 0;
  virtual bool probe(TwoWire& wire) = 0;
  virtual bool read(Reading& reading) = 0;
  virtual void appendBeaconFields(const Reading& reading, char* buf, size_t buf_len) const = 0;

  bool present() const { return _present; }

protected:
  bool _present = false;
};
