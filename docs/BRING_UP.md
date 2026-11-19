# Hardware bring-up and acceptance

No serial device was available during this implementation. All steps below are **pending physical verification**. Use ordinary harmless test objects only.

1. Disconnect motors and VMOT. Inspect the possibly damaged ESP32-CAM and buck. Measure buck output at 5.0 V with correct polarity before connecting it. Check the board's flash/PSRAM capacity and actual sensor (history says OV3660; older README said OV2640).
2. Use the MB/USB interface alone for logic power. Run `tools\idf.cmd -p COMx flash monitor` with the observed port, GPIO0 low for upload and released for reset as the adapter requires. Firmware is in `build/sorter/`, not the old root build output. Never select a guessed COM port.
3. Confirm PSRAM/camera initialization and the `WEB` log. Join `ESP32-CAM-MB` using the **new password printed at each boot**. Open `http://192.168.4.1/`. Expect UNHOMED, simulated motors, mock inference, camera JPEG. Confirm `/capture` and the optional preview on port 81. Close preview while comparing inference images.
4. With motor wires still disconnected, Arm → Confirm zero → Capture and sort. Test each of the nine mock scenarios. The first three should complete the estimated move/settle/return cycle; empty/low/unknown/multiple/conflict should remain at zero; error should latch FAULT. Repeat trigger during motion: it must be rejected, not queued into a later cycle. Stop during capture/motion must invalidate reference; clear fault must return to UNHOMED. No physical position verification is claimed.
5. Test camera failure with power off before unplugging the ribbon; on next boot the UI should report camera unavailable and refuse arming. Verify watchdog/brownout logs and boot pin levels. Do not hot-plug a camera ribbon.
6. Follow WIRING.md: confirm IC and carrier orientation, coil pairs, sense resistors, phase ratings, external pull resistors, capacitor polarity and VREF. Start with one disabled driver and no motor, then power down and attach one unloaded motor. Use a common ground and isolated logic/USB power arrangement. Establish a readily accessible motor-power disconnect.
7. Configure the physical motor backend with extremely small, mechanically safe test coordinates and slow periods. The firmware additionally requires calibrated-motion and mock-motion bench acknowledgments. Begin with only one driver's VMOT powered; keep the other disconnected from VMOT. Check boot remains disabled and arming enables holding without unexpected rotation. Do not force an energized shaft.
8. Verify STEP/DIR/ENABLE polarity and pulse counts with a scope/logic analyzer if available. A multimeter cannot validate pulse timing. Confirm direction, cancellation and estimated return under no load; then test the second axis separately. Attach/change motors only with power removed and capacitors discharged. A4988/DRV8825 carrier swaps require rewiring review.
9. Measure adapter current/voltage and driver/motor temperatures with both enabled, then moving sequentially. Both hold current after Arm. Qualify or replace the 12 V / 1 A supply before coupled motion. Confirm no brownouts or missed steps under the real plate load.
10. Establish physical zero marks, steps/revolution, microstep settings and gear ratio. `pulses = angle / 360 × full_steps_per_rev × microstep_factor × gear_ratio`. NEMA17 is a frame size, not proof of 200 steps/rev. Set each bin's two coordinates and axis bounds. Verify the sequential path, return path, holding requirements and settling time. Default coordinates are examples. A physical release/ejection mechanism and its timing remain mechanical work.
11. Add the actual exported model using MODEL_INTEGRATION.md. Test held-out images and live harmless objects with simulated motors. Record accuracy/confusion, false accepts, capture/preprocess/inference latency, free internal RAM/largest block/PSRAM and stack margins. Only enable model-controlled physical output after those checks.
12. For unattended operation, add real home/limit/object sensors and electrical stop circuitry, then implement measured homing and verification. Until then, this is a manually triggered open-loop prototype requiring supervision and re-reference after every fault, slip, reset, or loss of torque.

## Inference-only session after a verified export is installed

Before training, follow [DATA_COLLECTION.md](DATA_COLLECTION.md) for owned images
of the actual plate, including rejection cases. Browser access is available; the
current blocker is accepted training data, not a serial device or login. Camera
collection can use the existing mock-inference build with VMOT disconnected.

1. Keep **VMOT disconnected**, `SORTER_MOTOR_HARDWARE=n` and
   `SORTER_MODEL_VALIDATED=n`. Follow AI_MODEL.md to install/select the adapter.
   `tools\idf.cmd build size` must pass before flashing.
2. Identify the serial port; use `tools\idf.cmd -p COMx flash monitor`. Record the
   logged backend/project/deployment version, PSRAM and camera status.
3. Join the AP with the UART password; open `http://192.168.4.1/`. Confirm simulated
   motors. Compare a color chart and crop grid from `/capture` with Studio's input.
   Close the port-81 stream for repeatable capture/inference timing.
4. Collect snapshots into ignored `captures/SESSION/` with object/session metadata.
   Reserve whole objects/sessions for test. Include empty plate, unfamiliar harmless
   objects, pairs, orientations and lighting changes, and safe special-waste
   replicas. Do not stage damaged batteries, pressurized cans or broken lamps.
5. Arm → Confirm zero → Capture and sort with simulated motors. Record decisions,
   capture/inference times, heap/largest block, PSRAM and stack margins. Compare
   probabilities in known-vector tests. Confirm negatives reject and errors fault.
   Evaluate held-out camera sessions without tuning thresholds on them.
6. Stop during capture/inference; confirm late results cannot change the fault.
   Recover only after vision returns, then re-arm/reference. Reset must return
   UNHOMED with no automatic trigger. Keep physical output disabled until model,
   power and mechanical acceptance pass; classification does not prove item count.

## Acceptance record to fill on the bench

| Evidence | Actual value |
|---|---|
| Board/sensor/flash/PSRAM and firmware commit | pending |
| Driver IC, carrier Rs, motor phase rating, chosen VREF | pending per axis |
| Supply at idle / both holding / moving; temperatures | pending |
| Measured STEP counts/timing and direction | pending |
| Bin/zero coordinates and repeated-cycle error | pending |
| Model version/checksum and held-out metrics | pending |
| Heap/largest block/PSRAM/stack and timing | pending |
| Stop/fault/recovery/brownout tests | pending |
