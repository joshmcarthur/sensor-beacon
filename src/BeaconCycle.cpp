#include "BeaconCycle.h"

#include <Arduino.h>
#include <Packet.h>
#include <helpers/TxtDataHelpers.h>
#include <target.h>

#include "BeaconCommon.h"
#include "config.h"

bool sendGroupText(mesh::Mesh& mesh, const mesh::GroupChannel& channel, const char* sender_name,
                   const char* text) {
  uint8_t temp[5 + MAX_PACKET_PAYLOAD];
  uint32_t timestamp = mesh.getRTCClock()->getCurrentTime();
  memcpy(temp, &timestamp, 4);
  temp[4] = 0;

  char* payload = (char*)&temp[5];
  snprintf(payload, sizeof(temp) - 5, "%s: %s", sender_name, text);
  int len = strlen(payload);
  if (len <= 0) {
    return false;
  }

  mesh::Packet* pkt = mesh.createGroupDatagram(PAYLOAD_TYPE_GRP_TXT, channel, temp, (size_t)(5 + len));
  if (!pkt) {
    return false;
  }

  mesh.sendFlood(pkt);
  return true;
}

void drainMeshTx(mesh::Mesh& mesh, uint32_t timeout_ms, uint32_t expected_flood_sends,
                 bool power_off_radio, StaticPoolPacketManager* outbound_queue) {
  const uint32_t start_flood = mesh.getNumSentFlood();
  const unsigned long start = millis();
  while (millis() - start < timeout_ms) {
    mesh.loop();
    const uint32_t sent = mesh.getNumSentFlood() - start_flood;
    const bool queue_empty =
        outbound_queue == nullptr || outbound_queue->getOutboundCount(millis()) == 0;
    if (sent >= expected_flood_sends && queue_empty) {
      for (uint8_t i = 0; i < 5; i++) {
        mesh.loop();
        delay(2);
      }
      break;
    }
    delay(1);
  }

  if (power_off_radio) {
    radio_driver.powerOff();
  }
}

namespace {

void logMeshTxStats(mesh::Mesh& mesh, StaticPoolPacketManager* outbound_queue,
                    uint32_t flood_sent) {
  if (outbound_queue == nullptr) {
    return;
  }
  int pending = outbound_queue->getOutboundCount(millis());
  Serial.print(F("mesh: flood_sent="));
  Serial.print(flood_sent);
  Serial.print(F(" pending="));
  Serial.print(pending);
  Serial.print(F(" total_flood="));
  Serial.println(mesh.getNumSentFlood());
  if (pending > 0) {
    Serial.println(F("mesh: warning: packets still queued (increase drain time or check radio)"));
  }
  if (flood_sent == 0) {
    Serial.println(F("mesh: warning: no flood packets transmitted on air"));
  }
}

void beaconJitter(mesh::RNG* rng) {
  if (rng != nullptr) {
    delay(rng->nextInt(50, 250));
  } else {
    delay(random(50, 250));
  }
}

}  // namespace

void runBeaconCycle(mesh::Mesh& mesh, SensorReader& reader, BeaconCycleConfig& cfg, mesh::RNG* rng,
                    bool power_off_radio, StaticPoolPacketManager* outbound_queue) {
  (*cfg.sequence)++;
  if (!saveBeaconSequence(*cfg.sequence)) {
    Serial.println(F("beacon: seq save failed"));
  }

  Reading reading = reader.read();
  char payload[120];
  int payload_len =
      reader.formatBeaconMessage(reading, payload, sizeof(payload), *cfg.sequence);
  if (payload_len <= 0) {
    Serial.println(F("beacon: empty payload"));
    return;
  }

  Serial.print(F("beacon: "));
  Serial.println(payload);

  const uint32_t flood_before = mesh.getNumSentFlood();
  for (int i = 0; i < BEACON_SEND_COUNT; i++) {
    if (!sendGroupText(mesh, cfg.channel, cfg.node_name, payload)) {
      Serial.println(F("beacon: send failed"));
      break;
    }
    if (i + 1 < BEACON_SEND_COUNT) {
      beaconJitter(rng);
    }
  }

  drainMeshTx(mesh, BEACON_TX_DRAIN_MS, BEACON_SEND_COUNT, power_off_radio, outbound_queue);
  logMeshTxStats(mesh, outbound_queue, mesh.getNumSentFlood() - flood_before);
  Serial.println(F("beacon: tx drain complete"));
}
