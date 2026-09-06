# sensor-beacon

Minimal MeshCore firmware for **Seeed Xiao nRF52840 + Wio SX1262** nodes that read optional I2C sensors and flood compact `GRP_TXT` beacons on a **private group channel**.

One firmware binary serves a heterogeneous fleet: BME280/DHT22 (temp/humidity), OPT3001/BH1750 (lux), and LTR390 (UV) are probed once at boot; absent sensors are skipped.

## Hardware

This firmware targets the **Seeed Studio XIAO nRF52840** MCU stacked on a **Wio-SX1262 for XIAO** LoRa carrier. Optional I2C (or 1-wire) sensors hang off the shared bus; each node can carry any supported subset of sensors.

### Bill of materials

| Item | Notes |
|------|-------|
| [XIAO nRF52840](https://www.seeedstudio.com/Seeed-XIAO-Bluetooth5-BLE-nRF52840-Sense-Microcontroller-p-4425.html) | Standard or **Sense** variant (Sense adds onboard IMU/mic — unused by this firmware) |
| [Wio-SX1262 for XIAO](https://www.seeedstudio.com/Wio-SX1262-for-XIAO-p-6379.html) | Semtech SX1262, 862–930 MHz |
| LoRa antenna | U.FL/IPEX pigtail matched to your band (e.g. 915 MHz NA, 868 MHz EU) |
| USB-C cable | Power and firmware upload |
| LiPo battery *(optional)* | 3.7 V single-cell, JST or solder to `BAT+` / `BAT−` pads |
| I2C breakout(s) *(optional)* | BME280, OPT3001, BH1750, LTR390 — any subset per node |

The pre-assembled [XIAO nRF52840 & Wio-SX1262 Kit](https://www.seeedstudio.com/XIAO-nRF52840-Wio-SX1262-Kit-for-Meshtastic-p-6400.html) (SKU 102010710) ships with the **V1.0** Wio PCB pinout this repo expects.

### Physical assembly

1. **Stack the boards** — seat the XIAO nRF52840 on the Wio-SX1262 castellated connector (USB-C on the XIAO faces out). Press firmly until all pins mate.
2. **Attach the antenna** before transmitting — never power the radio without an antenna or dummy load.
3. **Sensors** — wire I2C breakouts to `3V3`, `GND`, `D6` (SCL), and `D7` (SDA).
4. **Battery** *(optional)* — connect a 3.7 V LiPo to the XIAO `BAT+` / `BAT−` pads. The onboard charger tops up from USB-C (50 mA or 100 mA depending on `HICHG` strap; default is 50 mA).

### Wio-SX1262 PCB revision (important)

Seeed ships two Wio-SX1262 PCB layouts. **This firmware is built for V1.0** — the layout bundled with the nRF52840 Meshtastic kit:

| Signal | V1.0 (this repo) | V1.1 (ESP32-S3 kit PCB) |
|--------|------------------|-------------------------|
| NSS / CS | D4 | D3 |
| DIO1 | D1 | D1 |
| BUSY | D3 | D3 |
| RESET | D2 | D2 |
| RXEN | D5 | D4 |

If you transplant a Wio module from an **XIAO ESP32-S3** kit onto an nRF52840, the radio pins differ and this binary will not work without changing the `P_LORA_*` / `SX126X_*` defines in `platformio.ini`. See the [Wio-SX1262 schematic (V1.0)](https://files.seeedstudio.com/products/SenseCAP/Wio_SX1262/Wio-SX1262%20for%20XIAO%20V1.0_SCH.pdf).

### Pin map (as used by this firmware)

| XIAO pin | Function in sensor-beacon |
|----------|---------------------------|
| D0 | User button (`PIN_BUTTON1`); optional DHT22 data pin |
| D1 | LoRa DIO1 |
| D2 | LoRa RESET |
| D3 | LoRa BUSY |
| D4 | LoRa NSS (SPI chip select) |
| D5 | LoRa RXEN |
| D6 | I2C SCL (`PIN_WIRE_SCL`) |
| D7 | I2C SDA (`PIN_WIRE_SDA`) |
| D8 | SPI SCK |
| D9 | SPI MISO |
| D10 | SPI MOSI |
| D11 | LoRa TX activity LED (red, on Wio carrier) |
| 3V3 / GND | Sensor and module power |

LoRa SPI and control lines are consumed by the Wio stack — do not reassign D1–D5 or D8–D10. **D0** is the only castellated GPIO free for 1-wire sensors when using the default pinout.

### Flashing and serial console

- Connect USB-C. The board enumerates as a serial port (115200 baud in `platformio.ini`).
- Upload: `pio run -e xiao_sensor_beacon -t upload` (uses `nrfutil` / Adafruit bootloader).
- If upload fails, **double-tap the reset button** quickly to enter the UF2/bootloader mode, then retry.
- Serial monitor: `pio device monitor` — boot prints detected sensors and each beacon cycle.
- Optional SWD debug: `debug_tool = jlink` in `platformio.ini`; SWD pads are on the XIAO underside.

On first boot over USB, firmware waits up to 5 s for a serial connection before continuing (handy for catching early log lines).

### Power and battery monitoring

| Source | Details |
|--------|---------|
| USB-C | 5 V input; powers the board and charges an attached LiPo |
| BAT pads | 3.0–4.2 V LiPo; onboard charger, ~50 µA standby target once radio is asleep |
| Beacon field `V=` | Battery voltage via `board.getBattMilliVolts()` (1 MΩ / 512 kΩ divider, ×3.0 multiplier) |

Between beacon cycles the firmware disables the battery voltage divider and puts the SX1262 to sleep to minimise idle draw. The nRF52840 stays in **System ON idle** (not SYSTEMOFF) so the RTC can wake for the next interval — see [Power / sleep](#power--sleep).

Use a cell with protection (PCM) and size it for your beacon interval and flood count. There is no solar/LDO path in this repo; add external regulation if you need 5 V sensors.

### I2C sensor bus

All I2C sensors share **D6 (SCL)** and **D7 (SDA)** at 3.3 V logic. Drivers probe once at boot and skip anything not found.

| Sensor | Bus | Addresses probed | Beacon fields |
|--------|-----|------------------|---------------|
| BME280 | I2C | `0x76`, `0x77` | `T=` `H=` `P=` |
| OPT3001 | I2C | `0x44`–`0x47` | `L=` |
| BH1750 | I2C | `0x23`, `0x5C` | `L=` |
| LTR390 | I2C | `0x53` | `UV=` |
| DHT22 | 1-wire on GPIO | `D0` (configurable) | `T=` `H=` — see [DHT22](#dht22-1-wire-not-i2c) |

Wire all breakouts in parallel on the same bus (each needs its own I2C address). Use one temp/humidity source per node — don't combine BME280 and DHT22.

Example hookup for a BME280 breakout:

| BME280 | XIAO |
|--------|------|
| VCC | 3V3 |
| GND | GND |
| SCL | D6 |
| SDA | D7 |

### Seeed reference docs

- [XIAO nRF52840 wiki](https://wiki.seeedstudio.com/XIAO_BLE) — pinout, battery charging, power notes
- [XIAO nRF52840 & Wio-SX1262 kit getting started](https://wiki.seeedstudio.com/xiao_nrf52840&_wio_SX1262_kit_for_meshtastic/)
- [XIAO nRF52840 schematic (PDF)](https://files.seeedstudio.com/wiki/XIAO-BLE/nRF52840-Seeed-Studio-XIAO-nRF52840-Schematic.pdf)
- [Wio-SX1262 for XIAO schematic (PDF)](https://files.seeedstudio.com/products/SenseCAP/Wio_SX1262/Wio-SX1262%20for%20XIAO%20V1.0_SCH.pdf)

## Quick start

```bash
cp platformio.local.ini.example platformio.local.ini
# Edit [beacon_secrets]: BEACON_CHANNEL_PSK (required); optional BEACON_NODE_PREFIX

pio run -e xiao_sensor_beacon
pio run -e xiao_sensor_beacon -t upload
pio device monitor
```

## Secrets (`platformio.local.ini`)

Generate a 16-byte random PSK as 32 hex characters:

```bash
openssl rand -hex 16
```

Copy `platformio.local.ini.example` to `platformio.local.ini` and set `[beacon_secrets]` (required). Repeater builds also need `[repeater_secrets]`. Do **not** replace the full `[env:…]` blocks — that drops radio and mesh build flags.

Example:

```ini
[beacon_secrets]
build_flags =
  -D BEACON_CHANNEL_PSK='"7b014167e5fe21a36ec407c86de92eb0"'
  ; Optional site label, e.g. garden01-a3f1c2 (default "beacon"):
  ; -D BEACON_NODE_PREFIX='"garden01"'
```

## Collector setup

1. In the MeshCore companion app, create a **private channel** with the same PSK (32 hex chars, 16 bytes).
2. The channel **name** is only a label in the app — routing uses the PSK hash, so any name works as long as the key matches.
3. Confirm messages arrive as `node_name: T=… H=… P=… L=… UV=… V=…` (only present fields).
4. Traffic stays on your channel hash — not the default public channel.

## Message format

Variable key=value fields after the node name prefix (`{BEACON_NODE_PREFIX}-{6 hex of pub_key}`). `seq` is a monotonic counter persisted across reboots (useful when the RTC is unset):

```
garden01-a3f1c2: seq=42 T=18.2 H=94 P=1012 L=320 UV=4.2 V=3.85
shed02-b4e2d1: seq=7 T=18.2 H=94 P=1012 V=3.85
post03-c5f3e2: seq=108 L=120 UV=2.1 V=3.90
```

Battery (`V=`) is always included. Message timestamps in the app come from the sender RTC (placeholder on XIAO without an I2C RTC); use `seq` to order beacons.

## Configuration

| Define | Default | Description |
|--------|---------|-------------|
| `BEACON_INTERVAL_SECS` | 300 | Seconds between beacon cycles (System ON idle; SX1262 asleep) |
| `BEACON_SEND_COUNT` | 3 | Flood sends per cycle (with jitter) |
| `DHT22_PIN` | *(off)* | Enable DHT22 on a GPIO, e.g. `D0` (see below) |
| `BEACON_NODE_PREFIX` | `beacon` | Site label in GRP_TXT (`garden01-a3f1c2`) |
| `LORA_FREQ` / `BW` / `SF` / `CR` | 917.375 / 62.5 / 7 / 5 | Must match your main node (`get radio`) |

## Power / sleep

Between beacon cycles the firmware enters **System ON idle** (not a busy `delay()`):

- SX1262 is put in sleep via `radio_driver.powerOff()`
- LEDs off, battery voltage divider disabled, I2C bus released
- MCU sleeps in `board.sleep()` until the next interval (FreeRTOS tickless idle)

The nRF52840 cannot wake itself on a timer from **SYSTEMOFF** (clocks stop), so timed beacons use System ON idle instead. That still drops current dramatically once the radio is asleep — the LoRa module was the main draw. Serial shows `sleep: 300s (battery, system-on idle)` before each nap.

## Sensor repeater (always-on hybrid)

For high-point nodes that **relay mesh traffic** and **send their own sensor beacons**, use a repeater env instead of the leaf beacon:

| Env | Hardware | Role |
|-----|----------|------|
| `xiao_sensor_repeater` | XIAO + Wio-SX1262 | Prototype / moderate traffic |
| `rak3172_sensor_repeater` | RAK3172-T | Production high-point node |

```bash
pio run -e xiao_sensor_repeater -t upload
pio run -e rak3172_sensor_repeater -t upload
```

Repeater firmware subclasses MeshCore's `MyMesh` (full repeater forwarding) and schedules the same `GRP_TXT` beacon cycle as leaf nodes on `BEACON_CHANNEL_PSK`. The radio stays in RX between beacons — expect **~20–40 mA average** (mains/solar/large battery), not coin-cell duty cycles.

Serial CLI is available at 115200 — set `ADMIN_PASSWORD` and `ADVERT_NAME` in `platformio.local.ini` `[repeater_secrets]` before building (see `platformio.local.ini.example`). Use `BEACON_NODE_PREFIX` to distinguish repeater senders in the companion app.

## Architecture

```
src/                      # shared library (SensorReader, sensors/, config.h)
examples/sensor_beacon/   # leaf TX-only firmware (BeaconMesh, deep sleep)
examples/sensor_repeater/ # always-on hybrid (MyMesh subclass + beacon timer)
```

- **MeshCore** via `lib_deps` (library mode, `MC_VARIANT` for board/radio glue)
- **`SensorReader`** — orchestrates pluggable I2C `SensorChannel` drivers (probe at boot, read each cycle)
- **`BeaconMesh`** (leaf only) — thin `mesh::Mesh` subclass; no forwarding, private-channel flood only
- **Leaf loop:** wake → read → flood 2–3× → drain TX → deep idle sleep (`BEACON_INTERVAL_SECS`)
- **Repeater loop:** relay continuously → scheduled sensor beacon → stay in RX

Sensor drivers live in `src/sensors/`. Each driver subclasses `SensorChannel` and implements probe, read, and beacon formatting. See [Adding a sensor](#adding-a-sensor).

### DHT22 (1-wire, not I2C)

**DHT22 / AM2302 only** — DHT11 uses different 1-wire timing and is not supported by this driver.

DHT22 uses a **single data pin** — not the I2C bus. On XIAO + Wio SX1262, **D0** is the usual choice (D1–D7 are LoRa or I2C).

Enable in `platformio.local.ini` or `platformio.ini`:

```ini
build_flags =
  -D DHT22_PIN=D0
```

Wiring (typical 3-pin module with onboard 10k pull-up):

| DHT22 | XIAO |
|-------|------|
| VCC | 3.3V |
| GND | GND |
| DATA | D0 (or your `DHT22_PIN`) |

Beacon fields match BME280: `T=` and `H=` (no pressure). Use **one** temp/humidity sensor per node — don't combine BME280 and DHT22 on the same board.

One XIAO = one DHT22. Your spare modules go on other beacon nodes.

## Adding a sensor

Use an existing driver as a template — `Bme280Sensor` (simple `begin()` probe) or `Opt3001Sensor` (manufacturer/device ID check).

### 1. Add the library dependency

If the sensor needs a new Arduino library, add it under `[arduino_base]` → `lib_deps` in `platformio.ini`.

### 2. Extend `Reading` (if needed)

Add fields and `has_*` flags to `src/SensorReading.h` for any new measurements your beacon will carry.

### 3. Implement `SensorChannel`

Create `src/sensors/MySensor.h` and `src/sensors/MySensor.cpp`:

```cpp
// MySensor.h
#pragma once
#include "SensorChannel.h"

class MySensor : public SensorChannel {
public:
  const __FlashStringHelper* name() const override;
  bool probe(TwoWire& wire) override;
  bool read(Reading& reading) override;
  void appendBeaconFields(const Reading& reading, char* buf, size_t buf_len) const override;

private:
  TwoWire* _wire = nullptr;
  uint8_t _addr = 0;
};
```

```cpp
// MySensor.cpp (sketch)
#include "MySensor.h"
#include "BeaconFields.h"
#include <MySensorLib.h>

const __FlashStringHelper* MySensor::name() const { return F("MYSENSOR"); }

bool MySensor::probe(TwoWire& wire) {
  _wire = &wire;
  // Detect device on the bus; set _addr and _present = true on success.
  // Use i2cDevicePresent() for a quick ACK check; verify chip IDs when the
  // library's begin() is unreliable (see Opt3001Sensor).
  return _present;
}

bool MySensor::read(Reading& reading) {
  if (!_present) return false;
  // Read hardware, set reading.has_mysensor / values, reading.ok = true.
  return true;
}

void MySensor::appendBeaconFields(const Reading& reading, char* buf, size_t buf_len) const {
  if (!reading.has_mysensor) return;
  char field[48];
  snprintf(field, sizeof(field), "X=%.1f", reading.my_value);
  appendBeaconField(buf, buf_len, field);
}
```

PlatformIO compiles all `src/**/*.cpp` automatically — no build config change needed.

### 4. Register with `SensorReader`

In `src/SensorReader.h`:

- `#include "sensors/MySensor.h"`
- Add a member: `MySensor _my_sensor;`

In `src/SensorReader.cpp` constructor:

```cpp
_channels[_channel_count++] = &_my_sensor;
```

Keep `_channel_count` below `kMaxChannels` (default 8).

### 5. Build and verify

```bash
pio run -e xiao_sensor_beacon
```

On hardware, confirm serial boot line lists the new sensor and beacon fields appear only when the device is connected:

```
sensors: BME280 MYSENSOR
beacon: T=18.2 … X=1.0 V=3.85
```

Battery (`V=`) is appended by `SensorReader` — drivers do not need to handle it.

## Field verification checklist

- [ ] `pio run -e xiao_sensor_beacon` compiles
- [ ] Serial: `sensors: …` at boot, `beacon: …` each cycle, `tx drain complete`
- [ ] Flash two hardware configs; confirm field set matches sensors present
- [ ] Companion receives on private channel
- [ ] 24 h stability run
- [ ] No traffic on public default channel

## License

MIT — see [LICENSE](LICENSE).
