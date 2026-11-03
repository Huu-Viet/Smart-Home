#include "device_controller.h"

#include <app_config.h>
#include <device_secrets.h>

namespace {
// Periodic sensor cadence only. Motion events are not gated by this:
// they are published on their PIR transition, immediately.
constexpr uint32_t kSensorIntervalMs = 60000;
// Retry a failed motion publish at most this often, so a Wi-Fi outage
// does not turn every loop iteration into a blocking TLS attempt.
constexpr uint32_t kMotionRetryIntervalMs = 5000;
}  // namespace

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

  handleMotionEvents(nowMs);

  // Sensor readings are independent of motion: they are sent on their own
  // cadence, and they carry no motion fields at all. A DHT failure blocks
  // only this path, never a motion event.
  if (!latestDhtReading_.valid || !wifiManager_.isConnected() ||
      !telemetryClient_.isConfigured() ||
      nowMs - lastSensorAtMs_ < kSensorIntervalMs) {
    return;
  }

  lastSensorAtMs_ = nowMs;
  if (telemetryClient_.publishSensorReading({latestDhtReading_.temperatureC,
                                             latestDhtReading_.humidityPercent, nowMs})) {
    Serial.println(F("Sensor reading sent to Supabase."));
  }
}

void DeviceController::handleMotionEvents(uint32_t nowMs) {
  const bool motionDetected = motionAlarm_.isMotionDetected();
  if (motionDetected != lastMotionDetected_) {
    lastMotionDetected_ = motionDetected;
    PendingMotionEvent& pending =
        motionDetected ? pendingStarted_ : pendingEnded_;
    pending.active = true;
    pending.motionStarted = motionDetected;
    pending.uptimeMs = nowMs;
    // Try immediately; publishPendingMotionEvents keeps it queued on failure.
    lastMotionRetryAtMs_ = 0;
  }

  publishPendingMotionEvents(nowMs);
}

void DeviceController::publishPendingMotionEvents(uint32_t nowMs) {
  if (!wifiManager_.isConnected() || !telemetryClient_.isConfigured()) {
    return;
  }
  if (!pendingStarted_.active && !pendingEnded_.active) {
    return;
  }
  if (nowMs - lastMotionRetryAtMs_ < kMotionRetryIntervalMs &&
      lastMotionRetryAtMs_ != 0) {
    return;
  }

  // Started before ended, so the event order in the database matches the
  // physical order even after a failed first attempt.
  PendingMotionEvent* pending =
      pendingStarted_.active ? &pendingStarted_ : &pendingEnded_;
  lastMotionRetryAtMs_ = nowMs;
  if (telemetryClient_.publishMotionEvent(
          {pending->motionStarted, pending->uptimeMs})) {
    Serial.println(
        pending->motionStarted ? F("Motion started event sent to Supabase.")
                               : F("Motion ended event sent to Supabase."));
    pending->active = false;
    // A queued second transition (e.g. ended right after started) must
    // not wait out the retry interval: only failures are throttled.
    lastMotionRetryAtMs_ = 0;
  }
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
