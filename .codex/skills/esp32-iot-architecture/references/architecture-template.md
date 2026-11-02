# Architecture brief — <device name>

Update this document before writing firmware that changes device behavior, connectivity, or release posture. Replace every angle-bracket placeholder.

## 1. Product and hardware

- **Purpose / user outcome:** <what this device does>
- **Board / chip / revision / flash:** <for example ESP32-WROOM-32, 4 MB; verify from hardware>
- **Power budget and failure behavior:** <supply, peak current, brownout policy, safe actuator state>
- **Capabilities:** <sensors, actuators, local UI>
- **Pin and electrical map:**

| Function | GPIO | Electrical constraints | Boot / safe state | Verified on hardware |
| --- | --- | --- | --- | --- |
| <function> | <GPIO> | <voltage, pull-up, bus, external driver> | <state> | <yes/no> |

## 2. Layering and ownership

```text
Hardware drivers -> Domain/device state -> Connectivity/services -> App orchestration
```

| Layer | Owns | Must not own |
| --- | --- | --- |
| Hardware drivers | GPIO, I2C/SPI/UART, sensor sampling, actuator primitives | Wi-Fi, cloud topics, product policy |
| Domain/device state | validation, state transitions, safe behavior, desired/reported reconciliation | Arduino transport APIs |
| Connectivity/services | Wi-Fi lifecycle, provisioning, MQTT/HTTPS adapter, time, OTA | hardware policy |
| App orchestration | startup order, scheduling, dependency wiring, lifecycle dispatch | business logic or protocol parsing |

- **Selected code layout:** <use the default `src/app`, `src/domain`, `src/hardware`, `src/connectivity`, `src/config`, and `src/utils` layout, or explain the justified variation>. See [project structure convention](project-structure.md).

## 3. Lifecycle and recovery

```text
boot -> self-check -> unprovisioned/provisioning -> connecting -> online/degraded -> OTA/recovery
```

| State | Entry condition | Timeout/retry/backoff | Safe behavior | Exit condition |
| --- | --- | --- | --- | --- |
| <state> | <condition> | <policy> | <actuator/network behavior> | <condition> |

Define reset behavior, credential reset authorization, watchdog policy, and how the device behaves during a cloud or sensor outage.

## 4. Connectivity and cloud contract

- **Primary connection:** Wi-Fi station; network and credential ownership: <details>.
- **Provisioning:** <BLE with PoP / protected SoftAP fallback / other>; stop or deinitialize BLE after: <condition>.
- **Cloud transport adapter:** <MQTT / HTTPS / both>; TLS trust source and certificate validation: <details>.
- **Device identity:** <immutable ID and per-device credential strategy>.

| Message kind | Direction | Cloud-neutral fields | Delivery / authorization |
| --- | --- | --- | --- |
| Telemetry | device -> service | `deviceId`, `timestamp`, `firmwareVersion`, `data` | <QoS/retry/ACL> |
| Command | service -> device | `commandId`, `issuedAt`, `payload` | <authentication/idempotency> |
| Desired state | service -> device | `version`, `state` | <retained only if controlled> |
| Reported state | device -> service | `version`, `state`, `timestamp` | <retry/ACL> |

## 5. Storage, OTA, and security

- **Partition/memory budget:** <OTA slots, NVS/config, filesystem if used; fit within actual flash size>.
- **OTA:** HTTPS endpoint/manifest, signature algorithm/key ownership, version policy, health confirmation, rollback trigger.
- **Secrets:** locations, rotation/reset path, log redaction; no real value belongs in Git.
- **Threat model:** physical access; rogue Wi-Fi; credential leak; malicious firmware; cloud compromise; power/network failure. For each, state the mitigation and residual risk.
- **Production assessment:** Secure Boot <status>; Flash Encryption <status>; NVS Encryption <status>; chip/revision and irreversible eFuse procedure <status>. Link the approved manufacturing/runbook before enabling irreversible settings.

## 6. Decisions and open risks

| Decision or risk | Chosen approach / mitigation | Owner | Review trigger |
| --- | --- | --- | --- |
| <item> | <decision> | <name> | <event> |
