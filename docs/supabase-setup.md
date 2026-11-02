# Supabase telemetry setup

The `ingest-device` Edge Function accepts telemetry only from a provisioned device. Do not give the ESP32 a Supabase `service_role` key or a database password.

## One-time device provisioning

Preferred method, on your own computer from the repository root:

```bash
python3 scripts/provision-device.py
```

It asks for Wi-Fi SSID/password locally, generates a 32-byte token, writes the raw token only to the gitignored `config/local_config.h`, and prints only the SHA-256 hash plus its SQL insert. Paste that SQL into the Supabase SQL editor. The script never prints the raw token and never needs a database password or service_role key.

Manual alternative, on your own computer: generate a random device token and its SHA-256 hash. Keep the token private; only the hash belongs in the database.

```powershell
$bytes = [byte[]]::new(32)
[System.Security.Cryptography.RandomNumberGenerator]::Fill($bytes)
$token = [Convert]::ToHexString($bytes).ToLowerInvariant()
$hash = [Convert]::ToHexString([System.Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes($token))).ToLowerInvariant()
Write-Output "DEVICE_TOKEN=$token"
Write-Output "DEVICE_TOKEN_SHA256=$hash"
```

Insert the displayed hash using the Supabase SQL editor, replacing the placeholders. Do not paste the raw token into SQL or source control.

```sql
insert into public.devices (id, name, device_token_hash)
values ('desk-esp32-01', 'Desk ESP32', '<DEVICE_TOKEN_SHA256>');
```

Copy `config/local_config.example.h` to the ignored `config/local_config.h`, then replace Wi-Fi settings and `kDeviceToken` with the raw token. The firmware sends telemetry every 60 seconds after Wi-Fi connects.

## Validation

- `GET https://vdkykmyjfrzaluycycla.supabase.co/functions/v1/ingest-device` returns `{ "ok": true }`.
- With a provisioned device, Serial Monitor reports `Telemetry sent to Supabase.`
- Confirm new rows appear in `public.telemetry`; never expose the tables directly to `anon` clients.
