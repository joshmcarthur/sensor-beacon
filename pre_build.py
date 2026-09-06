import os
import shutil

Import("env")  # type: ignore
menv = env  # type: ignore

env_name = menv["PIOENV"]
variant_name = None

for item in menv.get("BUILD_FLAGS", []):
    if "MC_VARIANT" in item:
        variant_name = item.split("=")[1]
        variant_dir = os.path.join(
            menv["PROJECT_DIR"],
            ".pio/libdeps",
            env_name,
            "MeshCore",
            "variants",
            variant_name,
        )
        if os.path.isdir(variant_dir):
            menv.Append(CPPPATH=[variant_dir])

libdeps = os.path.join(menv["PROJECT_DIR"], ".pio/libdeps", env_name)
mc_dir = os.path.join(libdeps, "MeshCore")

ed_dir = os.path.join(libdeps, "ed25519")
if not os.path.exists(ed_dir) and os.path.isdir(os.path.join(mc_dir, "lib/ed25519")):
    shutil.copytree(os.path.join(mc_dir, "lib/ed25519"), ed_dir)
    with open(ed_dir + "library.properties", "w", encoding="utf-8") as f:
        f.write("name=ed25519\nversion=1.0.0\n")

nrf52_api = os.path.join(mc_dir, "lib/nrf52/s140_nrf52_7.3.0_API/include")
if os.path.isdir(nrf52_api):
    menv.Append(CPPPATH=[nrf52_api, os.path.join(nrf52_api, "nrf52")])

if env_name.startswith("rak3172"):
    stm32_helpers = os.path.join(
        menv["PROJECT_DIR"],
        ".pio/libdeps",
        env_name,
        "MeshCore",
        "src/helpers/stm32",
    )
    if os.path.isdir(stm32_helpers):
        menv.Append(CPPPATH=[stm32_helpers])

    stm32_lfs_src = os.path.join(
        menv["PROJECT_DIR"],
        ".pio/libdeps",
        env_name,
        "MeshCore",
        "arch/stm32/Adafruit_LittleFS_stm32",
    )
    if os.path.isdir(stm32_lfs_src):
        lfs_inc = os.path.join(stm32_lfs_src, "src")
        menv.Append(CPPPATH=[lfs_inc, os.path.join(lfs_inc, "littlefs")])
        menv.BuildSources(
            os.path.join("$BUILD_DIR", "Adafruit_LittleFS_stm32"),
            lfs_inc,
        )

    try:
        stm32_pkg = menv.PioPlatform().get_package_dir("framework-arduinoststm32")
    except Exception:
        stm32_pkg = None
    if stm32_pkg:
        subghz = os.path.join(stm32_pkg, "libraries/SubGhz/src")
        if os.path.isdir(subghz):
            menv.Append(CPPPATH=[subghz])
