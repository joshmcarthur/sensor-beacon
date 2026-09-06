#include <Arduino.h>
#include <InternalFileSystem.h>
#include <helpers/ArduinoHelpers.h>
#include <helpers/IdentityStore.h>
#include <helpers/SimpleMeshTables.h>
#include <target.h>

#include "BeaconCommon.h"
#include "RepeaterCli.h"
#include "SensorRepeaterApp.h"
#include "config.h"

StdRNG rng;
SimpleMeshTables tables;
SensorRepeaterApp the_mesh(board, radio_driver, *new ArduinoMillis(), rng, rtc_clock, tables);

char beacon_node_name[32];

void setup() {
  beginSerial();
  board.begin();

  if (!radio_init()) {
    Serial.println(F("radio init failed"));
    halt();
  }

  rng.begin(radio_driver.getRngSeed());
  InternalFS.begin();

  IdentityStore store(InternalFS, "");
  loadOrCreateMainIdentity(store, the_mesh.getSelfId());
  strncpy(beacon_node_name, BEACON_NODE_NAME, sizeof(beacon_node_name) - 1);
  beacon_node_name[sizeof(beacon_node_name) - 1] = '\0';

  mesh::GroupChannel beacon_channel;
  if (!loadChannelFromPsk(BEACON_CHANNEL_PSK, beacon_channel)) {
    Serial.println(F("invalid BEACON_CHANNEL_PSK (expected 32 or 64 hex chars)"));
    halt();
  }

  Serial.print(F("node id: "));
  mesh::Utils::printHex(Serial, the_mesh.getSelfId().pub_key, PUB_KEY_SIZE);
  Serial.println();

  logRadioConfig();
  logChannelConfig(beacon_channel, beacon_node_name);

  const uint32_t beacon_sequence = loadBeaconSequence();
  Serial.print(F("beacon seq: "));
  Serial.println(beacon_sequence);

  sensors.begin();
  the_mesh.configure(beacon_channel, beacon_node_name, beacon_sequence);
  the_mesh.beginSensors();
  the_mesh.begin(&InternalFS);
  board.onBootComplete();

  Serial.println(F("boot: ready (repeater)"));
  Serial.flush();
  the_mesh.runBeaconCycle();
}

void loop() {
  pollRepeaterCli(the_mesh);
  the_mesh.loop();
  sensors.loop();
  rtc_clock.tick();

  if (the_mesh.beaconDue(millis())) {
    the_mesh.runBeaconCycle();
  }
}
