#pragma once

#include <MeshCore.h>
#include <Arduino.h>
#include <helpers/NRF52Board.h>

#ifndef USER_BTN_PRESSED
#define USER_BTN_PRESSED LOW
#endif

#ifdef XIAO_NRF52

class XiaoNrf52Board : public NRF52BoardDCDC {
protected:
#if NRF52_POWER_MANAGEMENT
  void initiateShutdown(uint8_t reason) override;
#endif

public:
  XiaoNrf52Board() : NRF52Board("XIAO_NRF52_OTA") {}
  void begin();

#if defined(P_LORA_TX_LED)
  void onBeforeTransmit() override {
    digitalWrite(P_LORA_TX_LED, LOW);
  }
  void onAfterTransmit() override {
    digitalWrite(P_LORA_TX_LED, HIGH);
  }
#endif

  uint16_t getBattMilliVolts() override;

  const char* getManufacturerName() const override {
    return "Seeed Xiao-nrf52";
  }

  void powerOff() override {
    digitalWrite(PIN_LED, LOW);
#ifdef PIN_USER_BTN
    while (digitalRead(PIN_USER_BTN) == USER_BTN_PRESSED);
#endif
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_BLUE, HIGH);
    digitalWrite(PIN_LED, HIGH);

#ifdef PIN_USER_BTN
    nrf_gpio_cfg_sense_input(digitalPinToInterrupt(g_ADigitalPinMap[PIN_USER_BTN]), NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_SENSE_LOW);
#endif

    sd_power_system_off();
  }
};

#endif
