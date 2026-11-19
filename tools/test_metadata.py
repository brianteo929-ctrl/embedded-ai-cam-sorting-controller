"""Negative compilation checks against fake SDK metadata, never model validation."""
import os
from pathlib import Path
import shutil
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / "build" / "host"
out.mkdir(parents=True, exist_ok=True)
cxx = os.environ.get("CXX") or shutil.which("g++") or shutil.which("clang++")
if not cxx:
    raise SystemExit("A native C++ compiler is required")
cases = [
    ("inference_ei.cpp", ["EI_CLASSIFIER_OBJECT_DETECTION=0"], "camera object-detection"),
    ("inference_ei.cpp", ["EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER=99"], "requires FOMO"),
    ("inference_ei_classification.cpp", [], "camera classification export"),
    ("inference_ei_classification.cpp", ["EI_CLASSIFIER_OBJECT_DETECTION=0", "EI_CLASSIFIER_RESIZE_MODE=3"], "Fit shortest axis"),
    ("inference_ei_classification.cpp", ["EI_CLASSIFIER_OBJECT_DETECTION=0", "EI_CLASSIFIER_PROJECT_ID=1"], "project 1168582"),
]
for source, defines, expected in cases:
    result = subprocess.run([cxx, "-std=c++11", "-Itests/fakes", "-Icomponents/fsm", "-Icomponents/vision",
                             *["-D" + x for x in defines], "-c", "components/vision/" + source,
                             "-o", str(out / "metadata-negative.o")], cwd=root, capture_output=True, text=True)
    if result.returncode == 0 or expected not in result.stderr:
        raise SystemExit(f"Unexpected metadata check result for {source}: {result.stderr}")
print("PASS metadata rejection: wrong output type, non-FOMO layer, resize mismatch, wrong project")
cmake = shutil.which("cmake")
if not cmake:
    raise SystemExit("CMake is required to verify missing-model and simulation configuration")
for backend in ("MOCK", "EI", "EI_CLASSIFICATION"):
    script = out / f"cmake-{backend}.cmake"
    script.write_text(f'set(PROJECT_DIR "{out.as_posix()}/absent-model")\n'
                      f'set(CONFIG_SORTER_INFERENCE_{backend} ON)\n'
                      'function(idf_component_register)\nendfunction()\n'
                      f'include("{root.as_posix()}/components/vision/CMakeLists.txt")\n')
    result = subprocess.run([cmake, "-P", str(script)], capture_output=True, text=True)
    if backend == "MOCK":
        assert result.returncode == 0, result.stderr
    else:
        assert result.returncode != 0 and "Edge Impulse export missing" in result.stderr, result.stderr
print("PASS component configuration: simulation without artifacts, missing FOMO/classification export fails closed")
