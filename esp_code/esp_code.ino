#include <WiFi.h>
#include <WiFiUdp.h>
#include "wifi_provisioning.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

// ==================== CONFIG ====================
const int CANDLE1_PIN = 4;
const int CANDLE2_PIN = 15;
const int RESET_WIFI_BTN = 27;
const int STATUS_LED = 2;

const unsigned int UDP_PORT = 4210;
WiFiUDP udp;
char udpBuffer[64];

unsigned long lastBroadcast = 0;

// ==================== STATE ====================
bool candlesOn = false;
String deviceId;

// ==================== HELPERS ====================
String buildDeviceId() {
  uint64_t mac = ESP.getEfuseMac();
  char buf[13];
  snprintf(buf, sizeof(buf), "%012llx", mac);
  return "esp32-" + String(buf);
}

void setCandles(bool on) {
  candlesOn = on;
  digitalWrite(CANDLE1_PIN, on ? HIGH : LOW);
  digitalWrite(CANDLE2_PIN, on ? HIGH : LOW);

  Serial.print("Candles -> ");
  Serial.println(on ? "ON" : "OFF");
}

// ==================== UDP ====================
void sendStatusBroadcast() {
  String payload = deviceId + "," + (candlesOn ? "ON" : "OFF");
  udp.beginPacket("255.255.255.255", UDP_PORT);
  udp.write((uint8_t*)payload.c_str(), payload.length());
  udp.endPacket();
}

void handleUDP() {
  int packetSize = udp.parsePacket();
  if (!packetSize) return;

  int len = udp.read(udpBuffer, sizeof(udpBuffer) - 1);
  if (len <= 0) return;
  udpBuffer[len] = 0;

  String msg = String(udpBuffer);
  msg.trim();

  int comma = msg.indexOf(',');
  if (comma <= 0) return;

  String id  = msg.substring(0, comma);
  String cmd = msg.substring(comma + 1);

  if (id != deviceId) return;

  cmd.toUpperCase();
  if (cmd == "ON")  setCandles(true);
  if (cmd == "OFF") setCandles(false);
}

// ==================== WIFI RESET ====================
void checkResetWiFiButton() {
  static unsigned long pressStart = 0;
  static bool pressed = false;

  if (digitalRead(RESET_WIFI_BTN) == LOW) {
    if (!pressed) {
      pressed = true;
      pressStart = millis();
    }
    if (millis() - pressStart > 3000) {
      Serial.println("Erasing Wi-Fi credentials...");
      WiFi.disconnect(true);
      esp_wifi_disconnect();
      nvs_flash_erase();
      nvs_flash_init();
      delay(100);
      ESP.restart();
    }
  } else {
    pressed = false;
  }
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);

  pinMode(CANDLE1_PIN, OUTPUT);
  pinMode(CANDLE2_PIN, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  pinMode(RESET_WIFI_BTN, INPUT_PULLUP);

  digitalWrite(CANDLE1_PIN, LOW);
  digitalWrite(CANDLE2_PIN, LOW);
  digitalWrite(STATUS_LED, LOW);

  initWiFi();
  waitForWiFi();

  digitalWrite(STATUS_LED, HIGH);

  deviceId = buildDeviceId();
  Serial.print("Device ID: ");
  Serial.println(deviceId);

  udp.begin(UDP_PORT);
  Serial.println("UDP ready");
}

// ==================== LOOP ====================
void loop() {
  checkResetWiFiButton();
  handleUDP();

  if (millis() - lastBroadcast > 1000) {
    lastBroadcast = millis();
    sendStatusBroadcast();
  }

  delay(10);
}
