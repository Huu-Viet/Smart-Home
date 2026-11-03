import "jsr:@supabase/functions-js/edge-runtime.d.ts";
import { createClient } from "npm:@supabase/supabase-js@2";

// Split payloads, one table per stream. The two streams are independent:
// a sensor reading never carries motion fields, and a motion event never
// carries temperature or humidity, so neither send is forced to include
// the other, and statistics on one stream cannot be skewed by the other.
//
// Sensor (periodic, every 60 s from the device):
//   { kind: "sensor", temperatureC, humidityPercent, uptimeMs, firmwareVersion }
// Motion (event-driven, on each PIR transition):
//   { kind: "motion", eventType: "motion_started" | "motion_ended",
//     uptimeMs, firmwareVersion }
//
// The old bundled payload (temperature + motion + buzzer in one object,
// no kind) is intentionally rejected with 400: it belonged to firmware
// 0.1.0 and public.telemetry, which is deprecated. Deploy this function
// together with the split migration, then flash firmware 0.2.0; during
// the short window between the two, old firmware gets 400, not a
// silent write to the wrong table.

type SensorRequest = {
  kind: "sensor";
  temperatureC: number;
  humidityPercent: number;
  uptimeMs: number;
  firmwareVersion: string;
};

type MotionRequest = {
  kind: "motion";
  eventType: "motion_started" | "motion_ended";
  uptimeMs: number;
  firmwareVersion: string;
};

const encoder = new TextEncoder();

function getServiceKey(): string {
  const legacyKey = Deno.env.get("SUPABASE_SERVICE_ROLE_KEY");
  if (legacyKey) return legacyKey;

  const keys = Deno.env.get("SUPABASE_SECRET_KEYS");
  if (!keys) throw new Error("Supabase secret key is unavailable");

  const parsed = JSON.parse(keys) as Record<string, string>;
  if (!parsed.default) throw new Error("Default Supabase secret key is unavailable");
  return parsed.default;
}

function json(status: number, body: Record<string, unknown>): Response {
  return new Response(JSON.stringify(body), {
    status,
    headers: { "Content-Type": "application/json" },
  });
}

function hasValidUptimeAndVersion(data: Record<string, unknown>): boolean {
  return Number.isInteger(data.uptimeMs) && data.uptimeMs >= 0 &&
      data.uptimeMs <= 4294967295 && typeof data.firmwareVersion === "string" &&
      data.firmwareVersion.length > 0 && data.firmwareVersion.length <= 64;
}

function hasValidSensorPayload(value: unknown): value is SensorRequest {
  if (!value || typeof value !== "object") return false;
  const data = value as Record<string, unknown>;
  return data.kind === "sensor" &&
      Number.isFinite(data.temperatureC) && data.temperatureC >= -40 &&
      data.temperatureC <= 85 && Number.isFinite(data.humidityPercent) &&
      data.humidityPercent >= 0 && data.humidityPercent <= 100 &&
      hasValidUptimeAndVersion(data);
}

function hasValidMotionPayload(value: unknown): value is MotionRequest {
  if (!value || typeof value !== "object") return false;
  const data = value as Record<string, unknown>;
  return data.kind === "motion" &&
      (data.eventType === "motion_started" || data.eventType === "motion_ended") &&
      hasValidUptimeAndVersion(data);
}

async function sha256Hex(value: string): Promise<string> {
  const digest = await crypto.subtle.digest("SHA-256", encoder.encode(value));
  return Array.from(new Uint8Array(digest), (byte) => byte.toString(16).padStart(2, "0")).join("");
}

function sameValue(left: string, right: string): boolean {
  const leftBytes = encoder.encode(left);
  const rightBytes = encoder.encode(right);
  let difference = leftBytes.length ^ rightBytes.length;
  const maxLength = Math.max(leftBytes.length, rightBytes.length);

  for (let index = 0; index < maxLength; index += 1) {
    difference |= (leftBytes[index] ?? 0) ^ (rightBytes[index] ?? 0);
  }

  return difference === 0;
}

Deno.serve(async (request: Request) => {
  if (request.method === "GET") return json(200, { ok: true });
  if (request.method !== "POST") return json(405, { error: "method_not_allowed" });

  const deviceId = request.headers.get("x-device-id") ?? "";
  const deviceToken = request.headers.get("x-device-token") ?? "";
  if (!/^[a-z0-9][a-z0-9-]{2,63}$/.test(deviceId) || deviceToken.length < 32) {
    return json(401, { error: "unauthorized" });
  }

  let payload: unknown;
  try {
    payload = await request.json();
  } catch {
    return json(400, { error: "invalid_json" });
  }

  const isSensor = hasValidSensorPayload(payload);
  const isMotion = !isSensor && hasValidMotionPayload(payload);
  if (!isSensor && !isMotion) return json(400, { error: "invalid_payload" });

  const supabase = createClient(Deno.env.get("SUPABASE_URL")!, getServiceKey());
  const { data: device, error: deviceError } = await supabase
    .from("devices")
    .select("id, device_token_hash, is_active")
    .eq("id", deviceId)
    .maybeSingle();

  if (deviceError || !device || !device.is_active ||
      !sameValue(await sha256Hex(deviceToken), device.device_token_hash)) {
    return json(401, { error: "unauthorized" });
  }

  const { error: writeError } = isSensor
    ? await supabase.from("sensor_readings").insert({
      device_id: device.id,
      temperature_c: (payload as SensorRequest).temperatureC,
      humidity_percent: (payload as SensorRequest).humidityPercent,
      uptime_ms: (payload as SensorRequest).uptimeMs,
      firmware_version: (payload as SensorRequest).firmwareVersion,
    })
    : await supabase.from("motion_events").insert({
      device_id: device.id,
      event_type: (payload as MotionRequest).eventType,
      uptime_ms: (payload as MotionRequest).uptimeMs,
      firmware_version: (payload as MotionRequest).firmwareVersion,
    });

  if (writeError) return json(500, { error: "telemetry_write_failed" });

  const { error: deviceUpdateError } = await supabase
    .from("devices")
    .update({ last_seen_at: new Date().toISOString() })
    .eq("id", device.id);

  if (deviceUpdateError) return json(500, { error: "device_update_failed" });
  return json(201, { ok: true });
});
