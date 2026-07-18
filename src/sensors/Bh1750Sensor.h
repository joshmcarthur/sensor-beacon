#pragma once

#include "SensorChannel.h"

class Bh1750Sensor : public SensorChannel {
public:
  const __FlashStringHelper* name() const override;
  bool probe(TwoWire& wire) override;
  bool read(Reading& reading) override;
  void appendBeaconFields(const Reading& reading, char* buf, size_t buf_len) const override;

private:
  TwoWire* _wire = nullptr;
  uint8_t _addr = 0;

  bool measureLux(float& lux) const;
};
