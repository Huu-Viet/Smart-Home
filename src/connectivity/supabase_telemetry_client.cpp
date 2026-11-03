#include "supabase_telemetry_client.h"

#include <HTTPClient.h>
#include <WiFiClientSecure.h>

namespace {
// Firmware 0.2.0 is the first build with split payloads: sensor readings
// and motion events are separate kinds, written to separate tables.
// Bumping the version lets statistics distinguish the old bundled rows
// in public.telemetry from the new split rows.
constexpr char kFirmwareVersion[] = "0.2.0";
}  // namespace

void SupabaseTelemetryClient::configure(const char* endpoint, const char* deviceId,
                                        const char* deviceToken) {
  endpoint_ = endpoint;
  deviceId_ = deviceId;
  deviceToken_ = deviceToken;
}

bool SupabaseTelemetryClient::isConfigured() const {
  return endpoint_ != nullptr && deviceId_ != nullptr && deviceToken_ != nullptr &&
         endpoint_[0] != '\0' && deviceId_[0] != '\0' && deviceToken_[0] != '\0';
}

bool SupabaseTelemetryClient::publishSensorReading(const SensorReading& reading) const {
  char body[256];
  const int written = snprintf(
      body, sizeof(body),
      "{\"kind\":\"sensor\",\"temperatureC\":%.2f,\"humidityPercent\":%.2f,"
      "\"uptimeMs\":%lu,\"firmwareVersion\":\"%s\"}",
      reading.temperatureC, reading.humidityPercent,
      static_cast<unsigned long>(reading.uptimeMs), kFirmwareVersion);

  if (written <= 0 || written >= static_cast<int>(sizeof(body))) {
    Serial.println(F("Sensor payload could not be serialized."));
    return false;
  }

  return postJson(String(body));
}

bool SupabaseTelemetryClient::publishMotionEvent(const MotionEvent& event) const {
  char body[256];
  const int written = snprintf(
      body, sizeof(body),
      "{\"kind\":\"motion\",\"eventType\":\"%s\",\"uptimeMs\":%lu,"
      "\"firmwareVersion\":\"%s\"}",
      event.motionStarted ? "motion_started" : "motion_ended",
      static_cast<unsigned long>(event.uptimeMs), kFirmwareVersion);

  if (written <= 0 || written >= static_cast<int>(sizeof(body))) {
    Serial.println(F("Motion payload could not be serialized."));
    return false;
  }

  return postJson(String(body));
}

bool SupabaseTelemetryClient::postJson(const String& body) const {
  if (!isConfigured()) {
    return false;
  }

  WiFiClientSecure tlsClient;
  tlsClient.useBuiltinCACertBundle();

  HTTPClient http;
  if (!http.begin(tlsClient, endpoint_)) {
    Serial.println(F("Supabase HTTPS connection could not start."));
    return false;
  }

  http.addHeader(F("Content-Type"), F("application/json"));
  http.addHeader(F("X-Device-Id"), deviceId_);
  http.addHeader(F("X-Device-Token"), deviceToken_);

  // Arduino core 3.x: POST(uint8_t*, size_t) takes a non-const buffer, so
  // use the POST(String) overload instead of casting away constness.
  const int statusCode = http.POST(body);
  http.end();

  if (statusCode < 200 || statusCode >= 300) {
    Serial.print(F("Supabase telemetry rejected. HTTP status: "));
    Serial.println(statusCode);
    return false;
  }

  return true;
}
