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
BeaconMesh mesh(radio_driver, arduino_ms, rng, rtc_clock, packet_mgr, tables);
SensorReader sensors;

mesh::GroupChannel beacon_channel;

void runBeaconCycle() {
  Reading reading = sensors.read();
  char payload[120];
  int payload_len = formatBeaconMessage(reading, payload, sizeof(payload));
  if (payload_len <= 0) {
    Serial.println(F("beacon: empty payload"));
    return;
  }

  Serial.print(F("beacon: "));
  Serial.println(payload);

  for (int i = 0; i < BEACON_SEND_COUNT; i++) {
    if (!mesh.sendGroupText(beacon_channel, BEACON_NODE_NAME, payload)) {
      Serial.println(F("beacon: send failed"));
      break;
    }
    if (i + 1 < BEACON_SEND_COUNT) {
      delay(rng.nextInt(50, 250));
    }
  }

  mesh.drainTx(BEACON_TX_DRAIN_MS);
  Serial.println(F("beacon: tx drain complete"));
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  board.begin();

  if (!radio_init()) {
    Serial.println(F("radio init failed"));
    halt();
  }

  radio_driver.begin();
  rng.begin(radio_driver.getRngSeed());

  InternalFS.begin();
  IdentityStore store(InternalFS, "");
  if (!store.load("_main", mesh.self_id)) {
    mesh.self_id = radio_new_identity();
    int count = 0;
    while (count < 10 &&
           (mesh.self_id.pub_key[0] == 0x00 || mesh.self_id.pub_key[0] == 0xFF)) {
      mesh.self_id = radio_new_identity();
      count++;
    }
    store.save("_main", mesh.self_id);
  }

  Serial.print(F("node id: "));
  mesh::Utils::printHex(Serial, mesh.self_id.pub_key, PUB_KEY_SIZE);
  Serial.println();

  if (!loadChannelFromPsk(BEACON_CHANNEL_PSK, beacon_channel)) {
    Serial.println(F("invalid BEACON_CHANNEL_PSK"));
    halt();
  }

  sensors.begin(Wire, readBatteryMv);
  mesh.begin();
  board.onBootComplete();

  runBeaconCycle();
}

void loop() {
  rtc_clock.tick();
  runBeaconCycle();
  delay((uint32_t)BEACON_INTERVAL_SECS * 1000UL);
}
