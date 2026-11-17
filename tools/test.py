"""Compile and run portable firmware logic without ESP-IDF."""
import os
from pathlib import Path
import shutil
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
out = root / "build" / "host"
out.mkdir(parents=True, exist_ok=True)
cc = os.environ.get("CC") or shutil.which("gcc") or shutil.which("clang")
if not cc:
    raise SystemExit("Install a native C compiler or set CC (not the Xtensa compiler).")
groups = {
    "core": (["tests/test_core.c", "components/fsm/fsm.c", "components/fsm/decision.c", "components/fsm/motion.c", "components/fsm/image.c"], []),
    "motor": (["tests/test_motor.c", "components/motor/motor.c", "components/fsm/motion.c"], []),
    "simulation": (["tests/test_motor.c", "components/motor/motor.c", "components/fsm/motion.c"], ["-DCONFIG_SORTER_MOTOR_HARDWARE=0"]),
    "inference": (["tests/test_inference.c", "components/vision/inference_mock.c", "components/fsm/decision.c"], []),
    "camera": (["tests/test_camera.c", "components/vision/camera_service.c"], []),
    "ei_contract": (["tests/test_ei_adapter.cpp", "components/vision/inference_ei.cpp"], []),
}
for name in sys.argv[1:] or groups:
    sources, flags = groups[name]
    exe = out / (name + (".exe" if os.name == "nt" else ""))
    cpp = name == "ei_contract"
    compiler = (os.environ.get("CXX") or shutil.which("g++") or shutil.which("clang++")) if cpp else cc
    if not compiler:
        raise SystemExit("Set CXX to a native C++ compiler for the EI adapter contract test.")
    subprocess.run([compiler, "-std=c++11" if cpp else "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic", "-g",
                    "-Itests/fakes", "-Icomponents/fsm", "-Icomponents/motor", "-Icomponents/vision",
                    *flags, *sources, "-lm", "-o", str(exe)], cwd=root, check=True)
    subprocess.run([str(exe)], cwd=root, check=True)
