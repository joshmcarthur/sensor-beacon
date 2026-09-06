#include "BeaconCommon.h"

#include <Arduino.h>
#include <InternalFileSystem.h>
#include <Wire.h>
#include <helpers/IdentityStore.h>
#include <target.h>

#include "SensorReader.h"
#include "config.h"

using Adafruit_LittleFS_Namespace::File;
using Adafruit_LittleFS_Namespace::FILE_O_READ;
using Adafruit_LittleFS_Namespace::FILE_O_WRITE;

namespace {

constexpr char kBeaconSeqPath[] = "beacon_seq";

}  // namespace

void beginSerial() {
  constexpr uint32_t kUsbSerialWaitMs = 5000;
  Serial.begin(115200);
#if defined(STM32_PLATFORM)
  Serial.println(F("boot: uart"));
  Serial.flush();
#else
  uint32_t start = millis();
  while (!Serial && millis() - start < kUsbSerialWaitMs) {
    delay(10);
  }
  delay(200);
#endif
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

void loadOrCreateMainIdentity(IdentityStore& store, mesh::LocalIdentity& id) {
  if (store.load("_main", id)) {
    return;
  }
  id = radio_new_identity();
  for (int count = 0; count < 10 && (id.pub_key[0] == 0x00 || id.pub_key[0] == 0xFF); count++) {
    id = radio_new_identity();
  }
  store.save("_main", id);
}

void beginSensorReader(SensorReader& reader) {
#if BEACON_HAS_VBAT
  reader.begin(Wire, readBatteryMv);
#else
  reader.begin(Wire, nullptr);
#endif
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

void logChannelConfig(const mesh::GroupChannel& channel, const char* node_name) {
  Serial.print(F("channel hash: 0x"));
  if (channel.hash[0] < 0x10) {
    Serial.print('0');
  }
  Serial.println(channel.hash[0], HEX);
  Serial.print(F("node name: "));
  Serial.println(node_name);
}

#if BEACON_HAS_VBAT
uint16_t readBatteryMv() {
  return board.getBattMilliVolts();
}
#endif
