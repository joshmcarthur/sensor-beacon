#pragma once

#include <Mesh.h>
#include <helpers/StaticPoolPacketManager.h>

class BeaconMesh : public mesh::Mesh {
  StaticPoolPacketManager& _packet_mgr;

public:
  BeaconMesh(mesh::Radio& radio, mesh::MillisecondClock& ms, mesh::RNG& rng,
             mesh::RTCClock& rtc, StaticPoolPacketManager& mgr, mesh::MeshTables& tables)
      : mesh::Mesh(radio, ms, rng, rtc, mgr, tables), _packet_mgr(mgr) {}

  bool sendGroupText(const mesh::GroupChannel& channel, const char* sender_name,
                     const char* text);

  void drainTx(uint32_t timeout_ms, uint32_t expected_flood_sends = 1);

  float getAirtimeBudgetFactor() const override { return 0.0f; }
  bool allowPacketForward(const mesh::Packet* packet) override;
  int getInterferenceThreshold() const override { return 0; }
  int searchChannelsByHash(const uint8_t* hash, mesh::GroupChannel channels[], int max_matches) override;

private:
  StaticPoolPacketManager& packetMgr() { return _packet_mgr; }
};
