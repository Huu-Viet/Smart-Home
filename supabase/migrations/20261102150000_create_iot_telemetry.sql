create table public.devices (
  id text primary key check (id ~ '^[a-z0-9][a-z0-9-]{2,63}$'),
  owner_user_id uuid references auth.users(id) on delete set null,
  name text not null check (char_length(name) between 1 and 100),
  device_token_hash text not null check (device_token_hash ~ '^[a-f0-9]{64}$'),
  is_active boolean not null default true,
  last_seen_at timestamptz,
  created_at timestamptz not null default now()
);

create table public.telemetry (
  id bigint generated always as identity primary key,
  device_id text not null references public.devices(id) on delete cascade,
  recorded_at timestamptz not null default now(),
  temperature_c numeric(5, 2) not null check (temperature_c between -40 and 85),
  humidity_percent numeric(5, 2) not null check (humidity_percent between 0 and 100),
  motion_detected boolean not null,
  buzzer_active boolean not null,
  uptime_ms bigint not null check (uptime_ms between 0 and 4294967295),
  firmware_version text not null check (char_length(firmware_version) between 1 and 64)
);

create index telemetry_device_recorded_at_idx
  on public.telemetry (device_id, recorded_at desc);

alter table public.devices enable row level security;
alter table public.telemetry enable row level security;

revoke all on table public.devices, public.telemetry from anon;
revoke all on table public.devices, public.telemetry from authenticated;
grant select on table public.devices, public.telemetry to authenticated;

create policy "Owners can read their devices"
  on public.devices for select to authenticated
  using ((select auth.uid()) = owner_user_id);

create policy "Owners can read their telemetry"
  on public.telemetry for select to authenticated
  using (
    exists (
      select 1 from public.devices
      where devices.id = telemetry.device_id
        and devices.owner_user_id = (select auth.uid())
    )
  );
