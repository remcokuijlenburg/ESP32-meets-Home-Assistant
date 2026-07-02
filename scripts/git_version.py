"""
Embeds the current git commit into the firmware as FIRMWARE_VERSION, so
serial output shows exactly which commit is running on a given panel --
useful once panels.py lets multiple boards' firmware drift apart over time.

No manual version bumping, so it can never go stale: it's derived from git at
every build. This repo has no tags, so the value is the short commit hash
(e.g. "a1b2c3d"), with a "-dirty" suffix appended if the working tree has
uncommitted changes at build time -- a real signal that what's on the panel
isn't exactly what's in git history.
"""
import subprocess

Import("env")


def get_firmware_version():
    try:
        result = subprocess.run(
            ["git", "describe", "--tags", "--always", "--dirty"],
            cwd=env["PROJECT_DIR"], capture_output=True, text=True, timeout=5,
        )
        version = result.stdout.strip()
        if result.returncode == 0 and version:
            return version
    except Exception:  # noqa: BLE001 — git missing, not a repo, etc.
        pass
    return "unknown"


version = get_firmware_version()
env.Append(BUILD_FLAGS=[f'-D FIRMWARE_VERSION=\\"{version}\\"'])
print(f"[git_version] FIRMWARE_VERSION={version}")
