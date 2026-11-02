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

  Dht11Sensor dht11_;
  MotionAlarm motionAlarm_;
  WifiManager wifiManager_;
  SupabaseTelemetryClient telemetryClient_;
  DhtReading latestDhtReading_{0.0F, 0.0F, false};
  uint32_t lastDhtReadAtMs_ = 0;
  uint32_t lastTelemetryAtMs_ = 0;
};
