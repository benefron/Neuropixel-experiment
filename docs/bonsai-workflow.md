# Bonsai Workflow Reference

**File:** `Bonsai code/Multisensory.bonsai`  
**Bonsai version:** 2.7.1  
**Lines:** ~3,800 (XML)

---

## Overview

The Bonsai workflow is a reactive, event-driven program built around RxJS-style observable sequences. It runs on a Windows PC and orchestrates all real-time aspects of the experiment: camera acquisition, online whisking detection, condition selection, motor commanding, audio output, DAQmx synchronization, and data logging.

The workflow is composed of several nested `GroupWorkflow` modules that communicate through shared `BehaviorSubject` and `MulticastSubject` channels.

---

## Module Map

```
Multisensory.bonsai
├── Whisking Detection
├── Trial Logic
│   ├── No-Whisking + Main
│   ├── Whisking + Main
│   ├── No-Whisking + Evoked
│   └── Whisking + Evoked
├── Reports (serial parser)
├── Face Camera Recording
├── Whisker Camera Recording
├── Audio Stimulus (Evoked_aud)
├── DAQmx Digital Sync
├── Data Logging (CSV writers)
└── Keyboard Controls
```

---

## Shared Channels (Subjects)

These subjects act as the inter-module communication bus:

| Subject Name | Type | Carries |
|--------------|------|---------|
| `endW` | BehaviorSubject\<bool\> | Current whisking state (true = whisking) |
| `outputs` | MulticastSubject\<string\> | "Whisking" / "No whisking" |
| `Face_Cam` | BehaviorSubject\<image\> | Latest face camera frame |
| `sound` | MulticastSubject | Audio trigger |
| `playSound` | MulticastSubject | Start audio playback |
| `stopSound` | MulticastSubject | Stop audio playback |
| `RandomCondition` | BehaviorSubject\<int\> | Current condition index (0–8) |
| `above30` | BehaviorSubject\<int\> | Evoked trial counter (no-whisking) |
| `above30No` | BehaviorSubject\<int\> | Evoked trial counter (whisking) |
| `countingN` | BehaviorSubject\<int\> | Main trial counter (no-whisking) |
| `countingW` | BehaviorSubject\<int\> | Main trial counter (whisking) |
| `indN` | BehaviorSubject\<int\> | Condition index pointer (no-whisking) |
| `indW` | BehaviorSubject\<int\> | Condition index pointer (whisking) |
| `At_object` | BehaviorSubject\<bool\> | Motor reached object position |
| `At_whiskers` | BehaviorSubject\<bool\> | Linear motor at whisker position |
| `motorBack` | BehaviorSubject\<bool\> | Motor returning to home |
| `Evoked_on` | MulticastSubject | Evoked stimulus active signal |

---

## Module: Whisking Detection

**Purpose:** Continuously classifies the animal's whisking state from the face camera.

**Pipeline:**

```
Face camera (cam0, 720×540 @ 100fps)
  → Grayscale
  → Crop ROI (128×94 px, positioned over whisker pad)
  → Background subtraction (7-px threshold, 4% adaptation rate)
  → ROI Avg (mean pixel intensity of motion residual)
  → Threshold (> 0.8 → whisking event)
  → Buffer(100) → Sum → Threshold(> 50)   ← short-window whisking
  → Buffer(300) → Sum → Threshold(> 200)  ← sustained whisking
  → Publish to `endW` (bool) and `outputs` (string)
```

The two-stage buffering creates hysteresis: the animal must show brief activity (100-frame window) to enter the whisking state, and sustained activity (300-frame window) to confirm it. This prevents spurious state transitions from individual frames.

---

## Module: Trial Logic

There are four parallel trial branches. All share the same internal structure but are gated on `endW` (whisking state) and on whether the trial is a "Main" or "Evoked" trial.

### Branch Structure (Main trials)

```
endW (whisking state gate)
  → WithLatestFrom(RandomCondition)
  → Filter(counter == 10)           ← 10 stable frames required
  → Map condition index → serial command via "mapping to serial"
  → SerialWrite(COM15)              ← command Teensy
  → Wait for "at Whiskers" from Reports module
  → 5-second stimulus window
    ├── Audio (if condition is 's' or 'e')
    └── DAQmx event marker HIGH
  → Increment countingN / countingW
  → Update indN / indW
```

### Condition Mapping ("mapping to serial")

| Condition Index | Serial Command | Motor Target | Audio? |
|-----------------|---------------|--------------|--------|
| 0 | `l` | 99 steps (aluminum) | No |
| 1 | `m` | 66 steps (muted) | No |
| 2 | `n` | 33 steps (non-stim) | No |
| 3 | `s` | 66 steps (muted pos) | Yes |
| 4 | `e` | 33 steps (non-stim pos) | Yes |

### Branch Structure (Evoked trials)

```
endW gate → Filter(above30 == 10)   ← only after 10 main trials
  → SerialWrite('o')                ← Teensy moves to evoked position
  → Activate Evoked_on
  → Audio stimulus (Evoked_aud)
  → Reset above30 counter
```

---

## Module: Reports (Serial Parser)

**Purpose:** Reads serial responses from the Teensy and updates shared state subjects.

**Source:** COM15 at 115,200 baud  

**Message routing:**

| Teensy message | Action |
|----------------|--------|
| `"aluminum"` | Set `At_object` = true, emit Condition 0 |
| `"aluminum silenced"` | Set `At_object` = true, emit Condition 1 |
| `"non"` | Set `At_object` = true, emit Condition 2 |
| `"at Whiskers"` | Set `At_whiskers` = true |
| `"reset"` | Set `At_start` = true |
| `"Evoked"` | Set `At_object` = true |
| `"Moving back"` | Set `motorBack` = true |

These boolean flags gate downstream operations: for example, the main trial branch waits for `At_whiskers = true` before starting the 5-second stimulus window.

---

## Module: Face Camera Recording

**Source:** cam0 (NI IMAQ USB3Vision)  
**Resolution:** 720×540 @ 100 fps  
**Analysis ROI:** (125,105) to (448,386) → 323×281 px (face region)

**Recording triggered by:** F3 / F5 keys or "Start Recording" signal (F10)

**Outputs:**

| File | Format | Content |
|------|--------|---------|
| `{ID}_Face.avi` | AVI (FourCC=40) | Full video |
| `{ID}_Face.csv` | CSV | Frame index per acquisition |

---

## Module: Whisker Camera Recording

**Source:** cam2 (NI IMAQ USB3Vision)  
**Resolution:** 720×540 @ 400 fps  
**ROI:** Full frame (no crop)

**Outputs:**

| File | Format | Content |
|------|--------|---------|
| `{ID}_Whiskers.avi` | AVI @ 400 fps | Full video |
| `{ID}_Whiskers.csv` | CSV | Frame index per acquisition |

---

## Module: Audio Stimulus (Evoked_aud)

**Purpose:** Generate multi-tone auditory stimulus via DAQmx analog output.

**Device:** Dev2/ao0  
**Sample rate:** 250,000 Hz  
**Buffer:** 60,000 samples, FiniteSamples mode

**Tone sequence:**

| Frequency | Amplitude | Onset offset |
|-----------|-----------|--------------|
| 500 Hz | 0.008 V | 0 ms |
| 2000 Hz | 0.15 V | +100 ms |
| 4000 Hz | 0.025 V | +200 ms |
| 8000 Hz | — | +300 ms |

Audio is triggered by `playSound` subject and terminated by `stopSound`.

---

## Module: DAQmx Digital Synchronization

**Device:** Dev2/port0  
**Sample rate:** 150,000 Hz  
**Mode:** Continuous digital output

Each digital line goes HIGH when the corresponding event is active, providing a TTL-level event record that can be sampled by the neural recording system:

| Line | Signal | Fired when |
|------|--------|------------|
| 0 | Record | Recording session is active |
| 2 | At-Whiskers | Linear motor confirmed at whisker position |
| 3 | At-Object | Object wheel confirmed at target position |
| 4 | Event | Condition/stimulus marker |
| 5 | Whisker-Cam | Whisker camera frame acquired |
| 6 | Face-Cam | Face camera frame acquired |
| 7 | Whisking | Animal detected as whisking |

---

## Module: Data Logging

| File | Trigger | Columns |
|------|---------|---------|
| `_conditions.csv` | Each trial start | Condition index, timestamp |
| `_allEvents.csv` | Every event marker | Event label, timestamp |
| `_Face.csv` | Each face frame | Frame index |
| `_Whiskers.csv` | Each whisker frame | Frame index |
| `_metadata.csv` | Session start | Animal ID, DOB, date, channel map |

---

## Keyboard Shortcuts Reference

| Key | Action |
|-----|--------|
| F3 | Start cameras |
| F4 | Manual condition trigger (`"3"`) |
| F5 | Stop cameras |
| F9 | End experiment → triggers stop sequence |
| F10 | Start recording |
| Left Shift + A | Manual: aluminum (`l`) |
| Right Shift + B | Manual: backward (`b`) |

---

## Timing and Latency Notes

- Whisking detection latency: ~100 ms (100-frame buffer @ 100 fps) for initial detection; ~300 ms (300-frame buffer) for sustained state confirmation
- Motor command round-trip: serial write → Teensy rotation → serial response ("at Whiskers") is blocking; duration depends on object wheel angular distance (~0.1–2 s depending on starting angle)
- Audio onset jitter: bounded by DAQmx buffer size (60,000 samples @ 250 kHz = 240 ms buffer)
- DAQmx digital sync jitter: <1 sample period = 6.7 µs @ 150 kHz
