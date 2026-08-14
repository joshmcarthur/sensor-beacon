# RAK3172 sensor-beacon target

Date: 2026-08-14

## Goal

Add a second PlatformIO environment so the same sensor-beacon firmware runs on a bare RAK3172 (STM32WLE5 + onboard SX126x) as well as the existing Seeed XIAO nRF52840 + Wio-SX1262 stack. Sensors, private-channel `GRP_TXT` flood, identity, and `seq` stay shared. Only board, radio, sleep, and a few HAL details differ.

## Hardware

- Bare RAK3172 stamp module / breakout (not WisBlock). High-power (HP) RF path, 22 dBm.
- I2C sensors on the module’s default Wire pins: **PA11 (SDA)**, **PA12 (SCL)**. Same probe-at-boot set as XIAO (BME280, OPT3001, BH1750, LTR390; optional DHT22 via `DHT22_PIN`).
- No status LED. Indicator blinks are no-ops; diagnostics are serial-only.
- Powered from LiPo (or a 3.3 V rail). No battery divider yet, so **do not report `V=`**.
- Serial and default upload on UART2: **PA2 (TX)**, **PA3 (RX)**, 115200 baud. ST-Link is optional.

## Architecture

New env `rak3172_sensor_beacon` uses MeshCore in library mode with `MC_VARIANT=rak3x72` (RadioLib `CustomSTM32WLx` / `CustomSTM32WLxWrapper`, `SPI_INTERFACES_COUNT=0`). LoRa parameters and `[beacon_secrets]` match the XIAO env so both node types join the same mesh.

Sleep must not call MeshCore’s STM32 `board.powerOff()`: that enters SHUTDOWN and never wakes. Between beacons, put the radio to sleep and enter STM32 STOP with RTC wakeup for `BEACON_INTERVAL_SECS`.

Identity and `seq` use MeshCore’s STM32 `InternalFS` (same Adafruit LittleFS API as nRF52). `pre_build.py` already copies MeshCore’s ed25519 tree into libdeps; extend that pattern so STM32 LittleFS is available to this env. `create_uf2.py` must skip STM32 (hex / UART / ST-Link only).

The XIAO env is unchanged.

## Components

| Piece | Change |
|--------|--------|
| `platformio.ini` | Add `[env:rak3172_sensor_beacon]`: `platform = ststm32`, `board = rak3172`, `MC_VARIANT=rak3x72`, plus MeshCore STM32 needs (`STM32_PLATFORM`, SubGhz include, STM32 helpers). Radio: `RAK_3X72`, `RADIO_CLASS=CustomSTM32WLx`, `WRAPPER_CLASS=CustomSTM32WLxWrapper`, `SPI_INTERFACES_COUNT=0`, `LORA_TX_POWER=22`, `RX_BOOSTED_GAIN`. Shared LoRa flags and secrets. Default `upload_protocol` serial (UART2). |
| `pre_build.py` | Keep `MC_VARIANT` include path. For STM32, add MeshCore `arch/stm32/Adafruit_LittleFS_stm32` into libdeps after MeshCore is present. |
| `create_uf2.py` | No-op when the env is not nRF52. |
| `BeaconPower` | Keep the nRF52 path. STM32 path: radio `powerOff()`, release I2C, STOP+RTC (chunked at 60 s like XIAO), then `Wire.begin()` + `radio_init()` on wake. No `VBAT_ENABLE`, no RGB / TX LED pins. |
| `BeaconIndicator` | No-ops when the board has no LED (RAK3172). |
| `SensorReader` / `main` | Battery callback may be `nullptr`. Append `V=` only when a reader is registered. RAK env does not pass `board.getBattMilliVolts()`. |
| Sensors, `BeaconMesh` | Unchanged. |

MeshCore’s `RAK3x72Board::begin()` still sets PA0/PA1 as outputs (WisBlock LED assumption). This firmware must not treat them as indicators. `getBattMilliVolts()` reads A0 with a WisBlock divider constant; never use it for the beacon payload on this target.

## Boot and loop

1. `board.begin()`, serial on UART2 at 115200. There is no VBUS detect and no LED, so serial debug stays **on by default** (unlike XIAO, which mutes serial on battery). Optional `BEACON_USB_SERIAL_WAIT_MS` still applies. Then `radio_init()`, `InternalFS` + identity, I2C probe, first beacon.
2. Each cycle: persist `seq++`, read sensors, flood `GRP_TXT` `BEACON_SEND_COUNT` times, drain TX.
3. Sleep: radio off, I2C released, STOP+RTC for `BEACON_INTERVAL_SECS`.
4. Wake: restore clocks, `Wire.begin()`, `radio_init()`. Radio restore failure → `NVIC_SystemReset()`.

Payload: `node: seq=…` plus present sensor fields. No `V=` on this target.

## Error handling

- Radio init failure at boot: log and halt (same as XIAO).
- Radio restore failure after sleep: log and reset.
- Invalid `BEACON_CHANNEL_PSK`: halt.
- Seq save failure: log and continue.
- No sensors found: still send `seq=…` only.

## Out of scope

- WisBlock base boards and their pin map / VBAT divider.
- Battery voltage reporting (add later when a divider is wired).
- Status LED.
- XIAO-style `*_measure` / `*_sleep_test` envs for RAK3172.
- Forking MeshCore’s `rak3x72` variant into this repo.
- Default DHT22 pin (still opt-in via `DHT22_PIN`).
- LP (14 dBm) RAK3172 / RAK3172LP-SiP.

## Verification

- `pio run -e rak3172_sensor_beacon` compiles.
- `pio run -e xiao_sensor_beacon` still compiles.
- On hardware: UART boot log, sensor probe line, beacons on the private channel, sleep then wake, payload has no `V=` field.
