# Project status

## Model work checkpoint — 2026-11-19

Baseline reestablished on clean `main` at `f9546c1`: all six native targets pass;
`tools\idf.cmd build size` passes (890,992-byte binary, 645,008 bytes / 42% free,
IRAM 93,635 bytes, static DRAM 51,059 bytes). Hardware configuration also builds
(797,760 bytes); its only warnings concern temporary IDF activation cleanup.
Root `sdkconfig` SHA-256: `c6ea3a17eefc9794ba4335b8ceebe572ba7f0559ab85196bc65626b69760692f`.

Private project 1168582 has not been read or changed. After the user reported
restoring access, the browser still denied `studio.edgeimpulse.com` because of
a saved permission / managed policy. Roboflow browser access was also denied.
Public research is complete within the five-dataset/three-model limit. See
DATASET_SELECTION.md and MODEL_SELECTION.md. Two CC-BY-4.0 datasets are queued
for audit; none is approved for training. Battery v12 has no held-out test split.
No dataset, trained model, experiment, metric, or hardware result is inferred.

Local implementation checkpoint: separate classification adapter; preserved and
strengthened FOMO metadata checks; central fine-grained/reject taxonomy; whole-
vector probability/margin rejection; physical model-validation gate off by default;
request-specific stale-result handling; metadata/missing-artifact tests; offline
COCO auditing/preparation and export checksum tools. Tests use synthetic fixtures
and fake SDKs, not trained weights. Dataset acquisition, Studio import/training,
real SDK component wiring, vector testing and on-device validation remain blocked.

Latest local milestone: `npm test` passes nine native targets, metadata negative
compile checks, absent-artifact CMake checks and three artifact-tool tests.
Four dataset-tool tests pass separately with Pillow 13.0.0. These tests are offline.
`tools\idf.cmd build size size-components` passes: **892,080 bytes**, an increase
of 1,088 bytes from this turn's baseline; **643,920 bytes (42%) remain**. Linked
image is 891,966 bytes; IRAM 93,635 and static DRAM 51,059 bytes are unchanged.
No weights are installed, so this is adapter infrastructure overhead, not a model
integration size. Preprocessing PSRAM allocations are 258,048 bytes at 96×96;
model arena, peak inference RAM and latency are unmeasured (AI_MODEL.md).
Root sdkconfig hash remains unchanged. No partition changes or physical tests.
The hardware/synthetic-camera regression also passes: **798,880 bytes** (`0xc30a0`),
737,120 bytes / 48% partition space free; physical arming gates remain closed.

Component-size analysis (bytes, default build, no generated model):

| Archive | Total | Static DRAM | Flash code | Flash data |
|---|---:|---:|---:|---:|
| main | 6,982 | 247 | 4,339 | 2,396 |
| vision | 4,072 | 3,105 | 935 | 32 |
| fsm | 2,737 | 0 | 2,513 | 224 |
| motor | 514 | 76 | 438 | 0 |

Full component output is retained in ignored `build/model-components.csv`.
Locally retained ignored artifacts: `.venv/` (Pillow tooling), native test
executables, IDF build outputs/logs/resource CSVs and prior audit backups under
`build/`, existing `node_modules/`, and original root `sdkconfig`. No dataset or
trained-model download was retained. Staged diff/credential/large-artifact checks
pass. The model work is a preparation milestone, not completed trained inference.

## Audit baseline — 2026-11-17

- Branch `main`, origin `https://github.com/brianteo929-ctrl/embedded-ai-cam-sorting-controller.git`, HEAD `db53f80`.
- Existing edits: `components/motor/{motor.c,CMakeLists.txt}` and `sdkconfig`. Originals retained locally under ignored `build/audit/`; the motor prototype is being extended. Local sdkconfig stays uncommitted.
- No repository AGENTS.md or firmware tests. `npm test` fails with “no test specified”; Node is an older upload/stream relay.
- IDF code: three-state periodic loop, one busy-wait motor on GPIO12/13/14, enabled on boot. No trigger, camera, inference, calibration, timeout, or fault handling.
- Arduino reference: OV3660-compatible AI-Thinker pin map, QVGA JPEG, AP `ESP32-CAM-MB`, MJPEG, and repetitive motor task. Not part of the IDF build.
- No Edge Impulse export, weights, dataset, or metrics. Website claims exceed implementation.
- Old `C:\Users\brian\esp-idf` is IDF 5.2; its export reported unavailable tools inside the sandbox. VS Code/build metadata select `C:\esp\v6.0.2\esp-idf` and its IDF 6.0 Python environment. The old and current toolchains must not share generated build files.
- Baseline: IDF 6.0.2 incremental build **PASS**, binary 144,800 bytes (`0x235a0`), 1 MiB app partition. Sandbox Windows DLL relocation error requires running outside this session's sandbox; installed tools work there. No reinstall/fullclean needed.
- No serial ports enumerated. Physical camera, motors, wiring, power, inference, and mechanics cannot be verified here.

## Implemented and locally verified

- Native IDF control/vision tasks, cycle-tagged queues, explicit UNHOMED/IDLE/CAPTURE/INFERENCE/ACTUATE/VERIFY_OPEN_LOOP/RESET/FAULT states, deadlines and latched recovery. No movement on boot, no assumed home, no automatic retry.
- Conservative three-bin policy rejects uncertainty and multiple objects. All nine deterministic mock scenarios feed the same production policy.
- Sequential two-axis RMT or simulation, bounded signed positions, symmetric period ramps, cancellation, and reference invalidation. Hardware output and its calibration/mock interlocks default off.
- AI-Thinker camera port with PSRAM, fresh-frame acquisition, bounded JPEG decoding and dimension checks, RGB crop/resize, mutex/frame ownership, snapshots and MJPEG on a separate image server.
- WPA2 AP at 192.168.4.1, per-boot UART-displayed credentials, bounded commands, status JSON, memory/stack/timing instrumentation. No embedded production credentials.
- External Edge Impulse interface/adapter and integration instructions; actual SDK/model unavailable. Adapter contract tested with a fake SDK only.
- GPIO/wiring, current-limit formulas, power qualification, mechanical bring-up, model workflow and learning strategy documented. Arduino/Node retained and marked historical.

## Previous milestone verification — 2026-11-17

| Check | Result |
|---|---|
| `npm test` / `python tools/test.py` | PASS: six native targets, warnings treated as errors |
| Core tests | category/threshold/ambiguity/invalid-result policy; FSM lifecycle, busy rejection, deadlines, rollover, stop/recovery; pulse limits/ramps; RGB crop/resize |
| Driver and camera contracts | PASS: real motor source with fake RMT, simulation backend, fresh-frame discard, mutex exclusion, allocation/get/decode failures, dimensions, independent JPEG copies |
| Model tests | PASS: nine deterministic mocks and fake-SDK adapter contract; **no real model validation** |
| `tools\idf.cmd build size` | PASS, ESP-IDF 6.0.2, ESP32, real camera + simulated motors |
| Default binary | 890992 bytes (`0xd9870`), 1500 KiB app partition, 42% free |
| Linker resource report | IRAM 93635 / 131072 bytes; static DRAM 51059 / 180736 bytes; total linked image 890870 bytes before bin padding |
| `tools\verify-hardware.cmd` | PASS: real RMT + synthetic camera, 797760 bytes (`0xc2c40`), 48% app space free; arming gates remain closed |
| `git diff --check` | PASS |
| Original root sdkconfig | SHA-256 compared with pre-edit backup: byte-identical |
| Physical / serial tests | NOT RUN: no COM ports enumerated; no repeat hardware attempts |

Logs and binaries are local under ignored `build/`. Resource figures are build/link evidence, not available runtime heap or proven stack margins. Runtime measurements are exposed in status for the bench operator.

Build repairs were evidence-based: selected the configured IDF 6.0.2 environment instead of inherited IDF 5.2; isolated the mixed old build cache; added explicit component dependencies; corrected IDF HTTP/IP APIs and its empty RMT configuration initializer. No tool reinstall or fullclean. Host test compilation caught and fixed a test-variable collision with the C library's `gets` symbol.

## Remaining blockers

| Area | Exact missing evidence / work |
|---|---|
| Software integration | Default and hardware-profile builds work locally. Actual exported EI SDK compatibility with IDF 6.0.2 cannot be checked until supplied; no firmware boot/network timing claims |
| Model/data | Browser permissions for Roboflow and existing EI project 1168582; audited licensed images; trained classification (preferred baseline) or FOMO C++ export; exact input/labels/version/checksum, held-out metrics and ESP32 profile |
| Hardware | No serial device; board condition after possible buck damage, sensor/flash/PSRAM identity, driver IC/Rs, motor phase ratings, VREF, boot-pin levels, cooling, and supply capacity unverified |
| Sensors | No validated object/home/limit sensors. Manual trigger/reference only; VERIFY is a timed settle, not feedback |
| Mechanical | Plate layout, gearing, microstep settings, reference marks, safe bin coordinates/path, release mechanism, load/holding behavior, repeatability and guarding |

The 12 V / 1 A adapter is not qualified for two unknown NEMA17 motors; both draw holding current after Arm even though stepping is sequential. NRF24L01 and rain sensor remain unused. Follow BRING_UP.md before enabling physical output.

## Git delivery

Implementation commit: `7227143` (`implement safe sorter pipeline`). Branch `main` and configured origin are retained as requested. The dependency-hygiene follow-up removes 709 previously tracked `node_modules` files and the generated root sdkconfig from the index only; their local copies remain intact. Original motor/config snapshots remain locally in `build/audit`. The final handoff records commit IDs and push verification; no force-push or history rewrite is used.
