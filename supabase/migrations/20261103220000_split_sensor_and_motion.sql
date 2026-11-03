-- Split the old bundled telemetry snapshot into two independent streams.
--
-- public.sensor_readings: periodic DHT11 measurements. Temperature and
-- humidity stay together because they are one sensor read at one instant.
--
-- public.motion_events: discrete PIR transitions. One row per transition
-- (motion_started / motion_ended), sent by the device the moment the PIR
-- output changes, not on the sensor cadence. Counting motion events is
-- therefore count(*) on this table, and averaging temperature no longer
-- has to filter out motion-event rows or vice versa.
--
-- public.telemetry is intentionally NOT dropped here: it holds the old
-- bundled snapshot rows, whose motion fields were only a 60-second
-- snapshot and are not reliable event history. The new firmware and Edge
-- Function no longer write to it. Drop it in a later migration, after the
-- split tables have been verified and any needed history has been kept.

create table public.sensor_readings (
  id bigint generated always as identity primary key,
  device_id text not null references public.devices(id) on delete cascade,
  recorded_at timestamptz not null default now(),
  temperature_c numeric(5, 2) not null check (temperature_c between -40 and 85),
  humidity_percent numeric(5, 2) not null check (humidity_percent between 0 and 100),
  uptime_ms bigint not null check (uptime_ms between 0 and 4294967295),
  firmware_version text not null check (char_length(firmware_version) between 1 and 64)
);

create index sensor_readings_device_recorded_at_idx
  on public.sensor_readings (device_id, recorded_at desc);

create table public.motion_events (
  id bigint generated always as identity primary key,
  device_id text not null references public.devices(id) on delete cascade,
  recorded_at timestamptz not null default now(),
  event_type text not null check (event_type in ('motion_started', 'motion_ended')),
  uptime_ms bigint not null check (uptime_ms between 0 and 4294967295),
  firmware_version text not null check (char_length(firmware_version) between 1 and 64)
);

create index motion_events_device_recorded_at_idx
  on public.motion_events (device_id, recorded_at desc);

alter table public.sensor_readings enable row level security;
alter table public.motion_events enable row level security;

revoke all on table public.sensor_readings, public.motion_events from anon;
revoke all on table public.sensor_readings, public.motion_events from authenticated;
grant select on table public.sensor_readings, public.motion_events to authenticated;

create policy "Owners can read their sensor readings"
  on public.sensor_readings for select to authenticated
  using (
    exists (
      select 1 from public.devices
      where devices.id = sensor_readings.device_id
        and devices.owner_user_id = (select auth.uid())
    )
  );

create policy "Owners can read their motion events"
  on public.motion_events for select to authenticated
  using (
    exists (
      select 1 from public.devices
      where devices.id = motion_events.device_id
        and devices.owner_user_id = (select auth.uid())
    )
  );

comment on table public.telemetry is
  'Deprecated: old bundled snapshot (temperature, humidity, motion, buzzer in one row). Superseded by public.sensor_readings and public.motion_events; no longer written by firmware 0.2.0 or ingest-device.';
