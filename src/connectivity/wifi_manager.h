#pragma once

#include <Arduino.h>

class WifiManager {
 public:
  void begin(const char* ssid, const char* password);
  void update(uint32_t nowMs);
  bool isConnected() const;

 private:
  void connect(uint32_t nowMs);

  const char* ssid_ = nullptr;
  const char* password_ = nullptr;
  uint32_t lastAttemptAtMs_ = 0;
  bool configured_ = false;
  bool connectionPreviouslyReported_ = false;
};
