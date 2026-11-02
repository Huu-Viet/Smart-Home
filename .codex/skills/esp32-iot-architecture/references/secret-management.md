# Secret management

Use a checked-in example only to show variable names; it must contain no usable SSID, password, token, private key, certificate, or device ID. Store the real local file in an ignored path such as `config/local_config.h` and inject CI/release credentials through the approved secret store.

Never commit:

- Wi-Fi passwords, API tokens, MQTT passwords, provisioning PoP values, or device certificates/private keys.
- OTA signing keys, Secure Boot keys, flash-encryption keys, or NVS-encryption keys.
- Firmware binaries or release artifacts that package protected credentials.

Use unique per-device credentials in production. A source-controlled public root CA may be appropriate, but a device certificate or any private key is not. Do not log raw configuration, HTTP/MQTT authorization headers, or TLS key material.

For key rotation or factory reset, document who authorizes it, the physical or authenticated trigger, whether old credentials are revoked, and the expected offline recovery behavior.
