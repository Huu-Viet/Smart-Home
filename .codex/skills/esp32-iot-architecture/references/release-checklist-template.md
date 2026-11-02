# ESP32 IoT release checklist

Mark an item only with evidence (test result, configuration review, or linked runbook). A prototype is not production-ready solely because it works on a bench.

## Prototype gate

- [ ] Board/chip/flash, power budget, pin map, and safe boot state are documented and tested.
- [ ] Secrets are supplied from ignored local configuration or secure CI variables; no credentials appear in source, logs, or Git history.
- [ ] Wi-Fi reconnect uses bounded retry/backoff and the device has defined degraded/offline behavior.
- [ ] Sensor and cloud failures are handled without unsafe actuator behavior; watchdog/reset behavior is known.
- [ ] Partition layout reserves two OTA app slots and persistent configuration storage appropriate to the real flash capacity.
- [ ] Logs redact passwords, tokens, private keys, and provisioned credentials.
- [ ] Device identity, telemetry, commands, desired state, and reported state are documented independently of the selected cloud vendor.

## Production gate

- [ ] Every device receives a unique identity and credential; cloud authorization restricts it to its own resources.
- [ ] TLS validates the server certificate and trust source; no insecure TLS bypass exists in release firmware.
- [ ] Provisioning uses BLE authentication/Proof of Possession or a protected, time-limited fallback; it exposes no public control or OTA endpoint.
- [ ] OTA uses HTTPS, signed firmware, version/manifest validation, a health confirmation window, and rollback on failed boot or health check.
- [ ] OTA signing keys are stored outside Git and separate development from production.
- [ ] Secure Boot, Flash Encryption, and NVS Encryption have been assessed for the exact chip/revision; the approved flash/eFuse process is reviewed before any irreversible action.
- [ ] Firmware, framework, and dependency versions are pinned; security advisories/CVEs were reviewed for the release.
- [ ] Recovery paths are tested for first provisioning, wrong Wi-Fi credentials, prolonged offline state, power loss during OTA, failed new firmware, credential reset, and unavailable cloud.
- [ ] Manufacturing, key custody, device replacement, deprovisioning, and incident-response ownership are recorded.
