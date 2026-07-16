#include "SensorChannel.h"

bool i2cDevicePresent(TwoWire& wire, uint8_t addr) {
  wire.beginTransmission(addr);
  return wire.endTransmission() == 0;
}
