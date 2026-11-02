#include "device_controller.h"

#include <app_config.h>
#include <device_secrets.h>

namespace {
constexpr uint32_t kTelemetryIntervalMs = 60000;
}

DeviceController::DeviceController()
    : dht11_(AppConfig::kDhtDataPin),
      motionAlarm_(AppConfig::kPirPin, AppConfig::kBuzzerPin,
                   AppConfig::kBuzzerActiveHigh, AppConfig::kPirWarmupMs,
                   AppConfig::kBuzzerHoldMs) {}

void DeviceController::begin() {
  Serial.println();
  Serial.println(F("Smart Home local hardware demo starting."));

  dht11_.begin();
  motionAlarm_.begin(millis());

#if DEVICE_LOCAL_CONFIG_AVAILABLE
  wifiManager_.begin(LocalConfig::kWifiSsid, LocalConfig::kWifiPassword);
  telemetryClient_.configure(LocalConfig::kSupabaseIngestUrl, LocalConfig::kDeviceId,
                             LocalConfig::kDeviceToken);
#else
  Serial.println(F("Create config/local_config.h to enable Wi-Fi and Supabase."));
#endif

  Serial.println(F("DHT11 ready: readings print every 2 seconds."));
  Serial.println(F("PIR warming up for 60 seconds; buzzer is off."));
}

void DeviceController::update(uint32_t nowMs) {
  motionAlarm_.update(nowMs);
  wifiManager_.update(nowMs);

  if (nowMs - lastDhtReadAtMs_ >= AppConfig::kDhtReadIntervalMs) {
    lastDhtReadAtMs_ = nowMs;
    printDhtReading();
  }

  if (!latestDhtReading_.valid || !wifiManager_.isConnected() ||
      !telemetryClient_.isConfigured() ||
      nowMs - lastTelemetryAtMs_ < kTelemetryIntervalMs) {
    return;
  }

  lastTelemetryAtMs_ = nowMs;
  telemetryClient_.publish({latestDhtReading_.temperatureC,
                            latestDhtReading_.humidityPercent,
                            motionAlarm_.isMotionDetected(),
                            motionAlarm_.isAlarmActive(), nowMs});
}

void DeviceController::printDhtReading() {
  latestDhtReading_ = dht11_.read();
  if (!latestDhtReading_.valid) {
    Serial.println(F("DHT11 read failed. Check power, data pin, and pull-up."));
    return;
  }

  Serial.print(F("Temperature: "));
  Serial.print(latestDhtReading_.temperatureC, 1);
  Serial.print(F(" C | Humidity: "));
  Serial.print(latestDhtReading_.humidityPercent, 1);
  Serial.println(F(" %"));
}
