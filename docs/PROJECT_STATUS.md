# Project status

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

## Verification evidence

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
| Model/data | Trained FOMO C++ export, exact labels/input/preprocessing agreement, version/checksum, dataset provenance, held-out metrics, tensor-arena/latency/RAM measurements |
| Hardware | No serial device; board condition after possible buck damage, sensor/flash/PSRAM identity, driver IC/Rs, motor phase ratings, VREF, boot-pin levels, cooling, and supply capacity unverified |
| Sensors | No validated object/home/limit sensors. Manual trigger/reference only; VERIFY is a timed settle, not feedback |
| Mechanical | Plate layout, gearing, microstep settings, reference marks, safe bin coordinates/path, release mechanism, load/holding behavior, repeatability and guarding |

The 12 V / 1 A adapter is not qualified for two unknown NEMA17 motors; both draw holding current after Arm even though stepping is sequential. NRF24L01 and rain sensor remain unused. Follow BRING_UP.md before enabling physical output.

## Git delivery

Implementation commit: `7227143` (`implement safe sorter pipeline`). Branch `main` and configured origin are retained as requested. The dependency-hygiene follow-up removes 709 previously tracked `node_modules` files and the generated root sdkconfig from the index only; their local copies remain intact. Original motor/config snapshots remain locally in `build/audit`. The final handoff records commit IDs and push verification; no force-push or history rewrite is used.
