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
