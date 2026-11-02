#!/usr/bin/env python3
"""Provision desk-esp32-01 on the developer's own machine only.

Run this from the repository root, on the machine that will build and flash
the firmware:

    python3 scripts/provision-device.py

What it does, locally:
  * Generates a fresh 32-byte random device token (64 hex characters).
  * Writes the raw token ONLY to config/local_config.h, which is gitignored.
  * Prints ONLY the SHA-256 hash of the token, plus the SQL statement that
    inserts that hash into public.devices. Paste that SQL into the Supabase
    SQL editor while logged in to your own project; never paste the raw
    token into SQL, source control, chat, or a log.

What it never does:
  * It never prints, logs, or uploads the raw token or the Wi-Fi password.
  * It never asks for, and never needs, a database password, a Supabase
    service_role key, or any Firebase credential.
  * It refuses to overwrite an existing config/local_config.h unless you
    pass --force, so an already-provisioned device is not clobbered.

After running it:
  1. Paste the printed SQL into the Supabase SQL editor and run it.
  2. Build and flash from the same machine:
         pio run -t upload
         pio device monitor
  3. Expect "Telemetry sent to Supabase." in the Serial Monitor, then
     confirm a new row in public.telemetry for desk-esp32-01.
"""

import argparse
import getpass
import hashlib
import os
import secrets
import subprocess
import sys
from pathlib import Path

DEVICE_ID = "desk-esp32-01"
DEVICE_NAME = "Desk ESP32"
INGEST_URL = "https://vdkykmyjfrzaluycycla.supabase.co/functions/v1/ingest-device"


def main() -> int:
    parser = argparse.ArgumentParser(description="Provision desk-esp32-01 locally.")
    parser.add_argument(
        "--force",
        action="store_true",
        help="Overwrite an existing config/local_config.h.",
    )
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parent.parent
    local_config = repo_root / "config" / "local_config.h"

    if local_config.exists() and not args.force:
        print(
            "config/local_config.h already exists. Refusing to overwrite it.\n"
            "Re-run with --force only if you intend to re-provision this device\n"
            "(the old token will stop working once you replace its hash).",
            file=sys.stderr,
        )
        return 1

    # Check the file is ignored before writing any secret into it.
    ignored = subprocess.run(
        ["git", "check-ignore", "-q", "config/local_config.h"],
        cwd=repo_root,
        capture_output=True,
    )
    if ignored.returncode != 0:
        print(
            "Refusing to write secrets: config/local_config.h is not gitignored "
            "in this checkout. Fix .gitignore first.",
            file=sys.stderr,
        )
        return 1

    ssid = input("Wi-Fi SSID: ").strip()
    if not ssid:
        print("Wi-Fi SSID must not be empty.", file=sys.stderr)
        return 1
    password = getpass.getpass("Wi-Fi password (typed locally, never printed): ")
    if not password:
        print("Wi-Fi password must not be empty.", file=sys.stderr)
        return 1

    # 32 random bytes -> 64 hex chars, comfortably above the function's
    # minimum token length of 32 characters.
    token = secrets.token_hex(32)
    token_hash = hashlib.sha256(token.encode("utf-8")).hexdigest()

    def c_escape(value: str) -> str:
        return value.replace("\\", "\\\\").replace('"', '\\"')

    local_config.parent.mkdir(parents=True, exist_ok=True)
    local_config.write_text(
        "#pragma once\n"
        "\n"
        "// Generated locally by scripts/provision-device.py. Gitignored.\n"
        "// Do not commit this file, and do not paste its token anywhere else.\n"
        "\n"
        "namespace LocalConfig {\n"
        f'constexpr char kWifiSsid[] = "{c_escape(ssid)}";\n'
        f'constexpr char kWifiPassword[] = "{c_escape(password)}";\n'
        "\n"
        f'constexpr char kSupabaseIngestUrl[] = "{INGEST_URL}";\n'
        f'constexpr char kDeviceId[] = "{DEVICE_ID}";\n'
        f'constexpr char kDeviceToken[] = "{token}";\n'
        "}  // namespace LocalConfig\n",
        encoding="utf-8",
    )
    try:
        os.chmod(local_config, 0o600)
    except OSError:
        # Best effort only (e.g. Windows filesystems); the gitignore check
        # above is the binding protection against committing the file.
        pass

    print()
    print("Local config written to config/local_config.h (gitignored).")
    print("The raw token was NOT printed. It lives only in that file.")
    print()
    print("Paste this SQL into the Supabase SQL editor (hash only, no token):")
    print()
    print(
        "insert into public.devices (id, name, device_token_hash)\n"
        f"values ('{DEVICE_ID}', '{DEVICE_NAME}', '{token_hash}')\n"
        "on conflict (id) do update set device_token_hash = excluded.device_token_hash, "
        "is_active = true;"
    )
    print()
    print("Then build and flash from this machine:  pio run -t upload")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
