#pragma once

#ifndef BEACON_INTERVAL_SECS
#define BEACON_INTERVAL_SECS 300
#endif

#ifndef BEACON_SEND_COUNT
#define BEACON_SEND_COUNT 3
#endif

#ifndef BEACON_NODE_NAME
#error "Define BEACON_NODE_NAME in platformio.local.ini (see platformio.local.ini.example)"
#endif

#ifndef BEACON_CHANNEL_PSK
#error "Define BEACON_CHANNEL_PSK in platformio.local.ini (see platformio.local.ini.example)"
#endif

#define BEACON_TX_DRAIN_MS 5000

// Include V= in the beacon when a battery ADC path exists (XIAO yes, bare RAK3172 no).
#ifndef BEACON_HAS_VBAT
#define BEACON_HAS_VBAT 1
#endif
