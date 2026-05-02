# Pool Temperature Monitor

Battery-powered ESP32 that measures pool water temperature with a DS18B20 probe and reports to Home Assistant via MQTT over Wi-Fi. Wakes every 3 hours, transmits in ~8 seconds, then deep-sleeps. A single 18650 cell lasts **2+ years**.

---

## Table of Contents

1. [Power Budget](#power-budget)
2. [Bill of Materials](#bill-of-materials)
3. [Wiring Diagram](#wiring-diagram)
4. [Firmware Setup](#firmware-setup)
5. [Home Assistant Setup](#home-assistant-setup)
6. [Enclosure & Waterproofing](#enclosure--waterproofing)
7. [Optimization Tips](#optimization-tips)
8. [Troubleshooting](#troubleshooting)

---

## Power Budget

| Parameter | Value |
|---|---|
| Battery | NCR18650B, 3.7 V, 3400 mAh |
| Usable capacity (80% DoD + regulator losses) | ~2,720 mAh |
| ESP32 deep-sleep current | ~10 µA |
| ESP32 active + Wi-Fi | ~200 mA avg |
| DS18B20 (12-bit conversion) | ~1 mA for 750 ms |
| Wake cycle duration | ~8 s |
| Measurements per day (every 3 h) | 8 |

| Daily Consumption | Value |
|---|---|
| Deep sleep (23 h 59 min) | 0.24 mAh |
| Active cycles (8 × 8 s) | 3.56 mAh |
| **Total** | **~3.8 mAh/day** |

| Projection | Value |
|---|---|
| 6 months (180 days) | 684 mAh (25% of capacity) |
| **Estimated runtime** | **~716 days (~24 months)** |

> **Important**: These numbers assume a bare ESP32 module or a dev board with the power LED disabled. Dev boards with USB-UART chips and LEDs always on will drain 20-50x faster.

---

## Bill of Materials

| # | Component | Specification | Est. Price |
|---|---|---|---|
| 1 | ESP32 Dev Board | ESP32-DevKitC or LOLIN32 Lite | ~4-8 EUR |
| 2 | DS18B20 Waterproof Probe | Stainless steel, 1 m cable, 3-wire | ~3-5 EUR |
| 3 | NCR18650B Battery | 3.7 V, 3400 mAh, protected cell | ~5-8 EUR |
| 4 | 18650 Battery Holder | Single-cell with solder tabs or spring clips | ~1-2 EUR |
| 5 | HT7333 Voltage Regulator | 3.3 V LDO, SOT-89 (Iq = 4 µA) | ~0.50 EUR |
| 6 | 4.7 kΩ Resistor | 1/4 W, for DS18B20 pull-up | ~0.05 EUR |
| 7 | 2× 100 kΩ Resistors | 1/4 W, for battery voltage divider | ~0.10 EUR |
| 8 | 2× 10 µF Capacitors | Ceramic, for LDO input/output decoupling | ~0.20 EUR |
| 9 | Waterproof Enclosure | IP67 ABS junction box, ~100×68×50 mm | ~3-5 EUR |
| 10 | PG7 Cable Gland | Waterproof pass-through for sensor cable | ~0.50 EUR |
| | **Total** | | **~18-30 EUR** |

### Optional Upgrades

- **TP4056 USB-C charging module** (~1 EUR) — allows in-place recharging via USB-C without removing the battery.
- **Solar panel** (5V, 1W, ~5 EUR) + charging circuit — for truly maintenance-free operation.
- **External antenna** — if the pool is far from the Wi-Fi access point.

---

## Wiring Diagram

```
                         ┌────────────────────────────┐
                         │        ESP32 Board          │
                         │                             │
  NCR18650B              │                             │
  ┌─────┐    HT7333      │                             │
  │ BAT+├───►VIN  VOUT───►│ 3V3                        │
  │     │    │    │       │                             │
  │     │  10µF  10µF     │ GPIO16 ──────► DS18B20 VDD │
  │     │    │    │       │                  (red)      │
  │ BAT-├──►GND──►GND────►│ GND ────────► DS18B20 GND │
  └──┬──┘                │                  (black)    │
     │                   │ GPIO4 ────────► DS18B20 DATA│
     │  ┌──100kΩ──┐      │                  (yellow)   │
     └──┤         ├──►   │                             │
        │  ADC    │      │    4.7kΩ pull-up            │
        └──100kΩ──┘      │    between DATA and VDD     │
            │             │                             │
           GND            └────────────────────────────┘

  Battery Voltage Divider:
  VBAT ─── 100kΩ ─── GPIO35 (ADC) ─── 100kΩ ─── GND
```

### Key Connections

| ESP32 Pin | Connects To | Purpose |
|---|---|---|
| 3V3 | HT7333 VOUT | Main power (3.3V) |
| GND | HT7333 GND, BAT-, DS18B20 GND | Common ground |
| GPIO4 | DS18B20 DATA (yellow) | Temperature data (OneWire) |
| GPIO16 | DS18B20 VDD (red) | Powers sensor only when awake |
| GPIO35 | Voltage divider midpoint | Battery level monitoring (ADC) |

### Why power the DS18B20 from a GPIO?

In deep sleep, all GPIOs go LOW. This cuts power to the DS18B20, saving its ~1 mA quiescent draw. Over 6 months this saves ~4,320 mAh — more than the entire battery capacity.

### Why use an HT7333 instead of the onboard regulator?

The ESP32 dev board's AMS1117 regulator has a quiescent current of ~5 mA — that alone would drain the battery in 23 days. The HT7333 draws only 4 µA, making it negligible during deep sleep.

**If using a bare ESP32 module**: Feed 3.3V directly from the HT7333 to the VDD pin.
**If using a dev board**: Power it via the 3V3 pin (bypassing the onboard regulator) OR cut the AMS1117 power LED trace and feed BAT+ to VIN.

---

## Firmware Setup

### Prerequisites

- [PlatformIO](https://platformio.org/install) (VS Code extension or CLI)
- USB cable for initial flashing

### Steps

1. **Clone or copy** the `firmware/` directory.

2. **Edit configuration** in `firmware/include/config.h`:
   ```cpp
   #define WIFI_SSID      "your_wifi_ssid"
   #define WIFI_PASSWORD   "your_wifi_password"
   #define MQTT_SERVER     "192.168.1.100"  // Home Assistant IP
   #define MQTT_USER       "mqtt_user"
   #define MQTT_PASSWORD   "mqtt_pass"
   ```

3. **Build and flash**:
   ```bash
   cd firmware
   # For ESP32:
   pio run -e esp32 -t upload

   # For ESP8266:
   pio run -e esp8266 -t upload
   ```

4. **Monitor serial output** (optional, for debugging):
   ```bash
   pio device monitor -b 115200
   ```

5. **Disconnect USB**, connect battery, and deploy.

### What Happens Each Wake Cycle

1. ESP32 wakes from deep sleep
2. GPIO16 goes HIGH → powers the DS18B20
3. Temperature conversion starts (750 ms)
4. Wi-Fi connects in parallel (~2-5 s)
5. Temperature is read from the DS18B20
6. Battery voltage is read via ADC
7. MQTT discovery config is published (retained)
8. Temperature and battery data are published
9. Wi-Fi and sensor are powered off
10. ESP32 enters deep sleep for 3 hours

---

## Home Assistant Setup

### Prerequisites

- MQTT broker installed (e.g., [Mosquitto add-on](https://github.com/home-assistant/addons/tree/master/mosquitto))
- MQTT integration configured in HA

### Auto-Discovery (Recommended)

The firmware publishes MQTT discovery messages. If MQTT auto-discovery is enabled (it is by default), two entities will appear automatically after the first boot:

- `sensor.pool_temperature` — Water temperature in °C
- `sensor.pool_monitor_battery` — Battery level in %

No manual configuration needed.

### Automations

Copy `homeassistant/automations.yaml` rules into your HA automations:

- **Low battery alert** — notification when battery drops below 20%
- **Offline alert** — notification if no data received for 7 hours
- **Swim-ready alert** — notification when water reaches 24°C

### Dashboard

See `homeassistant/dashboard-card.yaml` for three card options:
1. Simple entities card
2. Rich tile + history graph (7-day temperature chart)
3. Gauge card with color-coded temperature zones

---

## Enclosure & Waterproofing

### Enclosure Requirements

- **IP67 rated** minimum (will be near water and outdoors)
- **ABS or polycarbonate** junction box, ~100 × 68 × 50 mm
- **PG7 cable gland** for the DS18B20 probe cable
- **Silicone sealant** around cable gland and any drill holes

### Assembly

1. Drill a hole in the enclosure bottom for the PG7 cable gland
2. Thread the DS18B20 cable through the gland and tighten
3. Apply silicone sealant around the gland for extra protection
4. Mount the ESP32 and battery holder inside with hot glue or standoffs
5. Wire everything according to the schematic
6. Close the enclosure — ensure the gasket seats properly

### Mounting Options

- **Pool edge**: Zip-tie or screw the box to the pool coping. Run the sensor probe cable down into the water (~30 cm depth).
- **Skimmer box**: Place inside the skimmer housing. The sensor sits in the water flow path.
- **Floating**: Seal the enclosure (use additional silicone) and let it float. Add ballast so the sensor end stays submerged.

### Sensor Placement Tips

- Mount the probe at least **20 cm below the water surface** to avoid sun-heated surface readings.
- Avoid placing it near return jets (artificially warm/cool readings).
- The stainless steel probe is chlorine-resistant — no special coating needed.

---

## Optimization Tips

### Extending Battery Life Even Further

1. **Use a bare ESP32 module** (ESP32-WROOM-32) instead of a dev board. Saves 10-50 mA by eliminating USB-UART chips and LEDs.

2. **Static IP address** — Skipping DHCP saves 1-3 seconds of Wi-Fi time. Add to `config.h`:
   ```cpp
   #define USE_STATIC_IP    true
   #define STATIC_IP        "192.168.1.50"
   #define GATEWAY_IP       "192.168.1.1"
   #define SUBNET_MASK      "255.255.255.0"
   ```

3. **Increase sleep interval** — Measuring every 6 hours instead of 3 halves the active consumption (still plenty for a pool).

4. **Use ESP-NOW instead of MQTT** — If you have a second ESP32 plugged into power acting as a gateway, ESP-NOW connects in ~10 ms vs. ~3 s for Wi-Fi+MQTT. This reduces active time 100x.

5. **Disable brownout detector** on ESP32 (saves ~1 µA in deep sleep):
   ```cpp
   #include "soc/soc.h"
   #include "soc/rtc_cntl_reg.h"
   WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
   ```

### About the 1.5V USB-C Battery

You mentioned a "Type C Li-Po rechargeable battery (1.5V, 3400 mAh)". These are USB-C rechargeable AA cells with a built-in boost converter that outputs a constant 1.5V. **They won't work directly** because:
- The ESP32 needs 3.3V minimum
- The built-in boost converter wastes power
- You can't stack them (2 × 1.5V = 3.0V, still under 3.3V)

**Use a standard NCR18650B** (3.7V nominal, 3400 mAh) with the HT7333 LDO regulator instead. This is the standard solution for battery-powered ESP32 projects.

---

## Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| No Wi-Fi connection | Wrong credentials or weak signal | Check `config.h`. Move device closer to AP or add an external antenna. |
| Temperature reads -127°C | DS18B20 wiring error | Check data pin, pull-up resistor (4.7 kΩ between DATA and VDD), and power pin. |
| Battery drains in weeks | Dev board power LED / USB chip | Cut the power LED trace or switch to a bare ESP32 module. Verify deep-sleep current with a multimeter (should be < 20 µA). |
| Sensor not in HA | MQTT misconfiguration | Check broker IP, credentials. Verify with `mosquitto_sub -t '#'` that messages arrive. |
| Inaccurate battery % | ADC calibration | The ESP32 ADC is nonlinear. For better accuracy, use `esp_adc_cal` calibration or map voltage-to-percentage with a lookup table from your specific cell's discharge curve. |
| Won't wake from sleep | ESP8266: GPIO16 not connected to RST | Wire D0 (GPIO16) to RST pin. Required for deep-sleep wake on ESP8266. |
| Readings drift over time | Sensor fouled or corroded | Clean the probe. Stainless DS18B20 probes resist pool chemicals well but inspect annually. |

---

## Project Structure

```
pool-temp-monitor/
├── README.md                          # This file
├── firmware/
│   ├── platformio.ini                 # PlatformIO build configuration
│   ├── include/
│   │   └── config.h                   # Wi-Fi, MQTT, pin, timing settings
│   └── src/
│       └── main.cpp                   # Main firmware (wake → read → publish → sleep)
└── homeassistant/
    ├── configuration.yaml             # Optional manual MQTT sensor config
    ├── automations.yaml               # Battery, offline, and swim-ready alerts
    └── dashboard-card.yaml            # Lovelace dashboard cards (3 options)
```

---

## License

This project is provided as-is for personal/educational use. No warranty expressed or implied.
