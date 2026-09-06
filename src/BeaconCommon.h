#pragma once

#include <Mesh.h>
#include <stdint.h>

#include "config.h"

class IdentityStore;
class SensorReader;

constexpr size_t kBeaconNodeNameMax = 32;

void beginSerial();
void halt();

void formatBeaconNodeName(char* dest, size_t dest_len, const uint8_t* pub_key);
bool loadChannelFromPsk(const char* psk_hex, mesh::GroupChannel& channel);

uint32_t loadBeaconSequence();
bool saveBeaconSequence(uint32_t seq);

void loadOrCreateMainIdentity(IdentityStore& store, mesh::LocalIdentity& id);
void beginSensorReader(SensorReader& reader);

void logRadioConfig();
void logChannelConfig(const mesh::GroupChannel& channel, const char* node_name);

#if BEACON_HAS_VBAT
uint16_t readBatteryMv();
#endif
