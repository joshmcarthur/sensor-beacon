#pragma once

#include <Mesh.h>
#include <helpers/StaticPoolPacketManager.h>

#include "SensorReader.h"

struct BeaconCycleConfig {
  mesh::GroupChannel channel;
  const char* node_name;
  uint32_t* sequence;
};

bool sendGroupText(mesh::Mesh& mesh, const mesh::GroupChannel& channel, const char* sender_name,
                   const char* text);

void drainMeshTx(mesh::Mesh& mesh, uint32_t timeout_ms, uint32_t expected_flood_sends,
                 bool power_off_radio, StaticPoolPacketManager* outbound_queue = nullptr);

void runBeaconCycle(mesh::Mesh& mesh, SensorReader& reader, BeaconCycleConfig& cfg, mesh::RNG* rng,
                    bool power_off_radio, StaticPoolPacketManager* outbound_queue = nullptr);
