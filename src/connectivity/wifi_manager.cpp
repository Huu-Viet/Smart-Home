#include "wifi_manager.h"

#include <WiFi.h>

namespace {
constexpr uint32_t kReconnectIntervalMs = 10000;
}

void WifiManager::begin(const char* ssid, const char* password) {
  ssid_ = ssid;
  password_ = password;
  configured_ = ssid_ != nullptr && password_ != nullptr && ssid_[0] != '\0';

  if (!configured_) {
    Serial.println(F("Wi-Fi is disabled: local configuration is missing."));
    return;
  }

  WiFi.mode(WIFI_STA);
  connect(millis());
}

void WifiManager::update(uint32_t nowMs) {
  if (!configured_) {
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (!connectionPreviouslyReported_) {
      Serial.print(F("Wi-Fi connected. IP: "));
      Serial.println(WiFi.localIP());
      connectionPreviouslyReported_ = true;
    }
    return;
  }

  connectionPreviouslyReported_ = false;
  if (nowMs - lastAttemptAtMs_ >= kReconnectIntervalMs) {
    connect(nowMs);
  }
}

bool WifiManager::isConnected() const {
  return configured_ && WiFi.status() == WL_CONNECTED;
}

void WifiManager::connect(uint32_t nowMs) {
  lastAttemptAtMs_ = nowMs;
  Serial.print(F("Connecting to Wi-Fi: "));
  Serial.println(ssid_);
  WiFi.begin(ssid_, password_);
}
