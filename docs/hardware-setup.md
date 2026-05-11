# Hardware Setup Guide

This guide covers physical wiring, device configuration, and initial calibration for the active multisensory integration rig.

---

## System Components

```
┌──────────────────────────────────────────────────────────┐
│                   Experiment PC (Windows)                 │
│                                                          │
│  Bonsai 2.7.1                                            │
│  NI DAQmx driver                                         │
│  NI IMAQ / NI Vision                                     │
└──┬──────────┬──────────┬──────────────┬──────────────────┘
   │ USB      │ USB      │ USB3         │ USB3
   │ Serial   │ Serial   │ (NI IMAQ)    │ (NI IMAQ)
   ▼          ▼          ▼              ▼
Teensy    Arduino     Face Camera    Whisker Camera
(COM15)   (COM9)      (cam0)         (cam2)
   │
   ├─ DIR/STEP → Rotary Stepper (object wheel)
   └─ DIR/STEP → Linear Actuator (whisker probe)

NI USB-6xxx (Dev2)
   ├─ port0/lines 0–7 → Neural recording system (TTL sync)
   ├─ ao0 → Speaker amplifier
   └─ ao2 → (reserved)
```

---

## Teensy Wiring

### Object Wheel (Rotary Stepper)

| Teensy Pin | Stepper Driver Pin |
|------------|-------------------|
| 0 (DIR)    | DIR |
| 1 (STEP)   | STEP |
| 2 (SLEEP)  | SLEEP / EN |
| GND        | GND |
| 3.3V or 5V | VDD (logic supply) |

Motor power supply: match motor rated voltage (connect to stepper driver VM/VMOT, not Teensy).

### Linear Actuator

| Teensy Pin | Stepper Driver Pin |
|------------|-------------------|
| 13 (DIR_L) | DIR |
| 14 (STEP_L)| STEP |
| 15 (SLEEP_L)| SLEEP / EN |
| GND        | GND |

### Bonsai ↔ Teensy Digital I/O (optional hardwired path)

These pins allow direct digital triggering from Bonsai DAQmx outputs as an alternative to serial commands. Wire from NI Dev2 digital output lines to corresponding Teensy input pins:

| NI DAQmx Line | Teensy Pin | Signal |
|---------------|------------|--------|
| (user-defined) | 11 (ALUM) | Aluminum trigger |
| (user-defined) | 20 (ATT) | Attenuated trigger |
| (user-defined) | 19 (NON) | Non-stim trigger |
| (user-defined) | 17 (LFWD) | Linear forward |
| (user-defined) | 12 | Reset interrupt |

> In the current software configuration, the Bonsai workflow primarily uses serial commands (`l`, `m`, `n`, `o`, `e`, `f`, `b`) rather than digital pin triggering. The digital input pins on the Teensy are available as a backup path.

### Feedback Pins (Teensy → Bonsai, optional)

| Teensy Pin | Signal | Wire to |
|------------|--------|---------|
| 3 (L_ST) | Linear motor status | NI DAQmx DI (optional) |
| 22 (R_reset) | Reset complete | NI DAQmx DI (optional) |

---

## NI DAQmx Device (Dev2)

### Digital Output Wiring

Connect Dev2/port0 lines to your neural recording system's digital input channels for TTL synchronization. Refer to your recording system's documentation for the appropriate input voltage range (NI output is 3.3V or 5V TTL depending on device model).

| Dev2 Terminal | Signal | Neural Recording Channel |
|---------------|--------|-------------------------|
| port0/line0 | Record gate | (your channel) |
| port0/line2 | At-Whiskers | (your channel) |
| port0/line3 | At-Object | (your channel) |
| port0/line4 | Event marker | (your channel) |
| port0/line5 | Whisker cam frame | (your channel) |
| port0/line6 | Face cam frame | (your channel) |
| port0/line7 | Whisking state | (your channel) |
| GND | Ground reference | (your ground) |

### Analog Output Wiring

| Dev2 Terminal | Connection |
|---------------|-----------|
| ao0 | Speaker amplifier input |
| ao GND | Speaker amplifier ground |

---

## Camera Mounting

### Face Camera (cam0) — 100 fps

- Positioned to capture the full face/snout of the animal
- The whisking detection ROI is a 128×94 px crop; this should cover the whisker pad area
- Analysis ROI in Bonsai: top-left (125,105), bottom-right (448,386) from the 720×540 frame
- Adjust physical camera position and Bonsai ROI parameters so the whisker pad fills the analysis crop

### Whisker Camera (cam2) — 400 fps

- High-speed camera for fine whisker tracking post-hoc
- Positioned for a side or top-down view of the whisker field
- Currently configured as full-frame (no crop); adjust in Bonsai if needed

---

## Object Wheel Calibration

The object wheel must be physically calibrated so the step counts in firmware match actual object positions.

### Calibration procedure:

1. Connect Teensy and open a serial terminal (115,200 baud, COM15)
2. On startup, the linear motor will home (rotate -36000°)
3. Manually send `r0` to confirm the stepper angle counter is at 0
4. Send `l` → wheel should rotate to aluminum object (99 steps / 49.5°)
5. Visually confirm the aluminum foil object is centered in the target zone
6. If not, adjust the physical mounting of objects on the wheel, or modify `al_1`, `at_1`, `no_1` in the firmware

### Step-to-angle mapping (200 steps/rev, 1/1 microstep):

```
1 step = 1.8°
33 steps = 59.4° (firmware unit: "33-step block")
```

Current object positions:

| Object | Steps | Degrees |
|--------|-------|---------|
| Home | 0 | 0° |
| Non-stimulus | 33 | 59.4° |
| Muted | 66 | 118.8° |
| Aluminum | 99 | 178.2° |

---

## Linear Motor Calibration

The linear motor (`whiskPos`) tracks absolute position as a sum of all rotations applied since startup. The homing sweep on boot resets the physical position but `whiskPos` is initialized to 0 and updated incrementally.

To set the whisker contact distance for a new animal:

1. Home the motor (power-cycle or trigger pin 12)
2. Open Bonsai or a serial terminal
3. Send integer commands (e.g., `5` = 5 × 360° = advance 5 mm equivalent) and observe contact
4. Note the total distance as the `whiskPos` baseline for this animal
5. Alternatively, use the Bonsai numeric serial interface to dial in position interactively

---

## Audio System

| Component | Specification |
|-----------|--------------|
| Output channel | Dev2/ao0 |
| Sample rate | 250,000 Hz |
| Signal level | Up to ±10 V (NI analog out) |
| Connect to | Audio amplifier input; attenuate as needed for speaker drive level |

Speaker placement: position speaker so sound reaches the animal's ear field bilaterally. Verify sound onset timing against DAQmx digital events using an oscilloscope if precise latency is required.

---

## Barcode / Sync Arduino (COM9)

A secondary Arduino on COM9 receives end-of-experiment sync commands from Bonsai:

| Command | Meaning |
|---------|---------|
| `q` | Quire — close barcode sequence |
| `s` | Sync — emit synchronization barcode |

This device enables offline alignment of the Bonsai event timeline with spike-sorted neural data from an independent recording system. Wiring details depend on the specific barcode implementation; connect the Arduino's sync output to an auxiliary digital input on the neural recording system.

---

## Power-Up Sequence

Follow this order to avoid ground loops and protect equipment:

1. Power on PC
2. Power on NI DAQmx device (USB connection auto-enumerates)
3. Power on camera USB hubs / cameras
4. Power on stepper motor power supplies (with Teensy USB already connected)
5. Connect Teensy USB — homing sequence begins immediately
6. Open Bonsai and load `Multisensory.bonsai`
7. Verify serial COM15 is assigned to the Teensy in Bonsai properties
8. Press F3 to start cameras; confirm live video in Bonsai monitor windows
9. Press F10 to begin recording session

---

## Troubleshooting

| Problem | Check |
|---------|-------|
| Camera not found | Confirm USB3 connection; verify cam0/cam2 indices in Windows NI-IMAQ device list |
| COM15 not found | Device Manager → Ports → confirm Teensy port; update COM number in Bonsai |
| DAQmx error on start | Confirm Dev2 appears in NI MAX; verify no other application holds the device |
| Stepper misses steps | Reduce RPM in firmware; check motor supply voltage and current limit on driver |
| Audio distorted | Check ao0 output level vs amplifier input range; add attenuator if needed |
| Whisking detection always fires | ROI captures background motion; reposition camera or tighten threshold in Bonsai |
| Whisking detection never fires | Animal not in frame; ROI misconfigured; background subtraction adaptation rate too high |
