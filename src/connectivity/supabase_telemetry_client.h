#pragma once

#include <Arduino.h>

struct TelemetryPayload {
  float temperatureC;
  float humidityPercent;
  bool motionDetected;
  bool buzzerActive;
  uint32_t uptimeMs;
};

class SupabaseTelemetryClient {
 public:
  void configure(const char* endpoint, const char* deviceId, const char* deviceToken);
  bool isConfigured() const;
  bool publish(const TelemetryPayload& payload) const;

 private:
  const char* endpoint_ = nullptr;
  const char* deviceId_ = nullptr;
  const char* deviceToken_ = nullptr;
};
