# ESP32 IoT release checklist

This project is currently an early scaffold. Use this checklist as features are added; no production gate is currently passed.

## Telemetry milestone status (2026-11-02)

- [x] Firmware compiles on PlatformIO `espressif32 @ 8.0.0`, with and without `config/local_config.h` (dummy local config used for the second build, then deleted).
- [x] HTTPS certificate validation kept: `useBuiltinCACertBundle()`, no `setInsecure()` in `src/`.
- [x] Deployed `ingest-device` error contract verified with dummy tokens only: GET 200, wrong/short/missing token 401, invalid payload / malformed JSON 400.
- [ ] Provisioning run on the developer's machine (`scripts/provision-device.py`), SQL hash insert applied in the Supabase SQL editor, firmware flashed, Serial Monitor checked, and a `public.telemetry` row confirmed. This needs the physical board and the developer's own Supabase dashboard access; it was not done from this build machine, and no database password, service_role key, or raw device token was requested or used to do it.
- [ ] No schema or Edge Function change was made in this turn, so there is no new change for the Supabase security/performance advisors to assess. Run Database > Advisors in the dashboard after any future migration or function change, and after provisioning if you want a fresh baseline.

Out of scope for this milestone, and not implemented here: mobile app, Firebase Cloud Messaging, weather API, remote buzzer-off command, and OTA.

## Prototype gate

- [ ] Board/chip/flash, power budget, pin map, and safe boot state are documented and tested. GPIO 4 (DHT11), GPIO 27 (PIR), and GPIO 26 (active buzzer) are documented but still require hardware verification.
- [x] `.gitignore` excludes local configuration, private key material, and build/release artifacts.
- [ ] Secrets are supplied locally or through secure CI variables; no credentials appear in source or logs.
- [ ] Wi-Fi reconnect/backoff, offline behavior, watchdog behavior, and failure-safe actuators are implemented and tested.
- [ ] Partition layout reserves dual OTA app slots and persistent configuration storage suitable for actual flash capacity.
- [ ] Cloud-neutral telemetry, command, desired-state, and reported-state contracts are documented.

## Production gate

- [ ] Unique device identity and restricted per-device credentials exist.
- [ ] TLS validates the server certificate without release-time bypasses.
- [ ] Provisioning is authenticated and time-limited; no public control or OTA endpoint exists.
- [ ] HTTPS OTA verifies signatures, confirms health after boot, and rolls back safely.
- [ ] Signing keys are outside Git and separated by environment.
- [ ] Secure Boot, Flash Encryption, and NVS Encryption are assessed for the exact chip/revision and production procedure.
- [ ] Framework/dependencies are pinned and security advisories reviewed.
- [ ] Failure recovery scenarios are tested and documented.
