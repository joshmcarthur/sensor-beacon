#include "SensorRepeaterApp.h"

#include "BeaconCommon.h"
#include "BeaconCycle.h"
#include "config.h"

SensorRepeaterApp::SensorRepeaterApp(mesh::MainBoard& board, mesh::Radio& radio,
                                     mesh::MillisecondClock& ms, mesh::RNG& rng,
                                     mesh::RTCClock& rtc, mesh::MeshTables& tables)
    : MyMesh(board, radio, ms, rng, rtc, tables) {}

void SensorRepeaterApp::configure(const mesh::GroupChannel& channel, const char* node_name,
                                  uint32_t sequence) {
  _channel = channel;
  strncpy(_node_name, node_name, sizeof(_node_name) - 1);
  _node_name[sizeof(_node_name) - 1] = '\0';
  _sequence = sequence;
  _cycle = {_channel, _node_name, &_sequence};
}

void SensorRepeaterApp::beginSensors() {
  beginSensorReader(_reader);
}

int SensorRepeaterApp::searchChannelsByHash(const uint8_t* hash, mesh::GroupChannel channels[],
                                            int max_matches) {
  if (max_matches <= 0) {
    return 0;
  }
  if (memcmp(hash, _channel.hash, sizeof(_channel.hash)) == 0) {
    channels[0] = _channel;
    return 1;
  }
  return 0;
}

void SensorRepeaterApp::runBeaconCycle() {
  ::runBeaconCycle(*this, _reader, _cycle, nullptr, false, nullptr);
  _last_beacon_ms = millis();
}

bool SensorRepeaterApp::beaconDue(unsigned long now_ms) const {
  return now_ms - _last_beacon_ms >= BEACON_INTERVAL_SECS * 1000UL;
}
