#pragma once

#include "SensorChannel.h"

class Opt3001Sensor : public SensorChannel {
public:
  const __FlashStringHelper* name() const override;
  bool probe(TwoWire& wire) override;
  bool read(Reading& reading) override;
  void appendBeaconFields(const Reading& reading, char* buf, size_t buf_len) const override;

private:
  uint8_t _addr = 0;
};
