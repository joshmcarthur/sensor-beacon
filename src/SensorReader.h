#pragma once

#include <Arduino.h>
#include <Wire.h>

#include "SensorReading.h"
#include "sensors/Bh1750Sensor.h"
#include "sensors/Bme280Sensor.h"
#include "sensors/Ltr390Sensor.h"
#include "sensors/Opt3001Sensor.h"
#include "sensors/SensorChannel.h"

// To add a sensor:
// 1. Subclass SensorChannel in src/sensors/
// 2. Add a member below and register it in SensorReader.cpp

class SensorReader {
public:
  SensorReader();

  bool begin(TwoWire& wire, uint16_t (*readBatteryMv)());
  Reading read();
  int formatBeaconMessage(const Reading& reading, char* buf, size_t buf_len, uint32_t seq) const;

private:
  static constexpr size_t kMaxChannels = 8;

  Bme280Sensor _bme280;
  Opt3001Sensor _opt3001;
  Bh1750Sensor _bh1750;
  Ltr390Sensor _ltr390;

  SensorChannel* _channels[kMaxChannels];
  size_t _channel_count = 0;
  uint16_t (*_readBatteryMv)() = nullptr;
};
