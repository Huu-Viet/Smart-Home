---
name: esp32-iot-architecture
description: Design the system architecture, connectivity, security, OTA, and release gates for ESP32 IoT devices built with PlatformIO and Arduino before application code is written.
---

# ESP32 IoT Architecture

Use this skill before implementing a new device capability, sensor or actuator integration, Wi-Fi/BLE provisioning, cloud connectivity, OTA, or a release. It designs the system; it does not write the application firmware.

The default target is ESP32 DevKit/WROOM with PlatformIO + Arduino, Wi-Fi as the primary transport, optional BLE provisioning, and an interface-neutral MQTT/HTTPS cloud integration for fleets of one to a few dozen devices. Treat Arduino as a convenience API on top of ESP-IDF capabilities; do not make an Arduino-library choice a domain or cloud architecture decision.

## Required outputs

Before application code, create or update:

1. `docs/architecture.md`, using [the architecture template](references/architecture-template.md).
2. `docs/release-checklist.md`, using [the release template](references/release-checklist-template.md).

Read [secret management](references/secret-management.md) whenever configuration, certificates, tokens, signing, or provisioning credentials are involved. Copy `assets/local_config.example.h` to the project's ignored local configuration location and fill it only on the developer machine; never add real credentials to source control.

When creating or restructuring firmware modules, read [the project structure convention](references/project-structure.md). It is the default layout for a small or medium device; promote only genuinely reusable modules to `lib/`.

## Architecture decisions

- Keep the dependency direction: **hardware drivers -> domain/device state -> connectivity/services -> app orchestration**. `setup()` and `loop()` only assemble and schedule these layers; they must not become a mixture of GPIO, Wi-Fi, MQTT, and business rules.
- Use module-local headers beside their implementation rather than a single global `include/` directory. `app_config.h` contains only non-secret build-time configuration; local secrets belong in the ignored local configuration file.
- Model device lifecycle explicitly: `boot -> self-check -> unprovisioned/provisioning -> connecting -> online/degraded -> OTA/recovery`. Specify retry limits, backoff, timeout, and the safe actuator behavior for every failed state.
- Make device identity, telemetry, commands, desired/reported state, firmware version, and timestamps cloud-independent. Put MQTT or HTTPS implementation behind a connectivity adapter. Use a per-device credential and authorization scoped to that device's resources.
- Default to Wi-Fi station mode. Use BLE provisioning only when it materially improves mobile onboarding; authenticate provisioning with Proof of Possession or an equivalent device-specific secret, then stop/deinitialize BLE when it is no longer needed. SoftAP is a protected, time-limited fallback and must not expose control or firmware-update endpoints publicly.
- Plan OTA before writing features: use dual OTA application slots, a versioned manifest, HTTPS, signature verification, post-boot health confirmation, and rollback. Do not adopt unauthenticated web OTA/AP examples as a product baseline.
- Pin the ESP32 Arduino core/platform version in `platformio.ini`. Treat dependency upgrades and security advisories as a release-gate review.

## Security gates

For prototypes, keep secrets outside Git, redact sensitive logs, use reconnect/backoff and watchdog/error handling, and reserve a partition layout capable of OTA.

Before production, record the target chip revision and manufacturing/flash procedure, then require unique device identities, TLS certificate validation, signed HTTPS OTA with rollback, and assessed Secure Boot, Flash Encryption, and NVS Encryption. Production private keys must not enter the repository or firmware image. Escalate to ESP-IDF-specific tooling where Arduino/PlatformIO cannot safely configure or verify these capabilities.

## Boundaries

Do not introduce Matter, Thread, Zigbee, continuous BLE control, or a specific cloud vendor unless the product request calls for it. Do not flash eFuses, enable irreversible security settings, provision real secrets, or deploy cloud resources without explicit user authorization.

## Primary references

- [ESP-IDF component architecture](https://github.com/espressif/esp-idf/blob/master/docs/en/api-guides/build-system.rst)
- [Espressif Wi-Fi provisioning](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/provisioning/provisioning.html)
- [Security enablement workflows](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/security/security-features-enablement-workflows.html)
- [Arduino ESP32 signed OTA support](https://github.com/espressif/arduino-esp32/blob/master/libraries/Update/README.md)
