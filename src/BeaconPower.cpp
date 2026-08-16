#include "BeaconPower.h"

#include <Arduino.h>
#include <Wire.h>
#include <target.h>

#if defined(STM32_PLATFORM)
#include <STM32LowPower.h>
#endif

namespace {

void preparePeripheralsForSleep() {
  radio_driver.powerOff();

#ifdef LED_RED
  digitalWrite(LED_RED, HIGH);
#endif
#ifdef LED_GREEN
  digitalWrite(LED_GREEN, HIGH);
#endif
#ifdef LED_BLUE
  digitalWrite(LED_BLUE, HIGH);
#endif
#ifdef PIN_LED
  digitalWrite(PIN_LED, HIGH);
#endif

#ifdef P_LORA_TX_LED
  digitalWrite(P_LORA_TX_LED, HIGH);
#endif

#ifdef VBAT_ENABLE
  pinMode(VBAT_ENABLE, OUTPUT);
  digitalWrite(VBAT_ENABLE, HIGH);
#endif

  Wire.end();
#if defined(PIN_WIRE_SDA) && defined(PIN_WIRE_SCL)
  pinMode(PIN_WIRE_SDA, INPUT);
  pinMode(PIN_WIRE_SCL, INPUT);
#endif
}

bool restorePeripheralsAfterSleep() {
#ifdef VBAT_ENABLE
  digitalWrite(VBAT_ENABLE, LOW);
#endif
  delay(10);

#if defined(PIN_WIRE_SDA) && defined(PIN_WIRE_SCL) && defined(NRF52_PLATFORM)
  Wire.setPins(PIN_WIRE_SDA, PIN_WIRE_SCL);
#endif
  Wire.begin();

  if (!radio_init()) {
    return false;
  }
  radio_driver.begin();
  return true;
}

void sleepUntilMillis(uint32_t wake_at_ms) {
#if defined(STM32_PLATFORM)
  int32_t remaining = (int32_t)(wake_at_ms - millis());
  if (remaining <= 0) {
    return;
  }
  LowPower.deepSleep((uint32_t)remaining);
#else
  while ((int32_t)(wake_at_ms - millis()) > 0) {
    board.sleep(0);
  }
#endif
}

}  // namespace

void sleepBetweenBeacons(uint32_t seconds) {
  if (seconds == 0) {
    return;
  }

#if defined(STM32_PLATFORM)
  static bool low_power_ready = false;
  if (!low_power_ready) {
    LowPower.begin();
    low_power_ready = true;
  }
#endif

  Serial.print(F("sleep: "));
  Serial.print(seconds);
#if defined(STM32_PLATFORM)
  Serial.println(F("s (stop+rtc)"));
#else
  if (board.isExternalPowered()) {
    Serial.println(F("s (usb, system-on idle)"));
  } else {
    Serial.println(F("s (battery, system-on idle)"));
  }
#endif
  Serial.flush();

  preparePeripheralsForSleep();

  uint32_t remaining_ms = seconds * 1000UL;
  while (remaining_ms > 0) {
    uint32_t chunk_ms = remaining_ms > 60000UL ? 60000UL : remaining_ms;
    sleepUntilMillis(millis() + chunk_ms);
    remaining_ms -= chunk_ms;
  }

  if (!restorePeripheralsAfterSleep()) {
    Serial.println(F("sleep: radio restore failed, resetting"));
    Serial.flush();
    NVIC_SystemReset();
  }
}
