#pragma once

#include <Arduino.h>

// Periodic sensor measurement. Temperature and humidity stay together:
// they come from one DHT11 read at one instant, so they are one measurement,
// not two unrelated streams.
struct SensorReading {
  float temperatureC;
  float humidityPercent;
  uint32_t uptimeMs;
};

// Discrete motion event. It carries no temperature or humidity on purpose:
// a motion event must be sendable on its own, without waiting for (or
// repeating) the latest sensor reading. The buzzer is deliberately absent
// too: it is a local actuator, not a measurement, and its 3-second hold
// would otherwise blur the true motion duration.
struct MotionEvent {
  // True for a PIR rising edge, false for a PIR falling edge.
  bool motionStarted;
  uint32_t uptimeMs;
};

class SupabaseTelemetryClient {
 public:
  void configure(const char* endpoint, const char* deviceId, const char* deviceToken);
  bool isConfigured() const;
  bool publishSensorReading(const SensorReading& reading) const;
  bool publishMotionEvent(const MotionEvent& event) const;

 private:
  bool postJson(const String& body) const;

  const char* endpoint_ = nullptr;
  const char* deviceId_ = nullptr;
  const char* deviceToken_ = nullptr;
};
