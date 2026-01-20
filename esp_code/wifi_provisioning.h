#ifndef WIFI_PROVISIONING_H
#define WIFI_PROVISIONING_H

#include <WiFi.h>
#include "WiFiProv.h"

// ----- Config -----
const char * POP = "abcd1234";
const char * SERVICE_NAME = "PROV_123";
const char * SERVICE_KEY = NULL;

// Event handler for provisioning
void SysProvEvent(arduino_event_t *sys_event) {
  switch (sys_event->event_id) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("\nConnected IP: ");
      Serial.println(IPAddress(sys_event->event_info.got_ip.ip_info.ip.addr));
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("\nWi-Fi disconnected. Reconnecting...");
      break;
    case ARDUINO_EVENT_PROV_START:
      Serial.println("\nProvisioning started.");
      break;
    case ARDUINO_EVENT_PROV_CRED_RECV:
      Serial.println("Received Wi-Fi credentials.");
      break;
    case ARDUINO_EVENT_PROV_CRED_FAIL:
      Serial.println("Provisioning failed.");
      break;
    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
      Serial.println("Provisioning successful!");
      break;
    case ARDUINO_EVENT_PROV_END:
      Serial.println("Provisioning ended.");
      break;
    default: break;
  }
}

// Initialize Wi-Fi (BLE provisioning if needed)
void initWiFi() {
  WiFi.onEvent(SysProvEvent);

  if (String(WiFi.SSID()) == "") {
    Serial.println("No Wi-Fi credentials found. Starting BLE provisioning...");

    uint8_t uuid[16] = {0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
                        0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02};

    WiFiProv.beginProvision(
      NETWORK_PROV_SCHEME_BLE, NETWORK_PROV_SCHEME_HANDLER_FREE_BLE,
      NETWORK_PROV_SECURITY_1, POP, SERVICE_NAME, SERVICE_KEY, uuid, false
    );
    WiFiProv.printQR(SERVICE_NAME, POP, "ble");
  } else {
    Serial.println("Wi-Fi credentials found. Connecting...");
    WiFi.begin();
  }
}

// Wait until Wi-Fi is connected
void waitForWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }
  Serial.println("Wi-Fi connected!");
}

#endif