# Project structure convention

Use this layout as the default for a small or medium PlatformIO + Arduino firmware. Add a directory only when the corresponding responsibility exists; this is a guide, not an empty-folder requirement.

```text
esp32_firmware/
├── platformio.ini
├── include/
│   └── app_config.h               # non-secret, build-time configuration only
├── src/
│   ├── main.cpp                   # composition root: create dependencies and run the app
│   ├── app/
│   │   ├── device_controller.h
│   │   └── device_controller.cpp  # lifecycle and application orchestration
│   ├── domain/                    # add when device state/policy becomes non-trivial
│   │   └── device_state.h
│   ├── hardware/
│   │   ├── gpio_service.h
│   │   └── gpio_service.cpp       # GPIO/bus/sensor/actuator primitives
│   ├── connectivity/
│   │   ├── wifi_manager.h
│   │   ├── wifi_manager.cpp
│   │   ├── provisioning_service.h # only when provisioning is implemented
│   │   ├── mqtt_client.h          # or http_client.h, behind an adapter
│   │   └── mqtt_client.cpp
│   ├── config/
│   │   ├── device_config.h
│   │   └── device_config.cpp      # validation and persisted configuration; no hard-coded secrets
│   └── utils/
│       ├── logger.h
│       └── logger.cpp             # cross-cutting helpers only
├── config/
│   └── local_config.h             # ignored; copied from the skill's example
└── test/
    ├── test_device_controller/
    └── test_device_config/
```

## Ownership rules

- `main.cpp` is a composition root. It wires dependencies and delegates lifecycle calls; it does not contain GPIO operations, HTTP/MQTT parsing, or product policy.
- `hardware/` knows physical interfaces, not cloud topics, Wi-Fi credentials, or business behavior.
- `domain/` owns device state, validation, desired/reported reconciliation, and safe behavior without depending on Arduino transport APIs.
- `connectivity/` owns Wi-Fi, provisioning, MQTT/HTTPS, time synchronization, and OTA transport. Prefer this name over `network/`, because it covers more than IP networking.
- Add an `http_server` only for a deliberate local UI or maintenance use case. It must be authenticated and is not the default cloud/OTA path; a cloud-first device usually needs an HTTPS client instead.
- `config/` loads, validates, and persists settings. It must not compile real credential values into firmware.
- `utils/` is only for small cross-cutting helpers. Do not turn it into an unowned collection of device behavior.

## Header and library policy

Place a module's public header next to its `.cpp` in `src/` during early development. This keeps its ownership explicit and avoids a global header bucket. Move a module to `lib/<module-name>/include` and `lib/<module-name>/src` only when it is independently reusable across firmware projects or needs independently declared dependencies.

Mirror behavior-focused tests under `test/`, for example `test/test_device_controller/` and `test/test_device_config/`. Hardware-dependent code should be behind an interface so domain behavior can be tested without a physical board.
