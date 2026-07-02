"""
Lets you flash/monitor a different board without editing platformio.ini.

Set TESSERA_PORT before running pio, and it overrides both upload_port and
monitor_port for this run. Leave it unset and the platformio.ini defaults
(COM7) apply as before.

  PowerShell:  $env:TESSERA_PORT = "COM6"; pio run -t upload
  bash:        TESSERA_PORT=COM6 pio run -t upload
"""
import os

Import("env")

port = os.environ.get("TESSERA_PORT")
if port:
    env.Replace(UPLOAD_PORT=port)
    env.Replace(MONITOR_PORT=port)
    print(f"[select_port] Using TESSERA_PORT={port} for upload/monitor")
