# Autonomous Waste Sorter

ESP32-CAM controller for a manually triggered, two-axis waste-sorting plate. Native ESP-IDF 6.0.2 + FreeRTOS; camera/JPEG preview, conservative three-category decisions, explicit state machine, and asynchronous RMT step generation.

**Current status:** firmware builds and host tests run locally. Default inference and motors are simulated. No trained Edge Impulse export is present; no board was connected for flashing or physical validation. This is an open-loop prototype, not an unattended or hazardous-waste handling system. The [portfolio concept](https://www.brianteo.dev/) describes the intended product; this repository's status and evidence are authoritative.

## Build on this Windows machine

From Command Prompt in this repository:

```bat
tools\idf.cmd build
npm test
tools\idf.cmd menuconfig
tools\idf.cmd size
```

The helper activates **`C:\esp\v6.0.2\esp-idf\export.bat`**, including the IDF Python virtual environment. No separate `venv` activation is needed. Set `SORTER_IDF_PATH` to another **6.0.2** installation if necessary. The older `C:\Users\brian\esp-idf` installation is 5.2 and does not match the current configuration/tools. In PowerShell use `.\tools\idf.cmd build`.

The helper keeps the user's root `sdkconfig` intact and uses `build/sdkconfig.sorter`, `sdkconfig.defaults`, and the isolated incremental output folder `build/sorter`. First configuration downloads pinned official camera dependencies; subsequent builds are incremental. Do not routinely delete build folders or run fullclean. This Codex sandbox required out-of-sandbox toolchain execution because Windows returned DLL relocation errors; that was not a missing-tool installation.

Equivalent manual setup:

```bat
call C:\esp\v6.0.2\esp-idf\export.bat
cd /d C:\Users\brian\Documents\embedded-ai-cam-sorting-controller
idf.py -B build/sorter -D SDKCONFIG=build/sdkconfig.sorter build
```

With an identified board and validated power/wiring:

```bat
tools\idf.cmd -p COMx flash monitor
```

Replace COMx with the actual port. Do not flash the obsolete root build output. Defaults assume 4 MB flash and working PSRAM; verify those against the board first.

## Operation

1. Read [wiring](docs/WIRING.md) and [bring-up](docs/BRING_UP.md). Keep VMOT disconnected for initial camera/UI checks.
2. Read UART at 115200 baud. Join `ESP32-CAM-MB` with its newly generated password, printed at each boot. Open `http://192.168.4.1/`.
3. The initial state is UNHOMED. Arm, then Confirm zero only when both physical axes are at their reference marks (simulation requires the same workflow).
4. Select a deterministic mock scenario and Capture and sort. Single accepted objects follow move → open-loop settle → estimated return to zero. Ambiguous results do not move.
5. Stop disables torque and invalidates position. Clear fault returns to UNHOMED; arm and reference again. No sensor-driven homing is implemented.

The page displays the inference, motor and camera modes. Physical motion requires `SORTER_MOTOR_HARDWARE` and `SORTER_MOTION_CALIBRATED`; mock-driven physical bench tests additionally require `SORTER_ALLOW_MOCK_MOTION`, and real-model control requires `SORTER_MODEL_VALIDATED`. None is enabled by default. Example bin positions are not a validated mechanism design.

| Endpoint | Purpose |
|---|---|
| `GET /` | Controls and status |
| `GET /api/status` | JSON state, mode, fault, decision, estimated position, heap/stack/timing |
| `POST /api/command` | Plain command: arm, home, trigger, stop, recover; requires page token |
| `GET /capture` | Independent JPEG snapshot |
| `GET /stream` | Redirect to separate MJPEG server at port 81 |

Mock trigger suffixes: `trigger-recyclable`, `trigger-organic`, `trigger-hazardous`, `trigger-empty`, `trigger-low`, `trigger-unknown`, `trigger-multiple`, `trigger-conflict`, `trigger-error`. A 202 response means queued: inspect status for the actual result. The software Stop is not a hardware emergency stop.

## Verification and layout

```bat
python tools\test.py
python tools\test.py motor camera
tools\verify-hardware.cmd
```

Native tests compile the actual C/C++ modules with GCC/G++ (or CC/CXX overrides). They cover classification, FSM/deadlines/rollover/recovery, motion planning, hardware-driver contracts through fake RMT, simulated motion, all mock scenarios, camera ownership/failure paths and the EI adapter contract through a **fake SDK**. These are not physical tests or validation of a real trained model. The alternate IDF build compiles real RMT plus a synthetic camera, with physical arming gates still closed; it does not flash anything.

| Directory | Role |
|---|---|
| `main/` | Control/vision tasks, HTTP/AP, Kconfig |
| `components/fsm/` | Portable FSM, classification, motion planning, preprocessing |
| `components/motor/` | Sequential non-blocking RMT or simulated driver |
| `components/vision/` | Camera ownership, mock inference, external EI adapter |
| `tests/`, `tools/` | Native regression and Windows IDF helpers |
| `PlatformIOfolder/`, `server/` | Historical Arduino and Node references; not production dependencies |

Image classification is the selected next baseline for one centered item; FOMO remains a separate supported adapter. Neither has installed trained weights. Browser access to the existing Edge Impulse project works. Two Roboflow candidates were downloaded and audited; neither passed training acceptance. The battery boxes describe chemistry text, repeated objects cross splits, and the garbage source includes stock-watermarked images with unresolved upstream rights. Real empty-plate and unfamiliar-object examples are still needed. See the [audit](docs/DATASET_AUDIT.md) and [capture procedure](docs/DATA_COLLECTION.md).

Real label routing is centralized in `components/fsm/waste_labels.def`; reject classes and ambiguous probabilities never default to a bin. Thresholds remain provisional and real-model physical motion is gated off.

See [current evidence and blockers](docs/PROJECT_STATUS.md), [AI model/export workflow](docs/AI_MODEL.md), [bounded dataset selection](docs/DATASET_SELECTION.md), [dataset audit tools](docs/DATASET_AUDIT.md), and [architecture](docs/ARCHITECTURE.md). Real inference performance, calibrated positions, supply capacity, camera operation, motor behavior and successful waste delivery remain to be measured on the bench.
