#pragma once

#ifndef BEACON_INTERVAL_SECS
#define BEACON_INTERVAL_SECS 300
#endif

#ifndef BEACON_SEND_COUNT
#define BEACON_SEND_COUNT 3
#endif

#ifndef BEACON_NODE_PREFIX
#define BEACON_NODE_PREFIX "beacon"
#endif

#ifndef BEACON_CHANNEL_PSK
#error "Define BEACON_CHANNEL_PSK in platformio.local.ini (see platformio.local.ini.example)"
#endif

#define BEACON_TX_DRAIN_MS 5000

// Include V= in the beacon when a battery ADC path exists.
#ifndef BEACON_HAS_VBAT
#define BEACON_HAS_VBAT 1
#endif

#ifndef BEACON_VBAT_REF_MV
#define BEACON_VBAT_REF_MV 3300
#endif

#ifndef BEACON_VBAT_MULTIPLIER
#define BEACON_VBAT_MULTIPLIER 1
#endif

#ifndef BEACON_VBAT_DIVISOR
#define BEACON_VBAT_DIVISOR 1
#endif
