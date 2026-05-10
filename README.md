# Pool Temperature Monitor

Battery-powered **ESP32 (Lolin32 Lite)** that measures pool water temperature
with a DS18B20 probe and reports to **Home Assistant via the ESPHome native
API** over Wi-Fi. Wakes every **2 hours**, publishes in ~5–10 seconds, then
deep-sleeps. A single charged **NCR18650B** is sized for **≥9 months** of
unattended operation.

The firmware is a single ESPHome YAML file ([`pooltemperature.yaml`](pooltemperature.yaml))
designed for critical-infrastructure stability: layered OTA recovery (no
GPIO button required), `safe_mode` crash recovery, multi-AP fallback, NVS-
persisted state across battery swaps, and full self-disclosure via diagnostic
sensors in HA.

---

## Table of Contents

1. [Power Budget](#power-budget)
2. [Bill of Materials](#bill-of-materials)
3. [Wiring](#wiring)
4. [Firmware (ESPHome)](#firmware-esphome)
5. [Home Assistant Integration](#home-assistant-integration)
6. [OTA Recovery Design](#ota-recovery-design)
7. [Bench Test Procedure](#bench-test-procedure)
8. [Enclosure & Waterproofing](#enclosure--waterproofing)
9. [Troubleshooting](#troubleshooting)
10. [Project Structure](#project-structure)

---

## Power Budget

Targets ≥9 months on a single charged NCR18650B above the 3.2 V cutoff.

| Parameter | Value |
|---|---|
| Battery | NCR18650B, 3.6 V nominal, 3400 mAh |
| Usable capacity above 3.2 V cutoff | ~3000 mAh |
| Lolin32 Lite deep-sleep current (typical) | ~150 µA |
| Voltage divider draw (1 MΩ : 1 MΩ at 4 V) | ~2 µA (negligible) |
| Active avg current (WiFi assoc + DS18B20 + publish) | ~120 mA |
| Active duration per wake | ~7 s |
| Wake cadence | **every 2 h (12 wakes/day)** |

| Per-cycle consumption | Value |
|---|---|
| Active 7 s × 120 mA | 0.233 mAh |
| Sleep ~2 h × 0.15 mA | 0.300 mAh |
| **Total per 2 h cycle** | **~0.53 mAh** |
| Per day (12 cycles) | ~6.4 mAh |

| Projection | Value |
|---|---|
| 9 months (~270 days) | **~1730 mAh** (≈58 % of usable) |
| Estimated runtime | **~14 months** before reaching 3.2 V cutoff |

> The ~14-month estimate is the *modelled* number. Real-world variation
> (WiFi retry storms, cold weather, marginal RSSI) typically shaves 20–30 %.
> The 9-month requirement has comfortable headroom; if real-world
> measurement after the first month projects > 2700 mAh / 9 months, swap
> the substitution `sleep_duration: "2h"` → `"3h"` in `pooltemperature.yaml`
> and re-flash.

### Why these numbers and not 2+ years

The Lolin32 Lite has a low-quiescent LDO and no USB-UART chip, giving
~150 µA in deep sleep — roughly 15× better than a generic ESP32-DevKitC
but still ~15× worse than a bare ESP32-WROOM module on a custom PCB. A
bare-module build with DS18B20 power-switching and lower active current
could plausibly reach 2+ years, but the Lolin32 Lite design (chosen for
ease of build and the integrated battery connector) is the realistic
optimum here.

---

## Bill of Materials

| # | Component | Specification | Notes |
|---|---|---|---|
| 1 | **Lolin32 Lite** ESP32 dev board | ESP32-WROOM-32, integrated 18650-friendly LDO, JST-PH battery connector | Critical: this exact board (or one with comparable sleep current). Generic ESP32-DevKitC will drain in weeks. |
| 2 | DS18B20 waterproof probe | Stainless steel, ≥1 m cable, 3-wire (red / yellow / black) | |
| 3 | NCR18650B Li-Ion cell | 3.6 V nominal, 3400 mAh, **protected** preferred | Charge to 4.1 V (per the SOC table used in firmware), not 4.2 V, for longer cycle life. |
| 4 | 18650 holder or JST-PH pigtail | Whatever mates with the Lolin32 Lite battery connector | |
| 5 | 4.7 kΩ resistor | 1/4 W | DS18B20 data-line pull-up to 3V3 |
| 6 | 2 × 1 MΩ resistors | 1/4 W, 1 % tolerance ideal | Battery voltage divider (see wiring) |
| 7 | Waterproof enclosure | IP67 ABS junction box, ~100 × 68 × 50 mm | |
| 8 | PG7 cable gland | Waterproof pass-through for the probe cable | |

**Why 1 MΩ : 1 MΩ for the divider?** At 4 V, a 100 kΩ : 100 kΩ divider
draws 20 µA *continuously* (160 mAh/year) — meaningful for a
9-month battery target. 1 MΩ : 1 MΩ draws only 2 µA (16 mAh/year),
negligible. ESP32 ADC input impedance can handle 1 MΩ source impedance
fine when `samples: 16` averaging is used (already configured in YAML).

---

## Wiring

```
 NCR18650B → JST-PH → Lolin32 Lite battery connector (built-in LDO → 3V3 rail)

 Lolin32 Lite                       DS18B20 probe
 ┌─────────────┐                    ┌──────────┐
 │       3V3   │── 4.7kΩ ──┐        │          │
 │             │           │        │          │
 │      GPIO16 │───────────┼────────┤ VDD (red)│
 │       GPIO4 │───────────┴────────┤ DATA(yel)│
 │         GND │────────────────────┤ GND(blk) │
 │             │                    └──────────┘
 │      GPIO35 │◄── voltage-divider midpoint
 │             │
 │         BAT │── (cell + via JST-PH)
 │         GND │── (cell − via JST-PH)
 └─────────────┘

 Battery voltage divider (high impedance, low quiescent):
       BAT+ ── 1 MΩ ── GPIO35 ── 1 MΩ ── GND
```

| ESP32 Pin | Wire | Purpose |
|---|---|---|
| `GPIO4` | DS18B20 DATA (yellow) | OneWire data, 4.7 kΩ pull-up to 3V3 |
| `GPIO16` | DS18B20 VDD (red) | Switched power rail; goes LOW in deep sleep |
| `GND` | DS18B20 GND (black) | Common ground |
| `GPIO35` | Divider midpoint | Battery voltage on ADC1 (works with WiFi active) |

### Why power the DS18B20 from a GPIO?

`GPIO16` goes LOW automatically when the ESP32 enters deep sleep, killing
sensor power. The DS18B20's idle current (~1 µA) is barely worth saving,
but the GPIO16 wiring also lets the firmware re-power the sensor cleanly
on wake — eliminating any "stuck bus" recovery work.

### Why ADC1 (GPIO32–39) and not ADC2?

ADC2 is silently disabled when WiFi is active on ESP32. GPIO35 is on
ADC1, so the battery reading works during WiFi association.

---

## Firmware (ESPHome)

The firmware is `pooltemperature.yaml` in this directory. It runs in the
ESPHome dashboard (Docker image `ghcr.io/esphome/esphome:latest`).

### Validate

```bash
docker exec esphome esphome config /config/pooltemperature.yaml
```

Expect `Configuration is valid!`.

### Compile + flash

First flash via USB (serial):

```bash
docker exec esphome esphome run /config/pooltemperature.yaml
```

Subsequent flashes via OTA (during an active OTA window — see below):

```bash
docker exec esphome esphome run /config/pooltemperature.yaml --device 192.168.1.86
```

### Stream wireless logs (during the awake window)

```bash
docker exec esphome esphome logs /config/pooltemperature.yaml --device 192.168.1.86
```

### What happens each wake cycle (normal path)

1. ESP32 wakes from deep sleep → `esp_reset_reason() == ESP_RST_DEEPSLEEP`.
   Reboot counter is **not** incremented (wake-from-sleep doesn't count).
2. `GPIO16` goes HIGH → DS18B20 powers up.
3. WiFi associates with `Outdoor_AP` (priority 10), falls back to
   `Aruba_HPE_WLAN` (priority 5). `fast_connect: true` reuses the last
   successful AP first, saving 1.5–3 s.
4. ESPHome native API connects to Home Assistant (encrypted).
5. Script `read_and_publish` triggers DS18B20 conversion (12-bit, 750 ms),
   reads battery voltage (16-sample average), waits 2 s for publishes
   to drain.
6. Device enters deep sleep for 2 h.

### What happens on a "real" reboot

Real reboots = RST button press, battery swap, HA software-restart,
panic, brownout. Detected by `esp_reset_reason() != ESP_RST_DEEPSLEEP`.
The persistent `reboot_count` (in NVS, survives battery swap) increments.
If `reboot_count % 10 == 0` (or HA set the `ota_request_pending` flag),
the dispatcher runs `ota_window` instead of `read_and_publish` — the
device stays awake **2 minutes** for OTA upload, then deep-sleeps.

---

## Home Assistant Integration

This firmware uses the **ESPHome native API** (encrypted), not MQTT. After
flashing, HA's ESPHome integration will auto-discover the device. Add it
once via **Settings → Devices & Services → ESPHome**, paste the API
encryption key from `secrets.yaml` (`pooltemperature_key`), and HA exposes
the entities below.

### Entities

| Entity | Purpose |
|---|---|
| `sensor.pool_water_temperature` | Water temperature (°C, 2 decimals) |
| `sensor.pooltemperature_battery_voltage` | Cell voltage (V, 3 decimals) |
| `sensor.pooltemperature_battery_level` | SOC percentage (NCR18650B 5-point curve) |
| `sensor.pooltemperature_wifi_signal` | RSSI (dBm) |
| `sensor.pooltemperature_uptime` | Seconds since wake |
| `sensor.pooltemperature_reboot_count` | Total real reboots (persistent) |
| `sensor.pooltemperature_wakes_until_ota_window` | 0..9 — `0` means OTA window is open right now |
| `text_sensor.pooltemperature_reset_reason` | Why the device booted (e.g. `deep_sleep_wake`, `power_on`, `panic`) |
| `text_sensor.pooltemperature_ip_address`, `_connected_ssid`, `_connected_bssid` | WiFi info |
| `text_sensor.pooltemperature_esphome_version` | Build version visible in HA |
| `button.pooltemperature_restart` | HA-triggered restart (counts as real reboot) |

### NCR18650B State of Charge

The `sensor.pooltemperature_battery_level` lambda implements the spec:

| SOC | Voltage |
|---|---|
| 100 % | 4.100 V |
| 75 %  | 3.875 V |
| 50 %  | 3.650 V |
| 25 %  | 3.425 V |
| 0 %   | 3.200 V |

Spacing is uniform (0.225 V per 25 %), so the curve is linear and
implemented as `pct = (V − 3.2) / 0.9 × 100`, clamped to [0, 100].

### HA action: `enter_ota_mode`

The firmware exposes a callable action on the device. From HA's
**Developer Tools → Actions**:

```yaml
action: esphome.pooltemperature_enter_ota_mode
data: {}
```

This sets the persistent `ota_request_pending` flag (saved to NVS). On the
**next** wake the device sees the flag, consumes it, and stays awake 2
minutes for OTA upload.

### Legacy MQTT/dashboard examples

The `homeassistant/` subdirectory contains MQTT-based discovery, automations,
and Lovelace cards from the previous PlatformIO firmware revision. They are
kept for reference but **do not apply** to the current ESPHome native-API
firmware. If you want updated automations (low-battery alert, offline alert)
for the native-API entities, port them against the entity IDs above.

---

## OTA Recovery Design

The hard problem with battery-powered deep-sleep devices is: **how do you
push new firmware to a device that's only reachable for 10 seconds every
2 hours, without adding hardware?** Three layered paths:

| # | Path | Trigger | When to use |
|---|---|---|---|
| 1 | **HA action** `enter_ota_mode` | Call action in HA → device sees flag on next wake | Routine updates |
| 2 | **Reboot counter** | Press RST 10 times (or do 10 HA software-restarts, or some mix) | API/HA broken; you only have physical access |
| 3 | **`safe_mode`** | A new firmware crashes within 1 min of boot, 10 times in a row | Bad OTA bricked the device; opens a permanent OTA listener until reflashed |

The reboot counter is stored in **NVS** via ESPHome `globals` with
`restore_value: true` — this survives a battery swap (RTC memory doesn't).
**Only "real" reboots count**: the `esp_reset_reason()` check at boot
priority 800 increments the counter unless the cause is `ESP_RST_DEEPSLEEP`.
Routine 2-hour deep-sleep wakes do **not** chip away at the counter.

The HA OTA flag `ota_request_pending` is also NVS-persisted, so even if
HA goes offline between you setting the flag and the device's next wake,
the flag is still there.

`sensor.pooltemperature_wakes_until_ota_window` shows in HA how many real
reboots remain before the next counter-driven OTA window.

---

## Bench Test Procedure

Before deploying to the pool, validate the OTA-recovery logic on the bench:

1. **Flash via USB** the production firmware (`docker exec esphome esphome run …`).
2. **Watch the first wake on serial / wireless logs**. Expect:
   - `Reset reason code: 1` (POWERON) → `Real reboot — counter now 1`
   - WiFi associates with `Outdoor_AP` (or `Aruba_HPE_WLAN` fallback).
   - Pool temp + battery published.
   - `Readings published — entering deep sleep for 2h`.
3. **Press RST 9 more times**, waiting ~5 s between presses (so the device
   actually boots each time and writes the counter to NVS).
4. **On the 10th press**, expect:
   - `Real reboot — counter now 10`
   - `OTA window: 10-th real reboot`
   - `OTA window open — staying awake 2min`
5. While the window is open, push an OTA update to verify the path works.
6. **Reset the counter** for clean operation: in HA Developer Tools, call
   the device's `restart` button 10× more, or just leave the counter
   running — it'll naturally hit the next multiple of 10 again over
   subsequent battery swaps / RST presses.
7. **Verify the HA path** independently: call
   `esphome.pooltemperature_enter_ota_mode` action, then wait for (or
   force) the next wake. Expect:
   - `OTA window: HA-requested via enter_ota_mode`
   - 2-minute window, OTA succeeds.

After this passes, deploy to the pool.

---

## Enclosure & Waterproofing

### Enclosure

- IP67 rated minimum (will be near water and outdoors)
- ABS or polycarbonate junction box, ~100 × 68 × 50 mm
- PG7 cable gland for the DS18B20 cable
- Silicone sealant around the gland and any drilled holes

### Assembly

1. Drill a hole in the enclosure bottom for the PG7 cable gland.
2. Thread the DS18B20 cable through the gland and tighten.
3. Apply silicone sealant around the gland for extra protection.
4. Mount the Lolin32 Lite and battery holder inside.
5. Wire per the schematic; check the 4.7 kΩ pull-up and divider continuity
   with a multimeter before sealing.
6. Close the enclosure — verify the gasket seats properly.

### Mounting options

- **Pool edge / coping**: zip-tie or screw the box to the coping. Run the
  probe down into the water (~30 cm depth).
- **Skimmer box**: place the enclosure inside the skimmer housing; the
  probe sits in the water flow path.
- **Floating**: seal the enclosure (extra silicone) and let it float; add
  ballast so the probe stays submerged.

### Sensor placement tips

- Mount the probe **≥20 cm below the surface** to avoid sun-heated
  surface readings.
- Avoid placing it near return jets (artificially warm/cool readings).
- The stainless steel probe is chlorine-resistant — no special coating
  needed; inspect annually for fouling.

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| Device never appears in HA after first flash | API encryption key mismatch | Check `pooltemperature_key` in `secrets.yaml` matches what HA's ESPHome integration was set up with. |
| Device shows "Unavailable" in HA after a few hours | Working as designed — only awake for ~10 s every 2 h | The diagnostic sensors update in HA only during the wake window. Use **last-changed** timestamps to confirm liveness. |
| Temperature reads `-127.0 °C` or NaN | DS18B20 wiring error | Verify GPIO4 data line, the 4.7 kΩ pull-up between data and 3V3, and that GPIO16 is actually going HIGH on wake. Check the address `0x8448695309646128` matches your sensor (capture from boot log). |
| Battery percentage shows wildly wrong values | Divider resistor tolerance / cold solder joint | Measure the actual resistor values; if they're not exactly 1 MΩ, adjust the lambda or use 1 % tolerance parts. |
| Device never wakes after deep sleep | `sleep_duration` typo, or hardware reset via brownout | Check the `sleep_duration: "2h"` substitution. Verify battery is actually charged (RTC stops if voltage drops below ~2.5 V). |
| Battery drains in days, not months | Wrong board (not Lolin32 Lite), or DS18B20 / divider drawing more than expected | Measure deep-sleep current with a USB-meter or µCurrent at the cell — should be ~150–200 µA for a stock Lolin32 Lite. If much higher, check for: extra LEDs, regulator quiescent, or accidental `ALWAYS_ON` on a high-current pin. |
| OTA flash fails mid-upload | Network glitch, OTA window expired | Trigger another OTA window (HA action or RST 10 times), retry. `safe_mode` will catch it on the next boot if firmware was actually corrupted. |
| Device boots into safe mode | A previous OTA crashed within 1 min of boot, 10 times | safe_mode keeps WiFi + OTA up indefinitely — flash a known-good firmware to recover. |
| Awake window > 15 s | Slow WiFi association (weak signal, AP under load) | Check RSSI in HA. If consistently below −75 dBm, relocate the device or AP. Consider raising `output_power: 17.0` → `20.0`. |

### A note on USB-C "1.5 V" rechargeable batteries

USB-C rechargeable AA cells (1.5 V output via internal boost converter)
**won't work** for this project: the ESP32 needs ≥3.0 V, the boost
converter wastes power, and stacking them is awkward. Use a real
NCR18650B with the Lolin32 Lite's built-in LDO.

---

## Project Structure

```
esp32_lolin32_lite/
├── README.md                ← this file
├── pooltemperature.yaml     ← ESPHome firmware (the actual build)
├── secrets.yaml             ← WiFi / OTA / API encryption keys (gitignored)
├── pooltemperature.factory.bin   ← captured factory image (legacy snapshot)
├── lolin32-lite-front.jpeg, lolin32-lite-rear.jpeg
├── schaltplan.jpg, wiring.png   ← schematic and wiring photos
├── poolmeter_DE.md          ← German design notes
├── SESSION_CHECKPOINT.md    ← running log of design decisions
├── HOW-TO-USE-GSD.md        ← project-specific GSD workflow notes
├── 00-prompt.txt            ← original brief
│
├── firmware/                ← LEGACY: prior PlatformIO/Arduino C++ build
│   ├── platformio.ini       ←   not the active firmware
│   ├── include/config.h
│   └── src/main.cpp
│
└── homeassistant/           ← LEGACY: prior MQTT-based HA config
    ├── configuration.yaml
    ├── automations.yaml
    └── dashboard-card.yaml
```

The active firmware is **`pooltemperature.yaml`** (ESPHome native API).
The `firmware/` and `homeassistant/` subdirectories are kept as
historical reference from the prior PlatformIO/MQTT revision.

---

## License

See [LICENSE](LICENSE).
