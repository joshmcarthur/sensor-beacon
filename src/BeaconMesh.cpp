#include "BeaconMesh.h"

#include <Arduino.h>
#include <Packet.h>
#include <helpers/TxtDataHelpers.h>

bool BeaconMesh::allowPacketForward(const mesh::Packet* packet) {
  (void)packet;
  return false;
}

int BeaconMesh::searchChannelsByHash(const uint8_t* hash, mesh::GroupChannel channels[], int max_matches) {
  (void)hash;
  (void)channels;
  (void)max_matches;
  return 0;
}

bool BeaconMesh::sendGroupText(const mesh::GroupChannel& channel, const char* sender_name,
                               const char* text) {
  uint8_t temp[5 + MAX_PACKET_PAYLOAD];
  uint32_t timestamp = getRTCClock()->getCurrentTime();
  memcpy(temp, &timestamp, 4);
  temp[4] = 0;

  char* payload = (char*)&temp[5];
  snprintf(payload, sizeof(temp) - 5, "%s: %s", sender_name, text);
  int len = strlen(payload);
  if (len <= 0) {
    return false;
  }

  mesh::Packet* pkt = createGroupDatagram(PAYLOAD_TYPE_GRP_TXT, channel, temp, (size_t)(5 + len));
  if (!pkt) {
    return false;
  }

  sendFlood(pkt);
  return true;
}

void BeaconMesh::drainTx(uint32_t timeout_ms) {
  unsigned long start = millis();
  while (millis() - start < timeout_ms) {
    loop();
    if (_packet_mgr.getOutboundCount(millis()) == 0) {
      break;
    }
    delay(1);
  }
}
