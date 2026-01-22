#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_AHTX0.h>
#include "ScioSense_ENS160.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>



#define WLAN_SSID     {WLAN-SSID}
#define WLAN_PASS     {WLAN-PW}
#define MQTT_BROKER   {BROKER-ID}
#define MQTT_PORT     1883
#define MQTT_TOPIC    "sensor/co2"

WiFiClient espClient;
PubSubClient mqtt(espClient);

// Display-Größe
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define I2C_ADDRESS 0x52
Adafruit_AHTX0 aht;

ScioSense_ENS160 ens160(ENS160_I2CADDR_1);

void connectWiFi() {
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) delay(250);
}

void connectMQTT() {
  while (!mqtt.connected()) mqtt.connect("esp32-CO2");
}

void setup() {
  Serial.begin(115200);
  connectWiFi();
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);

  // OLED initialisieren
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED nicht gefunden"));
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  Serial.println("------------------------------------------------------------");
  Serial.println("ENS160 - Digital air quality sensor");
  Serial.println("Sensor readout with rH & T compensation");
  Serial.println("------------------------------------------------------------");

  delay(1000);

  Serial.print("ENS160...");
  ens160.begin();
  Serial.println(ens160.available() ? "done." : "failed!");
  if (ens160.available()) {
    Serial.print("\tRev: "); Serial.print(ens160.getMajorRev());
    Serial.print("."); Serial.print(ens160.getMinorRev());
    Serial.print("."); Serial.println(ens160.getBuild());

    Serial.print("\tStandard mode ");
    Serial.println(ens160.setMode(ENS160_OPMODE_STD) ? "done." : "failed!");
  }

  Serial.print("AHT21...");
  if (!aht.begin()) {
    Serial.println("AHT21 nicht gefunden!");
    while (1);
  }
}

void loop() {

  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);

  Serial.print("Temp: "); Serial.print(temp.temperature); Serial.print(" °C, ");
  Serial.print("Humidity: "); Serial.print(humidity.relative_humidity); Serial.println(" %");

  if (ens160.available()) {
    ens160.set_envdata(temp.temperature, humidity.relative_humidity);
    ens160.measure(true);
    ens160.measureRaw(true);
  }

  uint8_t aqi = ens160.getAQI();
  uint16_t tvoc = ens160.getTVOC();
  uint16_t eco2 = ens160.geteCO2();
  // Konsolen-Ausgabe
  Serial.print("AQI: "); Serial.print(aqi); Serial.print("\t");
  Serial.print("TVOC: "); Serial.print(tvoc); Serial.print("ppb\t");
  Serial.print("eCO2: "); Serial.print(eco2); Serial.print("ppm\t");
  Serial.println();
  Serial.println();

  // OLED-Ausgabe
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.printf("T: %.1fC  H: %.0f%%\n", temp.temperature, humidity.relative_humidity);
  display.printf("AQI:%d TVOC:%d\n", aqi, tvoc);
  display.setTextSize(2);
  display.setCursor(0, 18);
  display.printf("eCO2:%dppm", eco2);
  display.display();

  // Daten-JSON erstellen
  String payload = "{\"temp\":" + String(temp.temperature,1) +
  ",\"humidity\":"  + String(humidity.relative_humidity,0) +
  ",\"aqi\":"  + String(aqi) + // AQI-Wert Air Quality Index (1-5)
  ",\"tvoc\":" + String(tvoc) + // TVOC-Wert Total Volatile Organic Compounds (ppb)
  ",\"eco2\":" + String(eco2) + "}";

  mqtt.publish(MQTT_TOPIC, payload.c_str());

  delay(2000);
}

