# Active Multisensory Integration Experiment

**Project Lead:** Ben Efron  
**PI:** Prof. Ilan Lampl  
**Department of Brain Sciences, Weizmann Institute of Science, Israel**

---

## Overview

This repository contains the complete control software for a closed-loop rodent neurophysiology experiment investigating active multisensory integration. The system delivers state-dependent tactile and auditory stimuli to a rodent while recording neural activity via Neuropixel probes. Stimulus delivery is contingent on real-time detection of the animal's whisking behavior, creating a behaviorally-locked experimental paradigm.

The system coordinates five subsystems in real time:

- **Online whisking detection** from high-speed face video
- **Object wheel positioning** (rotary stepper motor) to select tactile stimulus type
- **Linear motor actuation** to bring objects to whisker contact range
- **Audio stimulus generation** synchronized to motor movement
- **Neural recording synchronization** via NI DAQmx digital event markers

---

## Repository Structure

```
Neuropixel-experiment/
├── Bonsai code/
│   ├── Multisensory.bonsai          # Main Bonsai reactive workflow (Bonsai v2.7.1)
│   └── Multisensory.bonsai.layout   # Bonsai UI layout configuration
├── stepperAndLinear/
│   ├── NeuropixelMotors/
│   │   └── NeuropixelMotors.ino     # Teensy firmware for motor control
│   └── Teensy mapping.xlsx          # Pin mapping reference sheet
├── sync-barcodes/                   # Barcode sync utilities (placeholder)
├── Mapping of conditions to dist and motor.xlsx  # Condition-to-motor mapping
├── test.wav                         # Audio test file
├── Aluminum_noise*.wav              # Tactile noise audio files
└── README.md                        # This file
```

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                        Bonsai Workflow (PC)                         │
│                                                                     │
│  ┌─────────────┐    ┌──────────────────┐    ┌───────────────────┐  │
│  │ Face Camera │───▶│ Whisking Detector │───▶│ Trial State Logic │  │
│  │ cam0 100fps │    │ (ROI + threshold) │    │ (4 parallel paths)│  │
│  └─────────────┘    └──────────────────┘    └────────┬──────────┘  │
│                                                       │             │
│  ┌───────────────┐                          ┌────────▼──────────┐  │
│  │ Whisker Camera│                          │  Condition Select  │  │
│  │ cam2  400fps  │                          │  (random, 9 types) │  │
│  └───────────────┘                          └────────┬──────────┘  │
│                                                       │             │
│  ┌──────────────────────────────────────────────────-▼──────────┐  │
│  │              NI DAQmx (Dev2)                                  │  │
│  │  Digital out ×8 @150 kHz ────────────────▶ Neural recording  │  │
│  │  Analog out ao0 @250 kHz ─────────────────▶ Speaker          │  │
│  └───────────────────────────────────────────────────────────────┘  │
│                                                                     │
│  Serial COM15 ◀───────────────────────────────────────────────────┐ │
└─────────────────────────────────────────────────────────────────┬─┘ │
                                                                  │   │
                         ┌────────────────────────────────────────▼───┘
                         │         Teensy Microcontroller              │
                         │                                             │
                         │  ┌──────────────────┐  ┌────────────────┐  │
                         │  │  Rotary Stepper  │  │  Linear Motor  │  │
                         │  │  Object Wheel    │  │  Whisker Probe │  │
                         │  │  40 RPM, 200stp  │  │  450 RPM       │  │
                         │  └──────────────────┘  └────────────────┘  │
                         └─────────────────────────────────────────────┘
```

---

## Experimental Design

### Stimulus Conditions

The experiment presents five tactile conditions selected pseudo-randomly:

| Condition | Serial Command | Object Position | Description |
|-----------|---------------|-----------------|-------------|
| Aluminum  | `l`           | 99 steps        | Aluminum foil object at whisker contact |
| Muted     | `m`           | 66 steps        | Attenuated/silenced tactile stimulus |
| Non-stimulus | `n`        | 33 steps        | No-contact control |
| Sound + stimulus | `s`   | 66 steps        | Tactile with concurrent audio tone |
| Evoked    | `e`           | 33 steps        | Passively-evoked whisker deflection |

### Whisking-State Gating

Each trial is tagged by the animal's whisking state at stimulus onset:

- **No-Whisking path:** triggers when facial motion < 0.8 threshold, sustained over 100-frame window
- **Whisking path:** triggers when facial motion > 0.8 threshold, sustained over 300-frame window

Both paths run four parallel trial branches (Main + Evoked × Whisking/No-Whisking). Evoked trials are only triggered after 10 repetitions of the main condition.

### Trial Sequence

```
1. Whisking state evaluated continuously from face camera ROI
2. When state is stable, condition selected from RandomCondition list
3. Serial command sent to Teensy → object wheel rotates to position
4. Teensy confirms: "at Whiskers" → linear motor advances
5. Stimulus presented for 5 seconds (audio if applicable)
6. Linear motor retracts → wheel returns to home position
7. DAQmx event markers logged throughout → aligned with neural data
8. Trial index + timestamp written to _conditions.csv
```

---

## Hardware Requirements

| Component | Specification |
|-----------|---------------|
| Face camera | NI IMAQ USB3Vision (cam0), 720×540 @ 100 fps |
| Whisker camera | NI IMAQ USB3Vision (cam2), 720×540 @ 400 fps |
| DAQ device | National Instruments DAQmx (Dev2) |
| Microcontroller | Teensy (tested with Teensy 4.x) |
| Rotary stepper | 200 steps/rev, 40 RPM, 1/1 microstep |
| Linear actuator | 200 steps/rev, 450 RPM, 1/1 microstep |
| Speaker | Connected to Dev2/ao0 |
| PC serial port | COM15 (Teensy), COM9 (barcode/sync Arduino) |

---

## Software Requirements

| Software | Version | Purpose |
|----------|---------|---------|
| [Bonsai](https://bonsai-rx.org) | 2.7.1 | Main workflow runtime |
| Arduino IDE / Teensyduino | Latest | Firmware compilation |
| BasicStepperDriver | Latest | Arduino stepper library |
| NI DAQmx driver | Latest | Hardware I/O |
| NI-IMAQ / NI Vision | Latest | Camera acquisition |

### Required Bonsai Packages

- `Bonsai.Vision` — camera acquisition and image processing
- `Bonsai.DAQmx` — NI DAQmx digital and analog I/O
- `Bonsai.Audio` — waveform generation and playback
- `Bonsai.IO` — serial communication and CSV logging
- `Bonsai.Reactive` — state machine and event routing

---

## Setup & Configuration

### 1. Flash Teensy Firmware

1. Install [Teensyduino](https://www.pjrc.com/teensy/td_download.html) and the `BasicStepperDriver` library
2. Open `stepperAndLinear/NeuropixelMotors/NeuropixelMotors.ino` in Arduino IDE
3. Select board: **Teensy 4.x** and the correct COM port
4. Upload — the linear motor will execute a homing rotation (`-36000°`) on startup

### 2. Configure Serial Ports

In the Bonsai workflow, verify these serial port assignments match your system:

| Port | Device | Baud Rate |
|------|--------|-----------|
| COM15 | Teensy motor controller | 115,200 |
| COM9  | Barcode/sync Arduino    | (default) |

### 3. Configure DAQmx Channels

The workflow uses **Dev2** with the following channel assignments:

**Digital Output (port0, 150 kHz):**

| Line | Signal |
|------|--------|
| 0 | Record control |
| 2 | At-whiskers indicator |
| 3 | At-object indicator |
| 4 | Event/condition marker |
| 5 | Whisker camera frame |
| 6 | Face camera frame |
| 7 | Whisking state |

**Analog Output (250 kHz):**

| Channel | Signal |
|---------|--------|
| ao0 | Speaker (audio stimulus) |
| ao2 | (reserved) |

### 4. Configure Cameras

| Parameter | Face Camera (cam0) | Whisker Camera (cam2) |
|-----------|-------------------|-----------------------|
| Resolution | 720×540 | 720×540 |
| Frame rate | 100 fps | 400 fps |
| ROI (analysis) | (125,105)–(448,386) | Full frame |
| Motion ROI | 128×94 px | — |

### 5. Set Experiment Parameters

In Bonsai, edit the externalized properties before each session:

| Property | Default | Description |
|----------|---------|-------------|
| Animal ID | `a10001` | Subject identifier |
| Date of Experiment | `2023-03-19` | Session date |
| DOB | `2022-11-07` | Animal date of birth |
| Path | `D:\Ben\testing\` | Output directory |

---

## Running an Experiment

### Keyboard Controls

| Key | Action |
|-----|--------|
| `F10` | Start recording (activates DAQmx, opens output files) |
| `F3` | Start camera streams |
| `F5` | Stop camera streams |
| `F9` | End experiment (finalizes files, runs end sequence) |
| `F4` | Manual condition trigger |
| `Shift+A` | Manual: send aluminum command (`l`) |
| `Shift+B` (right) | Manual: send backward command (`b`) |

### Session Checklist

- [ ] Verify Teensy connected on COM15 and firmware uploaded
- [ ] Verify cameras enumerated as cam0 and cam2
- [ ] Verify DAQmx device shows as Dev2
- [ ] Set Animal ID, date, and output path in Bonsai properties
- [ ] Press **F3** to confirm camera streams are live
- [ ] Run manual motor commands to verify object wheel positions
- [ ] Press **F10** to begin recording
- [ ] Press **F9** to end experiment

---

## Output Files

All files are saved to the configured `Path` directory with the animal ID as prefix:

| File | Content |
|------|---------|
| `{ID}_Face.avi` | Face camera video |
| `{ID}_Face.csv` | Face camera frame timestamps |
| `{ID}_Whiskers.avi` | Whisker camera video |
| `{ID}_Whiskers.csv` | Whisker camera frame timestamps |
| `{ID}_conditions.csv` | Trial condition index + timestamp |
| `{ID}_allEvents.csv` | All event markers with timestamps |
| `{ID}_metadata.csv` | Session metadata (animal, date, channel map) |

---

## Detailed Documentation

- [Bonsai Workflow](docs/bonsai-workflow.md) — workflow architecture, modules, and reactive logic
- [Teensy Firmware](docs/teensy-firmware.md) — motor control protocol and pin mapping
- [Hardware Setup](docs/hardware-setup.md) — wiring diagrams and device configuration
- [Synchronization](docs/synchronization.md) — how data streams are aligned for analysis

---

## Contact

Ben Efron — Lampl Lab, Department of Brain Sciences, Weizmann Institute of Science
