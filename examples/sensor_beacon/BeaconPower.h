#pragma once

#include <stdint.h>

// Low-power sleep between beacon cycles (radio off, MCU in System ON idle).
void sleepBetweenBeacons(uint32_t seconds);
