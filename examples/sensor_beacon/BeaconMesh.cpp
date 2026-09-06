#include "BeaconMesh.h"

bool BeaconMesh::allowPacketForward(const mesh::Packet* packet) {
  (void)packet;
  return false;
}

int BeaconMesh::searchChannelsByHash(const uint8_t* hash, mesh::GroupChannel channels[],
                                     int max_matches) {
  (void)hash;
  (void)channels;
  (void)max_matches;
  return 0;
}

bool BeaconMesh::sendGroupText(const mesh::GroupChannel& channel, const char* sender_name,
                               const char* text) {
  return ::sendGroupText(*this, channel, sender_name, text);
}

void BeaconMesh::drainTx(uint32_t timeout_ms, uint32_t expected_flood_sends) {
  drainMeshTx(*this, timeout_ms, expected_flood_sends, true, &_packet_mgr);
}
