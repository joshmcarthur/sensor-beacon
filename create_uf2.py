# PlatformIO post-build: convert firmware.hex -> firmware.uf2 for drag-and-drop
# flashing (double-tap reset, copy UF2 to the XIAO drive).

import glob
import os

Import("env")

UF2_FAMILY = "0xADA52840"  # Adafruit nRF52840 bootloader


def _find_uf2conv():
    try:
        pkg = env.PioPlatform().get_package_dir("framework-arduinoadafruitnrf52")
    except Exception:
        pkg = None
    if pkg:
        candidate = os.path.join(pkg, "tools", "uf2conv", "uf2conv.py")
        if os.path.isfile(candidate):
            return candidate

    libdeps = os.path.join(env["PROJECT_DIR"], ".pio", "libdeps")
    for path in glob.glob(os.path.join(libdeps, "*", "MeshCore", "bin", "uf2conv", "uf2conv.py")):
        if os.path.isfile(path):
            return path

    return None


def create_uf2(source, target, env):
    hex_path = str(target[0])
    uf2_path = hex_path.replace(".hex", ".uf2")
    uf2conv = _find_uf2conv()
    if not uf2conv:
        print("create_uf2: uf2conv.py not found; skipping UF2 generation")
        return

    env.Execute(
        " ".join(
            [
                '"$PYTHONEXE"',
                f'"{uf2conv}"',
                "-f",
                UF2_FAMILY,
                "-c",
                f'"{hex_path}"',
                "-o",
                f'"{uf2_path}"',
            ]
        )
    )
    print(f"create_uf2: wrote {uf2_path}")


if env.get("PIOPLATFORM") == "nordicnrf52":
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.hex", create_uf2)
