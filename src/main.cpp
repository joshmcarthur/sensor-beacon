#include <Arduino.h>
#include <InternalFileSystem.h>
#include <base64.hpp>
#include <helpers/ArduinoHelpers.h>
#include <helpers/IdentityStore.h>
#include <helpers/SimpleMeshTables.h>
#include <target.h>

#include "BeaconMesh.h"
#include "SensorReader.h"
#include "config.h"

namespace {

constexpr uint32_t kUsbSerialWaitMs = 5000;

void waitForUsbSerial(uint32_t timeout_ms = kUsbSerialWaitMs) {
  uint32_t start = millis();
  while (!Serial && millis() - start < timeout_ms) {
    delay(10);
  }
  delay(200);
}

void halt() {
  while (true) {
    delay(1000);
  }
}

bool loadChannelFromPsk(const char* psk_base64, mesh::GroupChannel& channel) {
  memset(channel.secret, 0, sizeof(channel.secret));
  int len = decode_base64((unsigned char*)psk_base64, strlen(psk_base64), channel.secret);
  if (len != 16 && len != 32) {
    return false;
  }
  mesh::Utils::sha256(channel.hash, sizeof(channel.hash), channel.secret, len);
  return true;
}

uint16_t readBatteryMv() {
  return board.getBattMilliVolts();
}

}  // namespace

StdRNG rng;
ArduinoMillis arduino_ms;
SimpleMeshTables tables;
StaticPoolPacketManager packet_mgr(8);
BeaconMesh beacon(radio_driver, arduino_ms, rng, rtc_clock, packet_mgr, tables);
SensorReader sensor_reader;

mesh::GroupChannel beacon_channel;

void runBeaconCycle() {
  Reading reading = sensor_reader.read();
  char payload[120];
  int payload_len = sensor_reader.formatBeaconMessage(reading, payload, sizeof(payload));
  if (payload_len <= 0) {
    Serial.println(F("beacon: empty payload"));
    return;
  }

  Serial.print(F("beacon: "));
  Serial.println(payload);

  for (int i = 0; i < BEACON_SEND_COUNT; i++) {
    if (!beacon.sendGroupText(beacon_channel, BEACON_NODE_NAME, payload)) {
      Serial.println(F("beacon: send failed"));
      break;
    }
    if (i + 1 < BEACON_SEND_COUNT) {
      delay(rng.nextInt(50, 250));
    }
  }

  beacon.drainTx(BEACON_TX_DRAIN_MS, BEACON_SEND_COUNT);
  Serial.println(F("beacon: tx drain complete"));
}

void setup() {
  Serial.begin(115200);
  waitForUsbSerial();

  board.begin();

  if (!radio_init()) {
    Serial.println(F("radio init failed"));
    halt();
  }

  radio_driver.begin();
  rng.begin(radio_driver.getRngSeed());

  InternalFS.begin();
  IdentityStore store(InternalFS, "");
  if (!store.load("_main", beacon.self_id)) {
    beacon.self_id = radio_new_identity();
    int count = 0;
    while (count < 10 &&
           (beacon.self_id.pub_key[0] == 0x00 || beacon.self_id.pub_key[0] == 0xFF)) {
      beacon.self_id = radio_new_identity();
      count++;
    }
    store.save("_main", beacon.self_id);
  }

  Serial.print(F("node id: "));
  mesh::Utils::printHex(Serial, beacon.self_id.pub_key, PUB_KEY_SIZE);
  Serial.println();

  if (!loadChannelFromPsk(BEACON_CHANNEL_PSK, beacon_channel)) {
    Serial.println(F("invalid BEACON_CHANNEL_PSK"));
    halt();
  }

  sensor_reader.begin(Wire, readBatteryMv);
  beacon.begin();
  board.onBootComplete();

  Serial.println(F("boot: ready"));
  Serial.flush();
  runBeaconCycle();
}

void loop() {
  rtc_clock.tick();
  runBeaconCycle();
  delay((uint32_t)BEACON_INTERVAL_SECS * 1000UL);
}
