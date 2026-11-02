# Architecture brief — Smart Home ESP32 baseline

## 1. Product and hardware

- **Purpose / user outcome:** Baseline project for learning and evolving an ESP32 smart-home device.
- **Board / chip / revision / flash:** `esp32doit-devkit-v1`; exact module revision and flash capacity must be verified from the physical board before feature implementation.
- **Build baseline:** PlatformIO `espressif32 @ 8.0.0` with Arduino framework. Update only through the release checklist and a rebuild/test.
- **Power budget and failure behavior:** DHT11, PIR, and active buzzer are powered locally. The buzzer starts off and is forced off during the PIR warm-up period. Verify the buzzer's current requirement; use a transistor driver and separate supply if it exceeds the ESP32 GPIO rating.
- **Capabilities:** DHT11 temperature/humidity telemetry to Serial Monitor and Supabase; PIR local motion detection; active buzzer local alarm. Telemetry is sent through a Supabase Edge Function, not directly to the database.
- **Pin and electrical map:**

| Function | GPIO | Electrical constraints | Boot / safe state | Verified on hardware |
| --- | --- | --- | --- | --- |
| DHT11 data | 4 | 3.3 V logic; provide data pull-up if the module does not include one | input | pending |
| PIR output | 27 | Digital output from PIR module | input, alarm disabled during 60 s warm-up | pending |
| Active buzzer | 26 | Confirm active-high logic and GPIO current draw | LOW/off | pending |

## 2. Layering and ownership

```text
Hardware drivers -> Domain/device state -> Connectivity/services -> App orchestration
```

`src/main.cpp` is the composition root. `src/hardware/dht11_sensor.*` reads DHT11, `src/hardware/motion_alarm.*` owns PIR/buzzer I/O, and `src/app/device_controller.*` schedules both and prints valid DHT11 readings. New functionality must be introduced in the corresponding layer rather than accumulating GPIO, Wi-Fi, cloud protocol, and policy logic in `setup()` or `loop()`.

When implementation begins, use the skill's default modular structure: `src/app`, optional `src/domain`, `src/hardware`, `src/connectivity`, `src/config`, and `src/utils`. Keep a module's header alongside its implementation; reserve `include/app_config.h` for non-secret build-time configuration, and keep local credentials in the ignored `config/local_config.h`.

## 3. Lifecycle and recovery

Target lifecycle:

```text
boot -> self-check -> unprovisioned/provisioning -> connecting -> online/degraded -> OTA/recovery
```

This iteration is `boot -> self-check -> local monitoring -> Wi-Fi connecting -> telemetry online/degraded`. PIR triggers are ignored during a 60-second warm-up; afterward a detected motion keeps the buzzer on until no motion has been observed for three seconds. Wi-Fi reconnects every 10 seconds; telemetry is attempted every 60 seconds once a valid DHT reading is available.

## 4. Connectivity and cloud contract

- **Primary connection:** Wi-Fi station.
- **Provisioning:** BLE with device-specific authentication/Proof of Possession is preferred when mobile onboarding is required. BLE must be stopped after provisioning. A protected, time-limited SoftAP fallback may be introduced later.
- **Cloud transport adapter:** HTTPS to the `ingest-device` Supabase Edge Function. The ESP32 does not access Postgres directly.
- **Device identity:** A per-device ID and random token; only the token SHA-256 hash is stored in `public.devices`.

Message contracts will contain device identity, timestamp, firmware version, telemetry, commands, desired state, and reported state without embedding provider-specific concepts in domain logic.

## 5. Storage, OTA, and security

- **Partition/memory budget:** Must be redesigned once actual flash capacity and firmware size are known; reserve dual OTA slots for any connected release.
- **OTA:** Production requires HTTPS, signed images, manifest/version validation, health confirmation, and rollback.
- **Secrets:** Keep values in ignored local configuration or an approved CI secret store; no secret currently exists in this repository.
- **Threat model:** The first feature design must address physical access, rogue Wi-Fi, credential leak, firmware tampering, cloud compromise, and power/network failure.
- **Production assessment:** Secure Boot, Flash Encryption, and NVS Encryption are not enabled or evaluated. Do not burn eFuses or make irreversible security changes without an approved manufacturing/recovery procedure.

## 6. Decisions and open risks

| Decision or risk | Chosen approach / mitigation | Review trigger |
| --- | --- | --- |
| Framework | PlatformIO + Arduino; retain an ESP-IDF migration path for hardening | First production-security requirement |
| Cloud vendor | Keep domain contracts interface-neutral | First mobile/cloud integration |
| Hardware constraints | Verify exact module, flash, voltage, pin map, and power budget | Before wiring or driver implementation |
