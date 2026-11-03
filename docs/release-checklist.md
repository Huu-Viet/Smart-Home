# ESP32 IoT release checklist

This project is currently an early scaffold. Use this checklist as features are added; no production gate is currently passed.

## Split-stream redesign status (2026-11-03, firmware 0.2.0)

- [x] Firmware builds: sensor readings every 60 s to `public.sensor_readings` (temperature + humidity only) and motion events immediately on PIR transitions to `public.motion_events` (`motion_started` / `motion_ended`, no sensor fields, buzzer stays local-only). Failed motion events are kept pending and retried at most every 5 s; DHT failure blocks only sensor readings.
- [x] Migration `20261103220000_split_sensor_and_motion.sql` creates both tables with owner-only RLS matching the old pattern. It does NOT drop `public.telemetry`; that table is deprecated and unwritten, pending a later cleanup migration.
- [x] Edge Function `ingest-device` routes by payload `kind` with the same device-token validation and 401/400 contract. The old bundled payload is now rejected with 400 by design.
- [ ] Deploy, in order, on the developer's own Supabase project: migration, then function, then firmware 0.2.0. Then verify: sensor rows in `sensor_readings`, motion rows in `motion_events` on a PIR transition, and Serial Monitor lines for both. Old firmware will get 400 between the function deploy and the flash, so keep that window short.
- [ ] Run Database > Advisors after this migration/function change and handle its findings. This turn DOES change schema and function, unlike the 2026-11-02 turn.
- [ ] The 2026-11-02 pending items (provisioning, flash, Serial Monitor, telemetry row) are superseded by the split deploy above: verify against the new tables, not `public.telemetry`.

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
