#pragma once

#include "BeaconCommon.h"
#include "BeaconCycle.h"

#ifndef ADMIN_PASSWORD
#error "Define ADMIN_PASSWORD in platformio.local.ini ([repeater_secrets]). See platformio.local.ini.example."
#endif
#ifndef ADVERT_NAME
#error "Define ADVERT_NAME in platformio.local.ini ([repeater_secrets]). See platformio.local.ini.example."
#endif

#include "MyMesh.h"
#include "SensorReader.h"

class SensorRepeaterApp : public MyMesh {
  mesh::GroupChannel _channel{};
  char _node_name[kBeaconNodeNameMax]{};
  SensorReader _reader;
  uint32_t _sequence = 0;
  BeaconCycleConfig _cycle{};
  unsigned long _last_beacon_ms = 0;

public:
  SensorRepeaterApp(mesh::MainBoard& board, mesh::Radio& radio, mesh::MillisecondClock& ms,
                    mesh::RNG& rng, mesh::RTCClock& rtc, mesh::MeshTables& tables);

  void configure(const mesh::GroupChannel& channel, const char* node_name, uint32_t sequence);
  void beginSensors();

  int searchChannelsByHash(const uint8_t* hash, mesh::GroupChannel channels[],
                           int max_matches) override;

  void runBeaconCycle();
  bool beaconDue(unsigned long now_ms) const;
};
