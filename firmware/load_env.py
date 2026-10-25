# PlatformIO pre-script: injects the values of .env as string macros (-D KEY="value").
Import("env")  # noqa: F821
import os

ENV_FILE = os.path.join(env["PROJECT_DIR"], ".env")  # noqa: F821
REQUIRED = ["WIFI_SSID", "WIFI_PASSWORD"]

if not os.path.isfile(ENV_FILE):
    raise SystemExit("Missing .env: copy .env.example to .env and fill in your values.")

values = {}
with open(ENV_FILE, encoding="utf-8") as f:
    for line in f:
        line = line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, val = line.split("=", 1)
        val = val.strip()
        if len(val) >= 2 and val[0] == val[-1] and val[0] in "\"'":
            val = val[1:-1]
        values[key.strip()] = val

missing = [k for k in REQUIRED if not values.get(k)]
if missing:
    raise SystemExit("Missing in .env: " + ", ".join(missing))

# Escape for a C string literal; SCons passes it through the compiler command line.
defs = []
for k in REQUIRED:
    esc = values[k].replace("\\", "\\\\").replace('"', '\\"')
    defs.append((k, env.StringifyMacro(esc)))  # noqa: F821
env.Append(CPPDEFINES=defs)  # noqa: F821
