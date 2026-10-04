# Hypotheses: Why the Fan Does Not Respond

The fans beep when the physical remote is pressed but are silent when the ESP32 transmits. This document lists the most likely root causes, ranked by probability, with evidence and a concrete test for each.

---

## H1: Raw Replay Polarity Is Inverted (Most Likely)

**Claim**: `timings[0]` from RCSwitch is the sync LOW gap (carrier OFF, ~10897 µs). Both `emitter.ino` and the YAML `transmit_raw` send it as the **first carrier-ON pulse** (positive value). The fan sees a ~11 ms carrier burst instead of a ~11 ms silence, which is not a valid sync preamble.

**Evidence**:

- RCSwitch Protocol 1 sync = 1×period HIGH + 31×period LOW. With ~350 µs period: sync LOW ≈ 31 × 350 = 10850 µs ≈ 10897 µs captured.
- The ISR stores `timings[0]` when it detects the long LOW gap (rising edge at end of gap). So `timings[0]` = sync carrier-OFF duration.
- `emitter.ino` line 51: `digitalWrite(TX_PIN, HIGH); delayMicroseconds(pulses[0])` — sends sync gap as carrier ON.
- YAML: `code: [10897, -327, ...]` — first value positive = carrier ON for 10897 µs.

**What the fan sees from our transmission**:

```
Carrier ON  (11 ms)  ← we sent this
Carrier OFF (327 µs)
Carrier ON  (1052 µs)
...
```

**What the fan expects from the real remote**:

```
Carrier OFF (11 ms)  ← sync gap / silence
Carrier ON  (327 µs) ← start of first bit
Carrier OFF (1052 µs)
...
```

**Test**: In `transmit_raw`, negate the first value and invert all signs:

```yaml
code: [-10897, 327, -1052, 1050, -350, 352, -1054, 1076, ...]
```

In `emitter.ino`, start LOW instead of HIGH:

```c
digitalWrite(TX_PIN, LOW);
delayMicroseconds(pulses[0]);
for (int i = 1; i < numPulses; i++) {
  digitalWrite(TX_PIN, (i % 2 == 1) ? HIGH : LOW);
  delayMicroseconds(pulses[i]);
}
```

Flash and test. If fan responds, this was the cause.

---

## H2: ceiling_fan_rf Bit Encoding Is Inverted for Our Fan — ✅ RESOLVED 2026-08-04

**Confirmed with real CC1101 captures (see `captures/parsed_results.md`): mark (carrier ON) LONGER than space (carrier OFF) = bit 1. Mark SHORTER than space = bit 0.** MSB-first, 32 bits per frame = 16-bit fan ID + 8-bit button ID + 8-bit rolling byte.

Decoding 7 independent clean captures (Fan Off ×3, Speed Down ×2, Speed Up ×2) with this rule gives `fan_id = 0x56E5` on every single one, and stable button IDs matching (or extending) the previously-known values from the old noisy RCSwitch data — see the table in `captures/parsed_results.md`. This confirms **our fan uses LONG-mark = 1** (RCSwitch Protocol 1's convention), the same polarity `ceiling_fan_rf` assumed was inverted below. **`ceiling_fan_rf`'s bit encoding is therefore backwards for our fan, as originally suspected** — its `PULSE_HIGH`/`PULSE_LOW` assignment for bit 1/bit 0 needs to be swapped (or all outgoing codes bitwise-NOT'd) before it will produce a signal our fan's receiver decodes correctly.

<details>
<summary>Original hypothesis (kept for context)</summary>

## H2: ceiling_fan_rf Bit Encoding Is Inverted for Our Fan

**Claim**: The `ceiling_fan_rf` component (esphome-bldc-ceil-fan) encodes bit 1 as SHORT mark (250 µs) + LONG space (750 µs). RCSwitch Protocol 1 decodes LONG mark as bit 1. These are opposites. The component sends the bitwise complement of what our fan expects.

**Evidence from `ceiling_fan_rf.cpp`**:

```cpp
static const int PULSE_HIGH = 250;  // mark duration
static const int PULSE_LOW  = 750;  // space duration

// bit 1:
d->item(PULSE_HIGH, PULSE_LOW);  // 250 µs mark, 750 µs space = SHORT mark

// bit 0:
d->item(PULSE_LOW, PULSE_HIGH);  // 750 µs mark, 250 µs space = LONG mark
```

**From `protocol.md`** (bldc-ceil-fan project):

```
Bit 1: 250 µs high / 750 µs low   ← SHORT mark = 1
Bit 0: 750 µs high / 250 µs low   ← LONG mark = 0
```

**RCSwitch Protocol 1**:

```
Bit 1: 3× HIGH + 1× LOW  ← LONG mark = 1
Bit 0: 1× HIGH + 3× LOW  ← SHORT mark = 0
```

These are **inverted**. When the component sends code `0x56E5...`, the fan's receiver (which uses LONG mark = 1) decodes each bit backwards, seeing `~0x56E5... = 0xA91A...`. The fan ID `0xA91A` is not its own ID, so it ignores the command.

**Important caveat**: This assumes our fans use the same physical encoding as our RCSwitch captures suggest (LONG mark = 1). It is possible our fans use the same encoding as the bldc-ceil-fan and RCSwitch is decoding them with inverted bit interpretation. A raw oscilloscope capture or ESPHome `remote_receiver` raw dump would confirm which encoding our remotes actually use.

**Test**:

1. Use ESPHome `remote_receiver` to capture the real remote's raw pulses (see CAPTURE_PLAN.md).
2. Find a bit you know is "1" (e.g., the MSB of `0x56E5` = `0101...` → second bit is 1).
3. Check whether that bit's physical pulse is SHORT mark or LONG mark.
4. If LONG mark = 1 is confirmed, the component sends inverted bits. Fix: swap `PULSE_HIGH`/`PULSE_LOW` in the component, or bitwise-NOT all codes before sending.

</details>

---

## H3: 33-Bit Repeat Frames Are Missing

**Claim**: The real remote sends `[32-bit header] + 8000 µs gap + [33-bit repeat × 6]`. The repeat frames have bit 32 set to 1 and bit 31 set to 0. Our YAML sends only the 32-bit frame repeated N times with 30 ms gaps — this does not match the actual burst structure.

**Evidence**:

- `ceiling_fan_rf.cpp` lines 45–54: builds a 33-bit `repeat_packet = (first_packet & 0x7FFFFFFF) | 0x100000000`, then sends 6 repeats with 8000 µs gap between them.
- Our YAML `transmit_raw` sends the same 65-value array repeated 7/9/13 times with `wait_time: 30ms`. This produces 7 copies of the 32-bit header — no 33-bit repeats, no 8 ms gaps.
- From the bldc-ceil-fan captures: the fan receiver specifically checks that each 32-bit header is followed by 6 identical 33-bit repeats before acting on the command.

**Test**: Capture a full button press burst with ESPHome `remote_receiver` (see CAPTURE_PLAN.md, Step 4). Verify the structure: count pulses, identify where the header ends and repeats begin, confirm bit 32 = 1 in repeat frames.

---

## H4: Captured Timings Contain RCSwitch Artifacts

**Claim**: The raw pulse array in `emitter.ino` and the YAML was captured via RCSwitch's software interrupt decoder. The values have 50–100 µs jitter. More critically, `getReceivedRawdata()` is called in `printBurst()` after the entire burst ends, returning data from the **last decoded frame** — not the first. The label "raw pulse data (first frame)" in `receiver.ino` is wrong.

**Evidence**:

- `receiver.ino` line 157: `unsigned int *raw = rf.getReceivedRawdata()` is called inside `printBurst()`, which runs after a 100 ms idle timeout.
- At that point, `rf.resetAvailable()` has been called multiple times (once per frame in the burst). `getReceivedRawdata()` returns the internal timing buffer, which was last written by the final frame of the burst, not the first.
- The pulse values in `remoteA_FAN_OFF[]` show inconsistencies: the "short" pulses range from 287 to 393 µs and "long" pulses range from 994 to 1115 µs — a ±20% spread that suggests multiple frames' worth of noise.

**Test**: Use ESPHome `remote_receiver` to get accurate single-frame capture. Compare against the current emitter array. Any values outside ±10% of the expected timing (260 µs base) are artifacts.

---

## H5: Rolling Code Is Out of Sync

**Claim**: The `ceiling_fan_rf` component initializes `step_ = 0` on boot. If the fan's internal counter is at step 3 (for example), the first few commands sent will use wrong rolling bytes and the fan may reject them.

**Evidence**:

- `ceiling_fan_rf.h` line 19: `uint8_t step_{0}` — always starts at step 0 after flash/reboot.
- The fan's rolling code acceptance window is unknown. Some rolling code protocols accept ±N steps from the last seen code; others require exact sequence.
- Our long-press captures show the rolling byte is fixed within a burst (e.g., `0x2B` for 13 frames), meaning the fan accepts repeated identical codes during a single press — so it does not require strictly advancing codes on every frame.

**Note**: This only matters once H1 or H2 is resolved (if the signal itself is malformed, the fan ignores it before checking the rolling code).

**Test**: After fixing signal polarity/encoding, send a long burst (13+ repeats) of a known-good code captured from the real remote. If the fan occasionally responds but not reliably, rolling code drift is the issue.

---

## H6: Preamble/Sync Pulse Is Missing or Wrong Length

**Claim**: The real remote may send a longer preamble before the 11 ms sync gap — a series of short mark/space pulses to wake up the fan's RF front-end. Our raw captures start directly with the sync gap. The fan's receiver may need the preamble to activate.

**Evidence**:

- The README's older raw capture data (from a different receiver library) showed `24267 39 1906 149...` — the leading `24267` is roughly 2× the sync gap we capture. This could indicate a longer preamble that RCSwitch doesn't expose.
- The bldc-ceil-fan protocol.md does not mention a preamble beyond the sync gap, but their capture used rtl_433 which may capture differently.

**Test**: ESPHome `remote_receiver` with `dump: raw` will capture the complete raw signal from first edge to last, including any preamble. Compare the total pulse count and leading pulses to the current 65-value array.

---

## Priority Order for Testing

| Priority | Hypothesis                  | Effort                                | Expected Impact                      |
| -------- | --------------------------- | ------------------------------------- | ------------------------------------ |
| 1        | H1: Polarity inversion      | 5 min (edit YAML, reflash)            | Likely — affects every transmission  |
| 2        | H4: Capture noise           | 30 min (add remote_receiver, capture) | Needed before deeper debugging       |
| 3        | H2: Bit encoding inversion  | 30 min (capture + decode check)       | Likely if H1 alone doesn't fix it    |
| 4        | H3: 33-bit repeat structure | 1 hr (implement in YAML)              | Needed once signal polarity is right |
| 5        | H5: Rolling code sync       | 15 min (test after H1-H4)             | Low if fan accepts burst replay      |
| 6        | H6: Preamble missing        | 1 hr (capture + compare)              | Low probability                      |

**Start with H1** (polarity flip in YAML `transmit_raw`). It requires no hardware changes, just editing the sign of values in the existing `code:` array. If the fan beeps, the root cause is confirmed. Then use ESPHome `remote_receiver` to nail down the accurate timings (H4) before tackling H2 and H3.
