#pragma once
// ============================================================
//  Pool Temperature Monitor — Configuration
// ============================================================
//  Adjust the values below to match your setup, then build.
// ============================================================

// ----- Wi-Fi ------------------------------------------------
#define WIFI_SSID         "YOUR_WIFI_SSID"
#define WIFI_PASSWORD     "YOUR_WIFI_PASSWORD"

// ----- MQTT (Home Assistant) --------------------------------
#define MQTT_SERVER       "192.168.1.100"   // Home Assistant IP
#define MQTT_PORT         1883
#define MQTT_USER         "mqtt_user"        // leave "" if no auth
#define MQTT_PASSWORD     "mqtt_password"
#define MQTT_CLIENT_ID    "pool_temp"

// MQTT topics (Home Assistant auto-discovery uses these)
#define MQTT_TOPIC_TEMP   "homeassistant/sensor/pool_temp/state"
#define MQTT_TOPIC_BATT   "homeassistant/sensor/pool_battery/state"
#define MQTT_TOPIC_CONFIG_TEMP "homeassistant/sensor/pool_temp/config"
#define MQTT_TOPIC_CONFIG_BATT "homeassistant/sensor/pool_battery/config"

// ----- Sensor -----------------------------------------------
// DS18B20 data pin — use a GPIO that supports input on your board.
// Pin map verified for LOLIN32 Lite v1.0.0 (Wemos).
#ifdef ESP32
  #define DS18B20_DATA_PIN   4     // GPIO4
  #define DS18B20_POWER_PIN  16    // GPIO16 — powers the sensor only when awake
#else  // ESP8266
  #define DS18B20_DATA_PIN   D4    // GPIO2
  #define DS18B20_POWER_PIN  D3    // GPIO0
#endif

// ----- On-board user LED ------------------------------------
// LOLIN32 Lite has a user LED on GPIO22. We drive it LOW at boot and leave
// it that way to guarantee the LED stays dark during the active phase.
// (The 3V3 power LED is *not* GPIO-controlled — that one must be physically
// desoldered / trace-cut; see poolmeter.md.)
#ifdef ESP32
  #define USER_LED_PIN       22
#endif

// ----- Timing -----------------------------------------------
// Deep-sleep interval in microseconds (3 hours = 10,800,000,000 µs)
#define SLEEP_DURATION_US  (3ULL * 60ULL * 60ULL * 1000000ULL)

// Maximum seconds to wait for Wi-Fi before giving up and sleeping
#define WIFI_TIMEOUT_SEC   15

// Maximum MQTT connection retries
#define MQTT_MAX_RETRIES   3

// ----- Battery monitoring -----------------------------------
// ESP32 ADC pin connected to battery via voltage divider.
// Low-power divider: VBAT --- 1MΩ --- ADC_PIN --- 1MΩ --- GND
// + 100 nF ADC_PIN -> GND to stabilise the high-impedance node.
// Halves the voltage so 4.2 V reads as ~2.1 V (within ADC range).
#ifdef ESP32
  #define BATTERY_ADC_PIN    35    // GPIO35 (ADC1_CH7, input-only)
  #define ADC_RESOLUTION     4095.0
  #define ADC_REF_VOLTAGE    3.3
#else
  #define BATTERY_ADC_PIN    A0
  #define ADC_RESOLUTION     1023.0
  #define ADC_REF_VOLTAGE    1.0   // ESP8266 A0 max is 1V (with internal divider)
#endif

// Voltage divider ratio (R1 = R2 = 1MΩ → ratio = 2.0)
#define VDIVIDER_RATIO     2.0

// Empirical ADC correction factor — set after one-shot calibration against a
// multimeter (measure battery with DMM, read raw mV, compute ratio). Leave
// at 1.0 until calibrated.
#define ADC_CAL_FACTOR     1.0

// Battery thresholds (NCR18650B: 4.2V full, 2.5V cutoff)
#define BATT_FULL_V        4.20
#define BATT_EMPTY_V       3.00   // conservative cutoff for % calculation

// Hard low-voltage cutoff. Below this, enter hibernation and do NOT publish
// — protects the cell if the protected-PCM fails or degrades.
#define BATT_CUTOFF_V      3.20

// ----- Static IP (optional) ---------------------------------
// Skipping DHCP saves 2-4 s of WiFi association per cycle — major battery
// win. Comment out USE_STATIC_IP to fall back to DHCP.
#define USE_STATIC_IP
#define STATIC_IP          192, 168, 1, 50
#define STATIC_GATEWAY     192, 168, 1, 1
#define STATIC_SUBNET      255, 255, 255, 0
#define STATIC_DNS1        192, 168, 1, 1
#define STATIC_DNS2        1, 1, 1, 1
