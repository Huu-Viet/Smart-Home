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

Copy `config/local_config.example.h` to the ignored `config/local_config.h`, then replace Wi-Fi settings and `kDeviceToken` with the raw token.

## Split streams (firmware 0.2.0)

Apply, in this order, on your own Supabase project: first the migration `supabase/migrations/20261103220000_split_sensor_and_motion.sql` (creates `public.sensor_readings` and `public.motion_events`, with the same owner-only RLS pattern as before), then deploy the updated `ingest-device` function, then flash firmware 0.2.0. Do not deploy only half of the pair: the new function rejects the old bundled payload with 400, and old firmware cannot write the new tables.

- Sensor readings: every 60 seconds after Wi-Fi connects, one row in `public.sensor_readings` with temperature and humidity only.
- Motion events: immediately on each PIR transition, one row in `public.motion_events` with `eventType` `motion_started` or `motion_ended`. No temperature, humidity, or buzzer field is attached.
- `public.telemetry` is deprecated and no longer written to; it is not dropped by this migration.

After any migration or function change, run Database > Advisors in the Supabase dashboard and handle its findings before calling the change done.

## Validation

- `GET https://vdkykmyjfrzaluycycla.supabase.co/functions/v1/ingest-device` returns `{ "ok": true }`.
- With a provisioned device, Serial Monitor reports `Sensor reading sent to Supabase.` every 60 seconds, and `Motion started event sent to Supabase.` / `Motion ended event sent to Supabase.` on PIR transitions.
- Confirm new rows appear in `public.sensor_readings` and `public.motion_events`; never expose the tables directly to `anon` clients.
