# sensor-beacon

Minimal MeshCore firmware for **Seeed Xiao nRF52840 + Wio SX1262** nodes that read optional I2C sensors and flood compact `GRP_TXT` beacons on a **private group channel**.

One firmware binary serves a heterogeneous fleet: BME280, OPT3001 (lux), and LTR390 (UV) are probed once at boot; absent sensors are skipped.

## Hardware

- **MCU / radio:** Xiao nRF52840 + Wio SX1262 (LoRa D1–D5, SPI D8–D10, I2C D6/D7)
- **Optional sensors:** BME280, OPT3001, LTR390 (any subset per node)
- **Battery:** read via Xiao ADC (`board.getBattMilliVolts()`)

## Quick start

```bash
cp platformio.local.ini.example platformio.local.ini
# Edit [beacon_secrets]: BEACON_NODE_NAME and BEACON_CHANNEL_PSK

pio run -e xiao_sensor_beacon
pio run -e xiao_sensor_beacon -t upload
pio device monitor
```

## Secrets (`platformio.local.ini`)

Generate a 16-byte random PSK and base64-encode it:

```bash
openssl rand -base64 16
```

Copy `platformio.local.ini.example` to `platformio.local.ini` and set the `[beacon_secrets]` section only. Do **not** replace the full `[env:xiao_sensor_beacon]` block — that drops radio and mesh build flags.

Example:

```ini
[beacon_secrets]
build_flags =
  -D BEACON_NODE_NAME='"garden01"'
  -D BEACON_CHANNEL_PSK='"izOH6cXN6mrJ5e26oRXNcg=="'
```

## Collector setup

1. In the MeshCore companion app, create a **private channel** with the same PSK.
2. Confirm messages arrive as `node_name: T=… H=… P=… L=… UV=… V=…` (only present fields).
3. Traffic stays on your channel hash — not the default public channel.

## Message format

Variable key=value fields after the node name prefix:

```
garden01: T=18.2 H=94 P=1012 L=320 UV=4.2 V=3.85
shed02: T=18.2 H=94 P=1012 V=3.85
post03: L=120 UV=2.1 V=3.90
```

Battery (`V=`) is always included.

## Configuration

| Define | Default | Description |
|--------|---------|-------------|
| `BEACON_INTERVAL_SECS` | 300 | Seconds between beacon cycles |
| `BEACON_SEND_COUNT` | 3 | Flood sends per cycle (with jitter) |
| `BEACON_NODE_NAME` | `beacon` | Prefix in GRP_TXT |
| `LORA_FREQ` / `BW` / `SF` / `CR` | 916.575 / 62.5 / 7 / 8 | Australia/NZ (Narrow); match your local mesh |

## Architecture

- **MeshCore** via `lib_deps` (library mode, `MC_VARIANT=xiao_nrf52` for board/radio glue)
- **`SensorReader`** — local I2C probe + reads (not MeshCore `EnvironmentSensorManager`)
- **`BeaconMesh`** — thin `mesh::Mesh` subclass; no forwarding, private-channel flood only
- **Loop:** wake → read → flood 2–3× → drain TX → `delay(interval)`

## Field verification checklist

- [ ] `pio run -e xiao_sensor_beacon` compiles
- [ ] Serial: `sensors: …` at boot, `beacon: …` each cycle, `tx drain complete`
- [ ] Flash two hardware configs; confirm field set matches sensors present
- [ ] Companion receives on private channel
- [ ] 24 h stability run
- [ ] No traffic on public default channel

## License

MIT — see [LICENSE](LICENSE).
