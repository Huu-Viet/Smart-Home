import "jsr:@supabase/functions-js/edge-runtime.d.ts";
import { createClient } from "npm:@supabase/supabase-js@2";

type TelemetryRequest = {
  temperatureC: number;
  humidityPercent: number;
  motionDetected: boolean;
  buzzerActive: boolean;
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

function hasValidPayload(value: unknown): value is TelemetryRequest {
  if (!value || typeof value !== "object") return false;
  const data = value as Record<string, unknown>;
  return Number.isFinite(data.temperatureC) && data.temperatureC >= -40 &&
      data.temperatureC <= 85 && Number.isFinite(data.humidityPercent) &&
      data.humidityPercent >= 0 && data.humidityPercent <= 100 &&
      typeof data.motionDetected === "boolean" && typeof data.buzzerActive === "boolean" &&
      Number.isInteger(data.uptimeMs) && data.uptimeMs >= 0 &&
      data.uptimeMs <= 4294967295 && typeof data.firmwareVersion === "string" &&
      data.firmwareVersion.length > 0 && data.firmwareVersion.length <= 64;
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

  if (!hasValidPayload(payload)) return json(400, { error: "invalid_payload" });

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

  const { error: telemetryError } = await supabase.from("telemetry").insert({
    device_id: device.id,
    temperature_c: payload.temperatureC,
    humidity_percent: payload.humidityPercent,
    motion_detected: payload.motionDetected,
    buzzer_active: payload.buzzerActive,
    uptime_ms: payload.uptimeMs,
    firmware_version: payload.firmwareVersion,
  });

  if (telemetryError) return json(500, { error: "telemetry_write_failed" });

  const { error: deviceUpdateError } = await supabase
    .from("devices")
    .update({ last_seen_at: new Date().toISOString() })
    .eq("id", device.id);

  if (deviceUpdateError) return json(500, { error: "device_update_failed" });
  return json(201, { ok: true });
});
