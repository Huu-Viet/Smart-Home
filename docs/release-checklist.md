# ESP32 IoT release checklist

This project is currently an early scaffold. Use this checklist as features are added; no production gate is currently passed.

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
