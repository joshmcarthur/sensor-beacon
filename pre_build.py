import os
import shutil

Import("env")  # type: ignore
menv = env  # type: ignore

env_name = menv["PIOENV"]
libdeps = f".pio/libdeps/{env_name}/"
mc_dir = libdeps + "MeshCore/"

ed_dir = libdeps + "ed25519/"
if not os.path.exists(ed_dir) and os.path.isdir(mc_dir + "lib/ed25519"):
    shutil.copytree(mc_dir + "lib/ed25519", ed_dir)
    with open(ed_dir + "library.properties", "w", encoding="utf-8") as f:
        f.write("name=ed25519\nversion=1.0.0\n")

nrf52_api = mc_dir + "lib/nrf52/s140_nrf52_7.3.0_API/include"
if os.path.isdir(nrf52_api):
    menv.Append(CPPPATH=[nrf52_api, nrf52_api + "/nrf52"])
