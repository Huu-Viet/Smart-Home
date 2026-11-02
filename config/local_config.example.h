#pragma once

// Copy this file to config/local_config.h. The real file is ignored by Git.
// Keep the device token private. Do not place Supabase service_role keys here.

namespace LocalConfig {
constexpr char kWifiSsid[] = "YOUR_WIFI_SSID";
constexpr char kWifiPassword[] = "YOUR_WIFI_PASSWORD";

constexpr char kSupabaseIngestUrl[] =
    "https://vdkykmyjfrzaluycycla.supabase.co/functions/v1/ingest-device";
constexpr char kDeviceId[] = "desk-esp32-01";
constexpr char kDeviceToken[] = "PASTE_A_PRIVATE_DEVICE_TOKEN";
}  // namespace LocalConfig
