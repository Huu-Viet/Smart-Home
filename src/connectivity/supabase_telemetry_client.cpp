#include "supabase_telemetry_client.h"

#include <HTTPClient.h>
#include <WiFiClientSecure.h>

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

bool SupabaseTelemetryClient::publish(const TelemetryPayload& payload) const {
  if (!isConfigured()) {
    return false;
  }

  char body[256];
  const int written = snprintf(
      body, sizeof(body),
      "{\"temperatureC\":%.2f,\"humidityPercent\":%.2f,\"motionDetected\":%s,"
      "\"buzzerActive\":%s,\"uptimeMs\":%lu,\"firmwareVersion\":\"0.1.0\"}",
      payload.temperatureC, payload.humidityPercent,
      payload.motionDetected ? "true" : "false",
      payload.buzzerActive ? "true" : "false",
      static_cast<unsigned long>(payload.uptimeMs));

  if (written <= 0 || written >= static_cast<int>(sizeof(body))) {
    Serial.println(F("Telemetry payload could not be serialized."));
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

  const int statusCode = http.POST(reinterpret_cast<const uint8_t*>(body), strlen(body));
  http.end();

  if (statusCode < 200 || statusCode >= 300) {
    Serial.print(F("Supabase telemetry rejected. HTTP status: "));
    Serial.println(statusCode);
    return false;
  }

  Serial.println(F("Telemetry sent to Supabase."));
  return true;
}
