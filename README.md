# sensor-beacon

Minimal MeshCore firmware for **Seeed Xiao nRF52840 + Wio SX1262** nodes that read optional I2C sensors and flood compact `GRP_TXT` beacons on a **private group channel**.

One firmware binary serves a heterogeneous fleet: BME280/DHT22 (temp/humidity), OPT3001/BH1750 (lux), and LTR390 (UV) are probed once at boot; absent sensors are skipped.

## Hardware

- **MCU / radio:** Xiao nRF52840 + Wio SX1262 (LoRa D1–D5, SPI D8–D10, I2C D6/D7)
- **Optional sensors:** BME280 or DHT22 (temp/humidity), OPT3001 or BH1750 (lux), LTR390 (UV) (any subset per node)
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

Generate a 16-byte random PSK as 32 hex characters:

```bash
openssl rand -hex 16
```

Copy `platformio.local.ini.example` to `platformio.local.ini` and set the `[beacon_secrets]` section only. Do **not** replace the full `[env:xiao_sensor_beacon]` block — that drops radio and mesh build flags.

Example:

```ini
[beacon_secrets]
build_flags =
  -D BEACON_NODE_NAME='"garden01"'
  -D BEACON_CHANNEL_PSK='"7b014167e5fe21a36ec407c86de92eb0"'
```

## Collector setup

1. In the MeshCore companion app, create a **private channel** with the same PSK (32 hex chars, 16 bytes).
2. The channel **name** is only a label in the app — routing uses the PSK hash, so any name works as long as the key matches.
3. Confirm messages arrive as `node_name: T=… H=… P=… L=… UV=… V=…` (only present fields).
4. Traffic stays on your channel hash — not the default public channel.

## Message format

Variable key=value fields after the node name prefix. `seq` is a monotonic counter persisted across reboots (useful when the RTC is unset):

```
garden01: seq=42 T=18.2 H=94 P=1012 L=320 UV=4.2 V=3.85
shed02: seq=7 T=18.2 H=94 P=1012 V=3.85
post03: seq=108 L=120 UV=2.1 V=3.90
```

Battery (`V=`) is always included. Message timestamps in the app come from the sender RTC (placeholder on XIAO without an I2C RTC); use `seq` to order beacons.

## Configuration

| Define | Default | Description |
|--------|---------|-------------|
| `BEACON_INTERVAL_SECS` | 300 | Seconds between beacon cycles (System ON idle; SX1262 asleep) |
| `BEACON_SEND_COUNT` | 3 | Flood sends per cycle (with jitter) |
| `DHT22_PIN` | *(off)* | Enable DHT22 on a GPIO, e.g. `D0` (see below) |
| `BEACON_NODE_NAME` | `beacon` | Prefix in GRP_TXT |
| `LORA_FREQ` / `BW` / `SF` / `CR` | 917.375 / 62.5 / 7 / 5 | Must match your main node (`get radio`) |

## Power / sleep

Between beacon cycles the firmware enters **System ON idle** (not a busy `delay()`):

- SX1262 is put in sleep via `radio_driver.powerOff()`
- LEDs off, battery voltage divider disabled, I2C bus released
- MCU sleeps in `board.sleep()` until the next interval (FreeRTOS tickless idle)

The nRF52840 cannot wake itself on a timer from **SYSTEMOFF** (clocks stop), so timed beacons use System ON idle instead. That still drops current dramatically once the radio is asleep — the LoRa module was the main draw. Serial shows `sleep: 300s (battery, system-on idle)` before each nap.

## Architecture

- **MeshCore** via `lib_deps` (library mode, `MC_VARIANT=xiao_nrf52` for board/radio glue)
- **`SensorReader`** — orchestrates pluggable I2C `SensorChannel` drivers (probe at boot, read each cycle)
- **`BeaconMesh`** — thin `mesh::Mesh` subclass; no forwarding, private-channel flood only
- **Loop:** wake → read → flood 2–3× → drain TX → deep idle sleep (`BEACON_INTERVAL_SECS`)

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
