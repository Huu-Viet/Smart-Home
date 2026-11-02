# Architecture brief — Smart Home ESP32 baseline

## 1. Product and hardware

- **Purpose / user outcome:** Baseline project for learning and evolving an ESP32 smart-home device.
- **Board / chip / revision / flash:** `esp32doit-devkit-v1`; exact module revision and flash capacity must be verified from the physical board before feature implementation.
- **Build baseline:** PlatformIO `espressif32 @ 8.0.0` with Arduino framework. Update only through the release checklist and a rebuild/test. Verified build (2026-11-02, without a local config file and again with a dummy local config file that was deleted afterwards): Arduino core 3.3.12, RAM 14.9% (48,888 / 327,680 bytes), flash 90.0% (about 1,179,000 / 1,310,720 bytes of the default app partition). That flash headroom is a watch item, not a blocker for this telemetry milestone, but it leaves little room before a custom partition layout is needed.
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

This iteration is `boot -> self-check -> local monitoring -> Wi-Fi connecting -> telemetry online/degraded`. PIR triggers are ignored during a 60-second warm-up; afterward a detected motion keeps the buzzer on until no motion has been observed for three seconds. Wi-Fi reconnects every 10 seconds; telemetry is attempted every 60 seconds once a valid DHT reading is available. Timing note, as implemented: the 60-second telemetry timer is anchored at boot (`lastTelemetryAtMs_` starts at 0) and is advanced before each publish attempt, so a failed publish still waits a full 60 seconds before the next attempt. Telemetry is blocking HTTPS while it runs; the local PIR/buzzer loop resumes afterwards. Neither behaviour has been changed in this milestone.

## 4. Connectivity and cloud contract

- **Primary connection:** Wi-Fi station.
- **Provisioning:** BLE with device-specific authentication/Proof of Possession is preferred when mobile onboarding is required. BLE must be stopped after provisioning. A protected, time-limited SoftAP fallback may be introduced later.
- **Cloud transport adapter:** HTTPS to the `ingest-device` Supabase Edge Function. The ESP32 does not access Postgres directly.
- **TLS:** `WiFiClientSecure.useBuiltinCACertBundle()` validates the server certificate. `setInsecure()` is never used; a grep of `src/` confirms it does not appear. The telemetry client uses the `HTTPClient::POST(String)` overload: on Arduino core 3.x the `POST(uint8_t*, size_t)` overload takes a non-const buffer, so passing the const-qualified body failed to compile, and casting away constness was rejected in favour of the String overload.
- **Device identity:** A per-device ID and random token; only the token SHA-256 hash is stored in `public.devices`. Provisioning is done on the developer's own machine with `scripts/provision-device.py`: it generates a 32-byte token, writes the raw token only to the gitignored `config/local_config.h`, prints only the hash and its SQL insert, and refuses to overwrite an existing local config without `--force`. It never needs a database password or a Supabase service_role key.
- **Edge Function error contract, verified against the deployed endpoint (2026-11-02) with dummy tokens only, no real device token:** `GET` returns `{ "ok": true }` (200); missing headers, a short token, and a valid payload with a wrong/dummy token return 401 `unauthorized`; an invalid payload and malformed JSON with a dummy token of at least 32 characters return 400 `invalid_payload` / `invalid_json`. A successful telemetry write (201) and the `public.telemetry` row can only be verified after provisioning on real hardware, and are still pending.

Message contracts will contain device identity, timestamp, firmware version, telemetry, commands, desired state, and reported state without embedding provider-specific concepts in domain logic.

## 5. Storage, OTA, and security

- **Partition/memory budget:** Must be redesigned once actual flash capacity and firmware size are known; reserve dual OTA slots for any connected release. The current verified build already uses 90.0% of the default 1,310,720-byte app partition, so any feature growth (or OTA) should trigger a partition review first.
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
