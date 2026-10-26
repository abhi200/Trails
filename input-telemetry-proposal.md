# Input and Interaction Telemetry: Proposal for System Review

## Purpose of this document

This is a **proposal written without knowledge of the actual system**. It was drafted from general principles for gaze-and-hand input devices (headset-style, eye tracking plus hand tracking). Every item below is an **estimate of what is worth tracking**, not a statement of what is possible.

**Your job (system agent):** use your knowledge of the real system to decide what is supportable, what needs a substitute, and what should be dropped or added. Where your judgment differs from this proposal, your judgment wins.

## Goals

1. **Locate the problem area** when input feels wrong: sensing, detection, dispatch, app, render, or interaction design.
2. **Infer interaction intent** at a coarse level: did the user get what they meant, or was the commit probably accidental or missed?
3. **Track trends** over builds, devices, and segments. Absolute accuracy is not required.

## Constraints

- **Low overhead.** No per-sample logging. Use counters and fixed-bucket histograms updated at state changes or discrete events, flushed periodically. Sampled full traces (1-5% of sessions) and triggered ring-buffer dumps (on anomalies) are acceptable.
- **Privacy.** Raw gaze data and hand skeletons stay on-device. Only derived values, histograms, and counters leave the device.
- **Log facts, not conclusions.** Do not log "false pinch" or "frustration" directly. Log the underlying signals and derive conclusions later.
- **Separate gaze from hands.** They fail differently. They share the pipeline only after an event is emitted.

## Assumptions made (plain words)

These are guesses. Please confirm or correct each.

| ID | Assumption |
|---|---|
| A1 | Every stage can stamp events on one shared monotonic clock (or clock offsets between components are known). |
| A2 | The tracker exposes per-frame tracking state and a confidence score for gaze and for hands. |
| A3 | Pinch gap (thumb-to-index distance) can be computed on-device from hand joints. |
| A4 | The event queue between the system and the app can be stamped or observed. |
| A5 | The gaze target under the user's eyes is observable. If not, only the target at selection time is. |
| A6 | Frame-presented time can be read. |

## Proposed metrics, in pipeline order

"Pose" means the tracker's estimate for a frame: hand joint positions, or the gaze ray/point.

Priority: **P0** = minimum to tell whether input is fast, healthy, and working. **P1** = lets you attribute a regression to a stage. **P2** = tuning and research.

| # | Segment | Track | What to trace and report | Priority | Relies on |
|---|---|---|---|---|---|
| 1 | **Capture to pose** | Gaze | Latency from sensor capture to gaze pose; tracking state share (valid / degraded / lost); confidence; loss count and time to recover; sample jitter | P0 | A1, A2 |
| 1 | | Hands | Latency from capture to hand pose; tracking state and out-of-FOV share; confidence; loss count and recovery time | P0 | A1, A2 |
| 2 | **Pose to intent signal** | Gaze | Pose to target resolved; fixation vs. saccade state; target ID and angular offset from target center | P1 | A1, A5 |
| 2 | | Hands | Pose to pinch threshold crossed; pinch gap at fire; near-misses (gap closed toward threshold, no fire); closing speed | P0 | A1, A3 |
| 3 | **Signal to event** | Gaze | Dwell or selection-logic time, if dwell is used | P2 | A1 |
| 3 | | Hands | Threshold crossed to event emitted; hold duration; anomalies (double-fire, stuck-down, begin without end); events per minute | P1 | A1 |
| 4 | **Event to queue** | Shared | Dispatch delay; events dropped before queueing | P1 | A1, A4 |
| 5 | **Queue wait** | Shared | Time from enqueued to dequeued by the app; queue depth; drops, coalesces, reorders; event rate vs. consume rate | P0 | A4 |
| 6 | **App handling** | Shared | Handler duration; main-thread stalls overlapping delivery; handler errors; events received but ignored | P0 | A1 |
| 7 | **Render and present** | Shared | Handler done to frame presented; frame drops; reprojection; hover or feedback delay beyond the designed delay | P0 | A1, A6 |
| 8 | **Commit and targeting** | Gaze | Offset from target center at commit; commits with no valid target under gaze | P1 | A5 |
| 8 | | Hands | Pinch quality at commit (clean / weak / near-miss before) | P1 | A3 |
| 9 | **Outcome** | Shared | Undo or back within N seconds of an action; retry bursts on the same target; fallback to another input modality; time from target visible to selection; task success rate and time on task | P0 | logging only |
| 10 | **Telemetry integrity** | Shared | Chain completeness (% of events with full timestamp chain); join success rate; clock drift per session; coverage shown beside every chart | P0 | A1 |

## Derived metrics (no extra logging needed)

| Derived metric | Built from |
|---|---|
| End-to-end input latency | Segments 1 through 7 (capture to frame presented) |
| System-side vs. app-side lag | Segments 1-4 vs. segments 5-7 |
| Commit intent proxy | Gaze state at commit (on target / off target / in transit) + gesture quality (clean / weak / near-miss) + reversed or kept |
| False-positive pinch proxy (range) | Fired pinch plus fast undo, no gaze target, or fired mid-saccade. Floor = fast reversals; ceiling = plus weaker signals |
| False-negative pinch proxy (range) | Near-misses plus retry bursts on the same target |
| Pinch threshold health | Overlap between pinch-gap-at-fire and near-miss gap distributions |

A commit is **probably intended** when gaze was on target, the gesture was clean, and it was not reversed. The rate of everything else is the interaction-problem trend line.

## How to read problems from this data

| Symptom | Likely area |
|---|---|
| Gaze loss up or confidence down | Gaze sensing (lighting, glasses, build) |
| Hand loss or out-of-FOV up | Hand sensing |
| Detection latency up | Gesture detection |
| Near-misses up, or fire-gap distribution shifted | Pinch threshold or detector |
| Queue wait or main-thread stalls up | Dispatch or app |
| Latency fine, suspect-commit rate up | Interaction design (target size, gaze-hand sync) |
| Chain completeness down | Telemetry itself; distrust the other charts |

## If an assumption fails: suggested fallbacks

| If this is not available | Use this instead |
|---|---|
| Confidence score (A2) | Tracking state share, loss and recovery times, jitter in the pose itself (e.g., finger-gap noise while the hand is still) |
| Gaze target under the eyes (A5) | Gaze offset and target only at selection; drop hover-latency rows |
| Queue stamping (A4) | One combined "event emitted to handler start" delivery lag |
| Pinch gap (A3) | Gesture-level events only (fired, hold duration, anomalies); drop near-miss metrics |
| Frame-presented time (A6) | Handler-done time plus frame-time and dropped-frame counters |
| Shared clock (A1) | Per-component latency only; skip cross-stage totals and flag them as unavailable |

## What I need back from you

1. **A verdict per row** of the metrics table: `Supported`, `Partial` (say what is missing), `Not supported`, or `Propose alternative`.
2. **A verdict on each assumption** (A1 to A6): true, false, or unknown.
3. **Substitutes** for any unsupported P0 item, using the fallbacks above or better ones you know of.
4. **Additions**: anything the system exposes that would locate problems or infer intent better than what is listed here.
5. **Overhead check**: flag anything likely to cost meaningful CPU, memory, power, or bandwidth, and suggest a cheaper form (coarser buckets, sampling, trigger-only).
6. **A recommended minimum set**: the smallest list you would ship first, with a one-line reason for anything you cut.

Please return the table with your verdict column filled in, followed by your recommended minimum set.
