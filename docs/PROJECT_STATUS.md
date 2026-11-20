# Project status

## USB camera bring-up checkpoint - 2026-11-20

Started from clean `main` and matching `origin/main` at
`1bb36480ff0a6b836aecd222221bbe11a00b3e2f`. Windows PnP identified
USB-SERIAL CH340 (COM3); exclusive serial open succeeded. This is the observed
port for this session only. The user reports ESP32-CAM seated in its MB programmer,
USB power only, and no drivers, motors, VMOT, buck, external supply, radio, rain
sensor or external GPIO wiring connected. No motion command was issued.

Tool-observed results:

- esptool 5.3.1 entered the bootloader automatically, identified ESP32-D0WD-V3
  revision 3.1, 40 MHz crystal, 4 MB flash (manufacturer 0x5e / device 0x4016).
  The baseline and subsequent logging-only diagnostic build flashed with all
  written-region hashes verified and automatic RTS reset. No erase-all was used;
  existing partition layout and root sdkconfig were preserved.
- Boot: ESP-IDF 6.0.2, DIO flash at 40 MHz, reset code 0x1 POWERON_RESET after
  intentional reset. Two bounded 25-second observations showed no brownout,
  watchdog or boot loop. This is a short boot check, not a soak test.
- PSRAM: 8 MB detected at 40 MHz, 4 MB mapped into the heap; memory test OK.
  The virtual-address warning explicitly reports the mapping limit. Runtime CPU
  is **160 MHz**, unlike the earlier EI target's 240 MHz profile; future model
  latency comparisons must account for this. No frequency setting was changed.
- Camera: PID 0x3660, detected OV3660 at SCCB 0x3c, camera init `ESP_OK`,
  15,360-byte PSRAM framebuffer allocated. Each observation logged two initial
  `NO-SOI - JPEG start marker missing` warnings. No sensor/pin change was made.
  QVGA JPEG is configured; actual JPEG dimensions/decoding await `/capture`.
- Latest diagnostics: `state=unhomed armed=0 referenced=0 motors=simulated`,
  `backend=mock model=simulation-no-model`. Motor GPIO configuration is compiled
  out in this build; electrical waveforms were not measured.
- AP/DHCP startup reported at 192.168.4.1; both HTTP servers reported ready on
  ports 80/81. Initial free internal memory **128,523 bytes**, free PSRAM
  **3,913,904 bytes**. These are observed boot snapshots, not peak-load margins.
- Client association, HTTP status, JPEG decoding and framing: **pending user
  verification**. No user-observed HTTP success has yet been reported.

Flashed diagnostic artifact provenance: based on `1bb3648` plus the `main/main.c`
and `main/web.c` boot-log additions in this milestone. The incremental build still
reports app version `1bb3648`; that string alone does not identify the changed
artifact. Binary SHA-256:
`d83b1e7f064a02e2840b1aab1f30ab4c94cba7c3e83dadfe009fe8fd26a25c59`;
ELF SHA-256:
`4739c82cc3fa106a26783e226300915a6028601480cdcb0454181f43f0337c96`.
Serial logs were redacted before writing to ignored `build/`; AP passwords and
device MACs are not included in committed evidence.

Capture workflow implemented in `tools/data/capture.py`: original JPEG validation,
same-host port-81 redirect, bounded requests, exact deduplication, no-overwrite
filenames, whole-object/session split checks, a collection lock and atomic CSV
replacement. A real Windows sharing violation exposed during testing led to a
maximum of three short rename attempts for Windows errors 32/33 only; persistent
locks preserve the old CSV and require inspection. Fifteen focused offline tests
pass, including a loopback HTTP server and interruption before/after replacement.

Milestone regressions: `npm test` and six dataset tests pass. Default incremental
build/size/component analysis: **892,368 bytes**, **643,632 bytes / 42% free**;
IRAM 93,635 and static DRAM 51,059 bytes. Hardware-profile compilation (never
flashed) passes at **799,152 bytes**, **736,848 bytes / 48% free**. No motor/model
authorization gate, GPIO assignment, partition or root sdkconfig change occurred.

Pilot: **0 images**, no captures directory at stage start. Proposed `session01`
training-only pilot and operator commands are in DATA_COLLECTION.md. First
external action is phone association and HTTP/JPEG verification using the locally
displayed per-boot password. The development computer's network is unchanged.
Original data collection and EI training remain gated on this pilot; no model
accuracy, on-device inference or physical motor behavior is claimed.

## Dataset audit checkpoint - 2026-11-19

Browser access and GitHub sign-in work. Existing private Edge Impulse project
1168582 is empty, with one label per item and no published model. Target settings
were independently verified: ESP-EYE/ESP32 240 MHz, custom AI-Thinker ESP32-CAM,
512 KB RAM, 512 KB ROM, 1000 ms. No second project, Keys access or API token was used.

Both selected CC-BY-4.0 COCO exports downloaded successfully. Every image decoded:
10,464 garbage images and 753 battery images. Full checks, hashes, raw class/split
counts and sampled visual findings are in DATASET_AUDIT.md and `docs/reports/`.
Garbage: 17 out-of-bounds boxes, 2,268 tiny boxes, 177 overlapping box pairs and
5,064 possible cross-split near pairs; no glass test images. Stock-watermarked
photos leave upstream rights unresolved. Battery: text-region annotations rather
than whole-object boxes, 40 tiny boxes, 1,067 possible cross-split near pairs and
no test split. Four inspected cross-split battery pairs repeat physical objects.
Near-pair counts are heuristics; garbage sampling also found false positives.
Neither source provides verified empty-plate/unknown rejection examples.

The converter now leaves zero/multiple-annotation labels unset; it cannot invent
background or multiple-item labels from box count. Preparation requires waste and
all three reject classes, each in training/validation/test. Six offline dataset
fixture tests pass. Deterministic review sheets and aggregate reports are
reproducible. Dataset acceptance correctly fails; no images were auto-approved.

**Current external blocker: accepted original data**, including upstream rights,
object/session-separated groups and plate/reject imagery. See DATA_COLLECTION.md
for the exact capture and metadata procedure. Both bounded audits are complete;
no raw upload, impulse creation, training or real model export is justified yet.
The existing classification/FOMO adapters and simulated motors remain unchanged.

Public EI candidate 676802 inspected directly: grayscale MobileNetV2 96x96 0.35,
10 classes, 13 epochs, learning rate 0.0005. Its int8 validation accuracy is 81.6%;
Raspberry Pi 5 EON RAM-optimized profile reports 214.6K RAM, 619.9K flash, 5 ms.
These are source-project results, not our measurements or an ESP32 profile. It
exceeds the flash budget and lacks verified source/reject coverage; not adopted.

Local artifacts retained intentionally: the two archives/extracted datasets and
full audit manifests under ignored `datasets/`, original downloads in Downloads,
contact sheets and regression logs under ignored `build/`, and the existing
Python environment/build outputs. No dataset image, archive or credential enters
Git. Root sdkconfig and partition table are unchanged. No physical tests occurred.

Audit-milestone verification: `npm test` passes all nine native targets, metadata
and absent-artifact checks, and three model-tool tests; six dataset-tool tests pass.
Incremental `tools\idf.cmd build size size-components` passes at 892,080 bytes,
643,920 bytes / 42% app space free, static DRAM 51,059 bytes. The hardware-profile
compile passes at 798,880 bytes, 737,120 bytes / 48% free. No model is installed;
these sizes match the preparation milestone. Full logs remain in `build/`.

## Model work checkpoint — 2026-11-19

Baseline reestablished on clean `main` at `f9546c1`: all six native targets pass;
`tools\idf.cmd build size` passes (890,992-byte binary, 645,008 bytes / 42% free,
IRAM 93,635 bytes, static DRAM 51,059 bytes). Hardware configuration also builds
(797,760 bytes); its only warnings concern temporary IDF activation cleanup.
Root `sdkconfig` SHA-256: `c6ea3a17eefc9794ba4335b8ceebe572ba7f0559ab85196bc65626b69760692f`.

At the earlier preparation milestone, browser policy blocked project and dataset
access. That access blocker is resolved by the newer checkpoint above. Public
research remains within five datasets/three models; two candidates were audited
without approving either for training. No trained application model exists.

Local implementation checkpoint: separate classification adapter; preserved and
strengthened FOMO metadata checks; central fine-grained/reject taxonomy; whole-
vector probability/margin rejection; physical model-validation gate off by default;
request-specific stale-result handling; metadata/missing-artifact tests; offline
COCO auditing/preparation and export checksum tools. Tests use synthetic fixtures
and fake SDKs, not trained weights. Acquisition is now complete. Studio import/training, real SDK component wiring
and vector tests await accepted data and an actual trained export; on-device
validation additionally requires a board.

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
`build/`, existing `node_modules/`, and original root `sdkconfig`. At that earlier milestone no dataset or
trained-model download was retained; the newer checkpoint lists current artifacts. Staged diff/credential/large-artifact checks
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
| Model/data | Browser access works. Both audited sources fail acceptance; original rights-cleared plate images, reject cases and object/session identity are needed (DATA_COLLECTION.md). Then train in existing project 1168582, export and verify exact metadata/checksum, held-out metrics and ESP32 profile |
| Hardware | USB/COM3 flashing, OV3660 initialization and PSRAM boot test now observed above. Client HTTP/JPEG verification pending. Driver IC/Rs, motor phase ratings, VREF, boot-pin levels, cooling and external supply remain unverified; no external motor wiring connected |
| Sensors | No validated object/home/limit sensors. Manual trigger/reference only; VERIFY is a timed settle, not feedback |
| Mechanical | Plate layout, gearing, microstep settings, reference marks, safe bin coordinates/path, release mechanism, load/holding behavior, repeatability and guarding |

The 12 V / 1 A adapter is not qualified for two unknown NEMA17 motors; both draw holding current after Arm even though stepping is sequential. NRF24L01 and rain sensor remain unused. Follow BRING_UP.md before enabling physical output.

## Git delivery

Implementation commit: `7227143` (`implement safe sorter pipeline`). Branch `main` and configured origin are retained as requested. The dependency-hygiene follow-up removes 709 previously tracked `node_modules` files and the generated root sdkconfig from the index only; their local copies remain intact. Original motor/config snapshots remain locally in `build/audit`. The final handoff records commit IDs and push verification; no force-push or history rewrite is used.
