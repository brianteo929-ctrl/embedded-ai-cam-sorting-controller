# Wiring and power

This is a **qualified bench plan**, not measured wiring. Confirm the exact ESP32-CAM revision, driver IC, carrier pin labels, sense resistors, and motor phase ratings before connection. No microSD card. Firmware defaults to simulated motors; do not flash the historical Arduino prototype for bring-up.

## GPIO feasibility

| ESP32 GPIO / header | Assignment | Constraint |
|---|---|---|
| 14 | Motor 1 STEP | Former SD CLK; add 10 kΩ to ground |
| 13 | Motor 1 DIR | Former SD D3; driver input only |
| 2 | Motor 2 STEP | Boot strap / SD D0; add 10 kΩ to ground, must stay low for UART download |
| 12 | Motor 2 DIR | Flash-voltage strap / SD D2; add 10 kΩ to ground, **never pull high during reset** |
| 15 | Shared active-low ENABLE | SD CMD / boot-log strap; add 4.7 kΩ to **3.3 V**, verify both drivers stay disabled during reset |
| 4 | Unused | Flash LED / SD D1; not a free sensor pin without LED-circuit analysis |
| 1 / U0T | UART TX | Preserve programming and logs |
| 3 / U0R | UART RX | Preserve programming |
| 0 | Camera XCLK / boot strap | Ground only during flashing, release for boot; never use for motors |
| 16 | PSRAM connection | Not available despite appearing on some headers |
| 5,18,19,21,36,39,34,35 | Camera D0–D7 | Reserved; 34–39 are input-only |
| 22,23,25 | Camera PCLK,HREF,VSYNC | Reserved |
| 26,27 | Camera SCCB data/clock | Reserved I²C-like configuration bus |
| 32 | Camera power-down | Reserved |
| 6–11,17 | Flash / module memory | Not user GPIO on this board |
| 33 | Board status LED, typically unexposed | Not part of the header plan |
| 5 V,3V3,GND | Power | Power pins, not GPIO; labels/revisions must be checked |

The pin validator only allows STEP on 2/14, DIR on 12/13, and ENABLE on 15, with unique pins. Axis swaps within those roles are supported. This changes the old STEP12/DIR13/ENABLE14 prototype: two axes need four signals plus a boot-safe shared disable. Firmware cannot replace the external resistors during reset.

GPIO12 low selects the normal 3.3 V flash supply; pulling it high can prevent boot. GPIO15 high preserves ROM logging. Confirm actual levels with drivers connected and VMOT off before applying motor power. These strap constraints come from the [ESP32 datasheet, strapping section](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf). The camera map is retained from this repository's working-reference prototype; the board itself has not been inspected.

Only one conflicted header GPIO remains. An object sensor plus two independent home switches cannot fit this allocation while preserving camera, UART, and LED. A secondary motor/sensor controller is the clean expansion path; an I²C expander also needs two usable bus pins, so it is not a drop-in answer here. Do not attach the rain sensor or NRF24L01 to reserved pins. Neither is required.

## Power and driver connections

```text
12 V adapter + ----+---- driver 1 VMOT   [bulk capacitor to GND nearby]
                  +---- driver 2 VMOT   [bulk capacitor to GND nearby]
                  +---- LM2596 IN+ -> adjusted 5.0 V OUT+ -> ESP32-CAM 5 V
adapter - -------- common ground: both driver grounds, buck IN-/OUT-, ESP GND
ESP 3V3 ---------- driver logic inputs / pulls (never VMOT)
```

Measure the buck's output and polarity **before** connecting the ESP32. Keep motor currents out of thin logic jumper paths. Use short VMOT wiring and secure screw terminals. Do not parallel USB/MB 5 V with the buck output unless the board's power isolation is verified. During programming, disconnect the buck's 5 V feed and VMOT; use USB logic power only. The prior commit mentioned possible buck-related board damage: verify the board independently first.

At each driver, place a correctly polarized 100–470 µF electrolytic, rated at least 25 V for the 12 V rail, across VMOT/GND close to the pins. The carrier guidance recommends at least 47 µF to reduce LC spikes; capacitor placement matters. Heatsinks must not short adjacent pads; provide cooling and check temperature. Never plug/unplug a motor or swap driver carriers while energized. See [DRV8825 carrier wiring](https://www.pololu.com/product/2133/) and [A4988 carrier wiring](https://www.pololu.com/product/1182).

Both drivers: STEP/DIR as above, common ENABLE to GPIO15 with the external pull-up. Hold RESET and SLEEP high at 3.3 V as required by the actual carrier; do not leave them floating. Tie mode pins low for full steps initially. DRV8825 has no separate VDD input on the usual carrier; A4988 requires logic VDD (use 3.3 V). The corresponding carrier pin can instead be FAULT on DRV8825: do not assume drop-in wiring or drive FAULT as a supply. Read the carrier labels, not its color.

For bench power-up, establish logic power first and measure ENABLE high, then connect VMOT with drivers disabled. For shutdown, remove VMOT first, wait for capacitors to discharge, then remove logic power. RESET/SLEEP must follow the verified logic rail so missing logic power cannot leave a DRV8825 awake with ENABLE pulled low internally. Do not rely on an unpowered ESP32 GPIO to keep a motor off; verify the carrier's pull resistors and power-loss behavior.

With power disconnected, use the multimeter's resistance setting to identify the two independent coil pairs. One pair goes to A1/A2, the other to B1/B2. Do not infer pairs from colors or the six-pin cable connector. Reversing one pair reverses direction; mixed pairs cause vibration. Software direction inversion is configurable. No powered resistance measurements.

## Current limit and the 12 V / 1 A supply

For the actual IC and sense resistance `Rs` in ohms:

- DRV8825: `I_limit = VREF / (5 × Rs)`.
- A4988: `I_limit = VREF / (8 × Rs)`.

Use the motor's rated phase current and the carrier's cooling limit to choose a conservative limit; then calculate VREF. There is no universal VREF for NEMA17 motors or clone carriers. Read the resistor markings/datasheet, measure VREF to driver GND with an insulated probe, and follow the carrier's power/enable procedure. Full-step coil current and supply input current are not the same as the configured limit. Formulas: [TI DRV8825 datasheet](https://www.ti.com/lit/ds/symlink/drv8825.pdf), [Allegro A4988 datasheet](https://www.pololu.com/file/download/A4988.pdf?file_id=0J450).

The supplied adapter provides only 12 W nominal, also feeding the ESP32 through the buck. It is **not qualified for these two unknown motors**. Sequential motion reduces simultaneous acceleration, but shared ENABLE holds **both** motors energized after arming. It does not halve holding-current demand. Validate one driver/motor first, then measure both at rest and under load, checking adapter voltage droop, ESP brownout, motor/driver temperature, and missed steps. Use a suitably rated supply if measured demand plus margin exceeds capacity. Do not choose a replacement current rating until motor data and measurements are known.

## Position and safety

Positions are pulse estimates, not sensor measurements. Boot, stop, fault, reset, power loss, or mechanical slip invalidate the reference. Arm may move a motor to its electrical holding position. Align the disconnected mechanism beforehand, then confirm the reference marks after arming; do not force a powered shaft. Clear fault does not home or re-enable anything.

Hardware mode also requires `SORTER_MOTION_CALIBRATED=y`; mock-to-hardware testing requires `SORTER_ALLOW_MOCK_MOTION=y`. These are operator attestations, not electrical measurements. Start with unloaded motors and small calibrated targets. The HTTP Stop is software control, not an emergency-stop circuit; provide an accessible motor-power disconnect and contain the plate so loss of torque cannot cause harm. Hazardous classification is a label only: never prototype with real hazardous waste.
