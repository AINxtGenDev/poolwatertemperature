// ============================================================
//  Pool Temperature Monitor — ESP32 / ESP8266 Firmware
// ============================================================
//  Reads water temperature from a DS18B20 probe, publishes to
//  Home Assistant via MQTT, then enters deep sleep for 3 hours.
//
//  Power-optimized: sensor is powered from a GPIO pin so it
//  draws zero current during deep sleep.
// ============================================================

#include <Arduino.h>

#ifdef ESP32
  #include <WiFi.h>
  #include <esp_wifi.h>
  #include <esp_sleep.h>
  #include "soc/soc.h"
  #include "soc/rtc_cntl_reg.h"
#else
  #include <ESP8266WiFi.h>
#endif

#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"

// ------------------------------------------------------------
//  Globals
// ------------------------------------------------------------
WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);
OneWire      oneWire(DS18B20_DATA_PIN);
DallasTemperature sensors(&oneWire);

// Store boot count in RTC memory (survives deep sleep)
#ifdef ESP32
  RTC_DATA_ATTR int bootCount = 0;
#endif

// ------------------------------------------------------------
//  Forward declarations
// ------------------------------------------------------------
bool     connectWiFi();
bool     connectMQTT();
float    readTemperature();
float    readBatteryVoltage();
uint8_t  batteryPercent(float voltage);
void     publishDiscoveryConfig();
void     publishData(float tempC, float battV, uint8_t battPct);
void     enterDeepSleep();
void     enterHibernation();

// ------------------------------------------------------------
//  setup() — runs once per wake cycle
// ------------------------------------------------------------
void setup() {
#ifdef ESP32
  // Disable brown-out detector. The LDO + 10 µF bulk cap can momentarily
  // droop below the BOD threshold during the first WiFi TX burst, causing
  // an unwanted reset. Safe because the protected 18650 PCM handles
  // under-voltage at the cell level.
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // Force user LED (GPIO22 on LOLIN32 Lite) dark during the active phase.
  // Try LOW first (active-high LED); if the LED stays lit in your batch,
  // flip to HIGH (active-low wiring).
  pinMode(USER_LED_PIN, OUTPUT);
  digitalWrite(USER_LED_PIN, LOW);
#endif

  Serial.begin(115200);
  delay(10);

#ifdef ESP32
  bootCount++;
  Serial.printf("\n== Pool Temp Monitor — Boot #%d ==\n", bootCount);
#else
  Serial.println("\n== Pool Temp Monitor ==");
#endif

  // 1. Power on the DS18B20 sensor via GPIO
  pinMode(DS18B20_POWER_PIN, OUTPUT);
  digitalWrite(DS18B20_POWER_PIN, HIGH);
  delay(50);  // let sensor stabilize

  // 2. Start temperature conversion (takes ~750ms at 12-bit)
  sensors.begin();
  sensors.setResolution(12);
  sensors.setWaitForConversion(false);  // non-blocking
  sensors.requestTemperatures();

  // 3. Read battery voltage FIRST — before any WiFi activity so the
  //    reading is not polluted by 200 mA TX-burst sag.
  float battV   = readBatteryVoltage();
  uint8_t battPct = batteryPercent(battV);
  Serial.printf("Battery:     %.2f V (%d%%)\n", battV, battPct);

  // 4. Hard cutoff — if cell is critically low, skip the cycle and
  //    hibernate. Hibernation draws less than timer-wake deep sleep.
  if (battV > 0.5 && battV < BATT_CUTOFF_V) {
    Serial.printf("Battery below cutoff (%.2f V) — hibernating.\n",
                  (double)BATT_CUTOFF_V);
    digitalWrite(DS18B20_POWER_PIN, LOW);
    enterHibernation();
  }

  // 5. While sensor converts, connect to WiFi (parallel work)
  bool wifiOk = connectWiFi();

  // 6. Read temperature (conversion should be done by now)
  delay(750);  // ensure 12-bit conversion completes
  float tempC = readTemperature();

  Serial.printf("Temperature: %.2f °C\n", tempC);

  // 7. Publish via MQTT if WiFi connected
  if (wifiOk && tempC > -100.0) {
    if (connectMQTT()) {
      publishDiscoveryConfig();
      publishData(tempC, battV, battPct);
      // Allow MQTT packets to be sent
      mqtt.loop();
      delay(100);
      mqtt.disconnect();
    }
  } else {
    Serial.println("Skipping MQTT — WiFi or sensor failure");
  }

  // 8. Power off sensor and WiFi, then deep sleep
  digitalWrite(DS18B20_POWER_PIN, LOW);
  WiFi.disconnect(true);
#ifdef ESP32
  esp_wifi_stop();
#else
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
#endif

  enterDeepSleep();
}

// Not used — everything happens in setup()
void loop() {}

// ------------------------------------------------------------
//  Wi-Fi connection with timeout
// ------------------------------------------------------------
bool connectWiFi() {
  Serial.printf("Connecting to WiFi '%s'", WIFI_SSID);
  WiFi.mode(WIFI_STA);

#ifdef ESP32
  // Disable WiFi power saving for faster connection
  WiFi.setSleep(false);
#endif

#ifdef USE_STATIC_IP
  IPAddress ip(STATIC_IP);
  IPAddress gw(STATIC_GATEWAY);
  IPAddress sn(STATIC_SUBNET);
  IPAddress d1(STATIC_DNS1);
  IPAddress d2(STATIC_DNS2);
  if (!WiFi.config(ip, gw, sn, d1, d2)) {
    Serial.println(" (static IP config failed, falling back to DHCP)");
  }
#endif

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > WIFI_TIMEOUT_SEC * 1000UL) {
      Serial.println(" TIMEOUT");
      return false;
    }
    delay(100);
    Serial.print(".");
  }

  Serial.printf(" OK (%s)\n", WiFi.localIP().toString().c_str());
  return true;
}

// ------------------------------------------------------------
//  MQTT connection with retries
// ------------------------------------------------------------
bool connectMQTT() {
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setBufferSize(512);

  for (int attempt = 1; attempt <= MQTT_MAX_RETRIES; attempt++) {
    Serial.printf("MQTT connect attempt %d/%d...", attempt, MQTT_MAX_RETRIES);

    bool connected;
    if (strlen(MQTT_USER) > 0) {
      connected = mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD);
    } else {
      connected = mqtt.connect(MQTT_CLIENT_ID);
    }

    if (connected) {
      Serial.println(" OK");
      return true;
    }

    Serial.printf(" FAILED (rc=%d)\n", mqtt.state());
    delay(500);
  }
  return false;
}

// ------------------------------------------------------------
//  Read temperature from DS18B20
// ------------------------------------------------------------
float readTemperature() {
  float tempC = sensors.getTempCByIndex(0);

  if (tempC == DEVICE_DISCONNECTED_C || tempC == -127.0) {
    Serial.println("ERROR: DS18B20 not found or read error");
    return -127.0;
  }
  return tempC;
}

// ------------------------------------------------------------
//  Read battery voltage via ADC + voltage divider
// ------------------------------------------------------------
float readBatteryVoltage() {
#ifdef ESP32
  // High-impedance 1MΩ divider needs a settling read before sampling.
  analogRead(BATTERY_ADC_PIN);
  delay(10);

  uint32_t adcSum = 0;
  const int samples = 32;
  for (int i = 0; i < samples; i++) {
    adcSum += analogRead(BATTERY_ADC_PIN);
    delay(2);
  }
  float adcAvg = (float)adcSum / samples;
  float voltage = (adcAvg / ADC_RESOLUTION) * ADC_REF_VOLTAGE
                  * VDIVIDER_RATIO * ADC_CAL_FACTOR;
#else
  float adcVal = analogRead(BATTERY_ADC_PIN);
  float voltage = (adcVal / ADC_RESOLUTION) * ADC_REF_VOLTAGE
                  * VDIVIDER_RATIO * ADC_CAL_FACTOR;
#endif

  return voltage;
}

// ------------------------------------------------------------
//  Map battery voltage to percentage (linear approximation)
// ------------------------------------------------------------
uint8_t batteryPercent(float voltage) {
  if (voltage >= BATT_FULL_V) return 100;
  if (voltage <= BATT_EMPTY_V) return 0;
  return (uint8_t)(((voltage - BATT_EMPTY_V) / (BATT_FULL_V - BATT_EMPTY_V)) * 100.0);
}

// ------------------------------------------------------------
//  Publish HA MQTT auto-discovery config (retained)
//  This lets Home Assistant auto-create the sensor entities.
// ------------------------------------------------------------
void publishDiscoveryConfig() {
  // Temperature sensor discovery
  const char* tempConfig =
    "{"
      "\"name\":\"Pool Temperature\","
      "\"unique_id\":\"pool_temp_sensor\","
      "\"state_topic\":\"" MQTT_TOPIC_TEMP "\","
      "\"unit_of_measurement\":\"\\u00b0C\","
      "\"device_class\":\"temperature\","
      "\"value_template\":\"{{ value_json.temperature }}\","
      "\"device\":{"
        "\"identifiers\":[\"pool_temp_monitor\"],"
        "\"name\":\"Pool Monitor\","
        "\"model\":\"ESP32 + DS18B20\","
        "\"manufacturer\":\"DIY\""
      "}"
    "}";
  mqtt.publish(MQTT_TOPIC_CONFIG_TEMP, tempConfig, true);

  // Battery sensor discovery
  const char* battConfig =
    "{"
      "\"name\":\"Pool Monitor Battery\","
      "\"unique_id\":\"pool_temp_battery\","
      "\"state_topic\":\"" MQTT_TOPIC_BATT "\","
      "\"unit_of_measurement\":\"%\","
      "\"device_class\":\"battery\","
      "\"value_template\":\"{{ value_json.battery }}\","
      "\"device\":{"
        "\"identifiers\":[\"pool_temp_monitor\"],"
        "\"name\":\"Pool Monitor\","
        "\"model\":\"ESP32 + DS18B20\","
        "\"manufacturer\":\"DIY\""
      "}"
    "}";
  mqtt.publish(MQTT_TOPIC_CONFIG_BATT, battConfig, true);
}

// ------------------------------------------------------------
//  Publish sensor data
// ------------------------------------------------------------
void publishData(float tempC, float battV, uint8_t battPct) {
  char payload[128];

  // Temperature
  snprintf(payload, sizeof(payload),
           "{\"temperature\":%.2f}", tempC);
  mqtt.publish(MQTT_TOPIC_TEMP, payload, true);
  Serial.printf("Published: %s -> %s\n", MQTT_TOPIC_TEMP, payload);

  // Battery
  snprintf(payload, sizeof(payload),
           "{\"battery\":%d,\"voltage\":%.2f}", battPct, battV);
  mqtt.publish(MQTT_TOPIC_BATT, payload, true);
  Serial.printf("Published: %s -> %s\n", MQTT_TOPIC_BATT, payload);
}

// ------------------------------------------------------------
//  Enter deep sleep
// ------------------------------------------------------------
void enterDeepSleep() {
  Serial.printf("Sleeping for %llu seconds...\n",
                SLEEP_DURATION_US / 1000000ULL);
  Serial.flush();

#ifdef ESP32
  esp_sleep_enable_timer_wakeup(SLEEP_DURATION_US);
  esp_deep_sleep_start();
#else
  // ESP8266: connect GPIO16 (D0) to RST for deep-sleep wake
  ESP.deepSleep(SLEEP_DURATION_US);
#endif
}

// ------------------------------------------------------------
//  Enter hibernation (lowest-power state)
//  Used when the cell is below the hard cutoff. No timer wake —
//  only a physical RESET will bring the device back. This lets the
//  cell sit idle for months without further discharge from the MCU.
// ------------------------------------------------------------
void enterHibernation() {
  Serial.println("Hibernating until reset.");
  Serial.flush();

#ifdef ESP32
  // Power down every RTC domain — reduces deep-sleep current to ~5 µA on
  // a well-designed board.
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH,    ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM,  ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM,  ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL,          ESP_PD_OPTION_OFF);
  // No wakeup source → sleeps until external RESET.
  esp_deep_sleep_start();
#else
  ESP.deepSleep(0);  // 0 = sleep until reset
#endif
}
