# Teensy Firmware Reference

**File:** `stepperAndLinear/NeuropixelMotors/NeuropixelMotors.ino`  
**Platform:** Teensy (4.x recommended)  
**Library:** [BasicStepperDriver](https://github.com/laurb9/StepperDriver)  
**Baud rate:** 115,200 (COM15)

---

## Overview

The Teensy firmware controls two stepper motors and acts as the physical actuator interface between the Bonsai software and the stimulus delivery hardware. It receives single-character commands over USB serial and responds with status messages that Bonsai's Reports module parses to gate trial progression.

On power-up, the linear motor performs a full homing rotation (`-36000°`) to establish a known reference position.

---

## Motor Configuration

### Rotary Stepper (Object Wheel)

Selects which tactile object is presented by rotating a wheel with objects mounted at fixed angular positions.

| Parameter | Value |
|-----------|-------|
| Steps/revolution | 200 |
| RPM | 40 |
| Microsteps | 1 |
| DIR pin | 0 |
| STEP pin | 1 |
| SLEEP pin | 2 |

### Linear Motor (Whisker Probe)

Advances or retracts the selected object toward the animal's whiskers.

| Parameter | Value |
|-----------|-------|
| Steps/revolution | 200 |
| RPM | 450 |
| Microsteps | 1 |
| DIR pin | 13 |
| STEP pin | 14 |
| SLEEP pin | 15 |

---

## Pin Map

### Inputs from Bonsai (digital HIGH = command active)

| Pin | Constant | Function |
|-----|----------|----------|
| 11 | `ALUM` | Aluminum foil stimulus command |
| 20 | `ATT` | Attenuated stimulus command |
| 19 | `NON` | Non-stimulus command |
| 17 | `LFWD` | Linear motor forward |
| 12 | `interruptPin` | Hardware reset trigger (RISING interrupt) |

### Outputs to Bonsai

| Pin | Constant | Function |
|-----|----------|----------|
| 3 | `L_ST` | Linear motor status |
| 5 | `L_CATCH` | Catch signal |
| 6 | `R_NON` | Ready: non-stimulus position |
| 7 | `R_MUT` | Ready: muted position |
| 8 | `R_ALUM` | Ready: aluminum position |
| 9 | `R_OBJ` | Ready: object (evoked) position |
| 18 | `Obj4` | Object 4 indicator |
| 22 | `R_reset` | Reset complete notification |

---

## Serial Command Protocol

Commands are single ASCII characters sent from Bonsai over COM15.

### Commands → Motor actions

| Command | Object Wheel Move | Linear | Serial Response |
|---------|-------------------|--------|-----------------|
| `l` | → aluminum position (99 steps) | — | `"endT"` then `"aluminum"` then `"at Whiskers"` |
| `m` | → muted position (66 steps) | — | `"endT"` then `"aluminum silenced"` then `"at Whiskers"` |
| `s` | → muted position (66 steps) | — | `"endT"` then `"aluminum silenced"` then `"at Whiskers"` |
| `n` | → non-stim position (33 steps) | — | `"endT"` then `"non"` then `"at Whiskers"` |
| `e` | → non-stim position (33 steps) | — | `"endT"` then `"non"` then `"at Whiskers"` |
| `o` | → non-stim position (33 steps) | — | `"evoked"` then `"at Whiskers"` |
| `f` | — | Forward (to whisker position) | `"Forward"` |
| `b` | — | Retract (full home rotation: -36000°) | `"Moving back"` |

### Numeric commands (integer in mm)

Sending an integer `N` (optionally negative) rotates the linear motor by `N × 360°`.  
This is used for fine-positioning during setup.

### `r` prefix command

Sending `rN` (e.g. `r99`) rotates the object wheel stepper by exactly N steps and resets `stepperAngle` to 0. Used for manual calibration.

---

## Object Wheel Positions

The wheel holds objects at fixed angular positions. All positions are defined in steps from the home reference (0):

| Object | Steps from home | Degrees |
|--------|----------------|---------|
| obj_4 (default/home) | 0 | 0° |
| Non-stimulus | 33 | 16.5° |
| Muted/Attenuated | 66 | 33° |
| Aluminum | 99 | 49.5° |

Position is tracked as an absolute offset (`stepperAngle`) and each move command applies a delta:

```
stepper.move(target_position - stepperAngle)
stepperAngle = target_position
```

This ensures the wheel always takes the shortest path to the target regardless of prior state.

---

## Interrupt: Hardware Reset

Interrupt pin 12 is configured as `INPUT_PULLDOWN` with a `RISING` edge interrupt.

When triggered (by Bonsai sending a digital HIGH to pin 12), the `advancemotor()` ISR executes:

1. `linnear.startBrake()` — stops linear motor immediately
2. `linnear.rotate(180)` — nudges forward 180°
3. Serial print `"reset"` → Bonsai sets `At_start = true`
4. `R_reset` pin (22) driven HIGH for 20 ms → TTL pulse to Bonsai

This provides a hard-reset path that re-establishes linear motor home position mid-session if needed.

---

## Startup Sequence

```
setup()
  1. linnear.begin(450 RPM, microstep=1)
  2. stepper.begin(40 RPM, microstep=1)
  3. Serial.begin(115200)
  4. Configure all input pins as INPUT_PULLDOWN
  5. Attach RISING interrupt on pin 12 → advancemotor()
  6. Print command legend to serial
  7. linnear.enable()
  8. linnear.rotate(-36000)   ← full homing sweep
  9. linnear.disable()
```

The `-36000°` homing rotation drives the linear stage to its mechanical limit, establishing a repeatable zero reference.

---

## Loop Behavior

Each `loop()` iteration:

1. Check `Serial.available()`:
   - If digit or `-`: parse as integer mm command → rotate linear
   - If `r`: parse remainder as step count → rotate stepper
   - Else: read single char → store in `ch`
2. Drive all output pins LOW (resets any previous HIGH state)
3. Execute action for `ch` (or `digitalRead(ALUM/ATT/NON/LFWD)`):
   - Route to the appropriate motor command and serial response
4. Loop immediately (no `delay()` in main path)

Note: motor `move()` calls are blocking — the loop stalls during rotation.

---

## Common Issues

| Symptom | Likely cause | Fix |
|---------|-------------|-----|
| Bonsai stuck waiting for "at Whiskers" | Serial command not received or Teensy in motor move | Check COM15 connection; verify no prior command left Teensy mid-move |
| Object wheel misaligned after session | `stepperAngle` drifted due to missed steps | Send `r0` to reset angle variable; manually home wheel |
| Linear motor doesn't home on startup | Mechanical obstruction | Clear obstruction; power-cycle Teensy |
| `advancemotor` ISR fires unexpectedly | Noise on pin 12 | Add pull-down resistor; check Bonsai DAQmx wiring |
