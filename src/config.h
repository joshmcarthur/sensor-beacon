#pragma once

#ifndef BEACON_INTERVAL_SECS
#define BEACON_INTERVAL_SECS 300
#endif

#ifndef BEACON_SEND_COUNT
#define BEACON_SEND_COUNT 3
#endif

#ifndef BEACON_NODE_NAME
#define BEACON_NODE_NAME "beacon"
#endif

#ifndef BEACON_CHANNEL_PSK
#error "Define BEACON_CHANNEL_PSK in platformio.local.ini (see platformio.local.ini.example)"
#endif

#define BEACON_TX_DRAIN_MS 5000
