#include "BeaconPower.h"

#include <Arduino.h>
#include <Wire.h>
#include <target.h>

namespace {

void preparePeripheralsForSleep() {
  radio_driver.powerOff();

  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);
  digitalWrite(PIN_LED, HIGH);

#ifdef P_LORA_TX_LED
  digitalWrite(P_LORA_TX_LED, HIGH);
#endif

  // Disable the battery voltage divider to save ~few µA.
  digitalWrite(VBAT_ENABLE, OUTPUT);
  digitalWrite(VBAT_ENABLE, HIGH);

  Wire.end();
#if defined(PIN_WIRE_SDA) && defined(PIN_WIRE_SCL)
  pinMode(PIN_WIRE_SDA, INPUT);
  pinMode(PIN_WIRE_SCL, INPUT);
#endif
}

bool restorePeripheralsAfterSleep() {
  digitalWrite(VBAT_ENABLE, LOW);
  delay(10);

#if defined(PIN_WIRE_SDA) && defined(PIN_WIRE_SCL)
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
  while ((int32_t)(wake_at_ms - millis()) > 0) {
    board.sleep(0);
  }
}

}  // namespace

void sleepBetweenBeacons(uint32_t seconds) {
  if (seconds == 0) {
    return;
  }

  Serial.print(F("sleep: "));
  Serial.print(seconds);
  if (board.isExternalPowered()) {
    Serial.println(F("s (usb, system-on idle)"));
  } else {
    Serial.println(F("s (battery, system-on idle)"));
  }
  Serial.flush();

  preparePeripheralsForSleep();

  uint32_t remaining_ms = seconds * 1000UL;
  while (remaining_ms > 0) {
    // Sleep in chunks so millis() stays well-behaved across long intervals.
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
