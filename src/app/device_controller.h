#pragma once

#include <Arduino.h>

#include "../hardware/dht11_sensor.h"
#include "../hardware/motion_alarm.h"
#include "../connectivity/supabase_telemetry_client.h"
#include "../connectivity/wifi_manager.h"

class DeviceController {
 public:
  DeviceController();

  void begin();
  void update(uint32_t nowMs);

 private:
  void printDhtReading();
  void handleMotionEvents(uint32_t nowMs);
  void publishPendingMotionEvents(uint32_t nowMs);

  // One pending event per transition direction. A started event must not
  // be overwritten by the ended event that follows it if the first publish
  // failed while Wi-Fi was down: both transitions are real history.
  struct PendingMotionEvent {
    bool active = false;
    bool motionStarted = false;
    uint32_t uptimeMs = 0;
  };

  Dht11Sensor dht11_;
  MotionAlarm motionAlarm_;
  WifiManager wifiManager_;
  SupabaseTelemetryClient telemetryClient_;
  DhtReading latestDhtReading_{0.0F, 0.0F, false};
  uint32_t lastDhtReadAtMs_ = 0;
  uint32_t lastSensorAtMs_ = 0;
  bool lastMotionDetected_ = false;
  PendingMotionEvent pendingStarted_;
  PendingMotionEvent pendingEnded_;
  uint32_t lastMotionRetryAtMs_ = 0;
};
