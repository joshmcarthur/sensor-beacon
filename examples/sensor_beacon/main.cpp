#include <Arduino.h>
#include <InternalFileSystem.h>
#include <helpers/ArduinoHelpers.h>
#include <helpers/IdentityStore.h>
#include <helpers/SimpleMeshTables.h>
#include <target.h>

#include "BeaconCommon.h"
#include "BeaconCycle.h"
#include "BeaconMesh.h"
#include "BeaconPower.h"
#include "SensorReader.h"
#include "config.h"

StdRNG rng;
ArduinoMillis arduino_ms;
SimpleMeshTables tables;
StaticPoolPacketManager packet_mgr(8);
BeaconMesh beacon(radio_driver, arduino_ms, rng, rtc_clock, packet_mgr, tables);
SensorReader sensor_reader;

mesh::GroupChannel beacon_channel;
uint32_t beacon_sequence = 0;
BeaconCycleConfig beacon_cycle;

void runBeaconCycle() {
  ::runBeaconCycle(beacon, sensor_reader, beacon_cycle, &rng, true, &packet_mgr);
}

void setup() {
  beginSerial();
  board.begin();

  if (!radio_init()) {
    Serial.println(F("radio init failed"));
    halt();
  }

  radio_driver.begin();
  rng.begin(radio_driver.getRngSeed());

  InternalFS.begin();
  beacon_sequence = loadBeaconSequence();
  IdentityStore store(InternalFS, "");
  loadOrCreateMainIdentity(store, beacon.self_id);

  Serial.print(F("node id: "));
  mesh::Utils::printHex(Serial, beacon.self_id.pub_key, PUB_KEY_SIZE);
  Serial.println();

  if (!loadChannelFromPsk(BEACON_CHANNEL_PSK, beacon_channel)) {
    Serial.println(F("invalid BEACON_CHANNEL_PSK (expected 32 or 64 hex chars)"));
    halt();
  }

  beacon_cycle = {beacon_channel, BEACON_NODE_NAME, &beacon_sequence};
  logRadioConfig();
  logChannelConfig(beacon_channel, BEACON_NODE_NAME);
  Serial.print(F("beacon seq: "));
  Serial.println(beacon_sequence);

  beginSensorReader(sensor_reader);
  beacon.begin();
  board.onBootComplete();

  Serial.println(F("boot: ready"));
  Serial.flush();
  runBeaconCycle();
}

void loop() {
  rtc_clock.tick();
  runBeaconCycle();
  sleepBetweenBeacons(BEACON_INTERVAL_SECS);
}
