#include <Arduino.h>
#include <InternalFileSystem.h>
#include <helpers/ArduinoHelpers.h>
#include <helpers/IdentityStore.h>
#include <helpers/SimpleMeshTables.h>
#include <target.h>

#include "BeaconMesh.h"
#include "BeaconPower.h"
#include "SensorReader.h"
#include "config.h"

namespace {

constexpr uint32_t kUsbSerialWaitMs = 5000;
constexpr char kBeaconSeqPath[] = "beacon_seq";

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

bool loadChannelFromPsk(const char* psk_hex, mesh::GroupChannel& channel) {
  memset(channel.secret, 0, sizeof(channel.secret));

  size_t hex_len = strlen(psk_hex);
  int secret_len = 0;
  if (hex_len == 32) {
    if (!mesh::Utils::fromHex(channel.secret, 16, psk_hex)) {
      return false;
    }
    secret_len = 16;
  } else if (hex_len == 64) {
    if (!mesh::Utils::fromHex(channel.secret, 32, psk_hex)) {
      return false;
    }
    secret_len = 32;
  } else {
    return false;
  }

  mesh::Utils::sha256(channel.hash, sizeof(channel.hash), channel.secret, secret_len);
  return true;
}

uint16_t readBatteryMv() {
  return board.getBattMilliVolts();
}

uint32_t loadBeaconSequence() {
  uint32_t seq = 0;
  if (!InternalFS.exists(kBeaconSeqPath)) {
    return seq;
  }
  File f = InternalFS.open(kBeaconSeqPath, FILE_O_READ);
  if (!f) {
    return seq;
  }
  if (f.read((uint8_t*)&seq, sizeof(seq)) != (int)sizeof(seq)) {
    seq = 0;
  }
  f.close();
  return seq;
}

bool saveBeaconSequence(uint32_t seq) {
  File f = InternalFS.open(kBeaconSeqPath, FILE_O_WRITE);
  if (!f) {
    return false;
  }
  bool ok = f.write((uint8_t*)&seq, sizeof(seq)) == sizeof(seq);
  f.close();
  return ok;
}

void logRadioConfig() {
#ifndef LORA_FREQ
#define LORA_FREQ 0
#endif
#ifndef LORA_BW
#define LORA_BW 0
#endif
#ifndef LORA_SF
#define LORA_SF 0
#endif
#ifndef LORA_CR
#define LORA_CR 0
#endif
  Serial.print(F("radio: "));
  Serial.print(LORA_FREQ, 3);
  Serial.print(F(" MHz bw="));
  Serial.print(LORA_BW, 1);
  Serial.print(F(" sf="));
  Serial.print(LORA_SF);
  Serial.print(F(" cr="));
  Serial.println(LORA_CR);
}

}  // namespace

StdRNG rng;
ArduinoMillis arduino_ms;
SimpleMeshTables tables;
StaticPoolPacketManager packet_mgr(8);
BeaconMesh beacon(radio_driver, arduino_ms, rng, rtc_clock, packet_mgr, tables);
SensorReader sensor_reader;

mesh::GroupChannel beacon_channel;
uint32_t beacon_sequence = 0;

void logChannelConfig() {
  Serial.print(F("channel hash: 0x"));
  if (beacon_channel.hash[0] < 0x10) {
    Serial.print('0');
  }
  Serial.println(beacon_channel.hash[0], HEX);
  Serial.print(F("node name: "));
  Serial.println(F(BEACON_NODE_NAME));
}

void logMeshTxStats(uint32_t flood_sent) {
  int pending = packet_mgr.getOutboundCount(millis());
  Serial.print(F("mesh: flood_sent="));
  Serial.print(flood_sent);
  Serial.print(F(" pending="));
  Serial.print(pending);
  Serial.print(F(" total_flood="));
  Serial.println(beacon.getNumSentFlood());
  if (pending > 0) {
    Serial.println(F("mesh: warning: packets still queued (increase drain time or check radio)"));
  }
  if (flood_sent == 0) {
    Serial.println(F("mesh: warning: no flood packets transmitted on air"));
  }
}

void runBeaconCycle() {
  beacon_sequence++;
  if (!saveBeaconSequence(beacon_sequence)) {
    Serial.println(F("beacon: seq save failed"));
  }

  Reading reading = sensor_reader.read();
  char payload[120];
  int payload_len =
      sensor_reader.formatBeaconMessage(reading, payload, sizeof(payload), beacon_sequence);
  if (payload_len <= 0) {
    Serial.println(F("beacon: empty payload"));
    return;
  }

  Serial.print(F("beacon: "));
  Serial.println(payload);

  uint32_t flood_before = beacon.getNumSentFlood();
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
  logMeshTxStats(beacon.getNumSentFlood() - flood_before);
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
  beacon_sequence = loadBeaconSequence();
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
    Serial.println(F("invalid BEACON_CHANNEL_PSK (expected 32 or 64 hex chars)"));
    halt();
  }
  logRadioConfig();
  logChannelConfig();
  Serial.print(F("beacon seq: "));
  Serial.println(beacon_sequence);

  sensor_reader.begin(Wire, readBatteryMv);
  beacon.begin();
  board.onBootComplete();

  Serial.println(F("boot: ready"));
  Serial.flush();
  runBeaconCycle();
}

void loop() {
  rtc_clock.tick();
  beacon.begin();
  runBeaconCycle();
  sleepBetweenBeacons(BEACON_INTERVAL_SECS);
}
