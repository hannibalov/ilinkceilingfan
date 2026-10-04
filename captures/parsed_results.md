# Parsed Capture Results

Generated with `tools/parse_capture.py` against the raw files in this
directory. See `README.md` here for what each source file is.

---

## Remote A — FANOFF (`remoteA_fanoff_raw.txt`)

Two long presses captured back to back, ~5s apart.

### Burst 1 (press 1) — ✅ trusted

- 384 raw values → split into 6 repeat sub-frames on ~8.1ms gaps, 4/6 kept (majority length 65)
- Gaps found: `-8105, -8095, -8107, -8090, -8091`

```
code:  # ESPHome transmit_raw
  - 250
  - -750
  - 770
  - -250
  - 260
  - -750
  - 770
  - -240
  - 270
  - -750
  - 760
  - -240
  - 780
  - -240
  - 260
  - -750
  - 760
  - -240
  - 780
  - -240
  - 760
  - -250
  - 260
  - -750
  - 260
  - -750
  - 780
  - -240
  - 260
  - -760
  - 760
  - -240
  - 280
  - -750
  - 260
  - -740
  - 270
  - -750
  - 260
  - -760
  - 770
  - -230
  - 260
  - -760
  - 770
  - -240
  - 780
  - -240
  - 260
  - -750
  - 760
  - -240
  - 780
  - -240
  - 260
  - -750
  - 760
  - -240
  - 770
  - -250
  - 760
  - -250
  - 760
  - -250
  - 270
```

Unsigned (Arduino style): `{ 250, 750, 770, 250, 260, 750, 770, 240, 270, 750, 760, 240, 780, 240, 260, 750, 760, 240, 780, 240, 760, 250, 260, 750, 260, 750, 780, 240, 260, 760, 760, 240, 280, 750, 260, 740, 270, 750, 260, 760, 770, 230, 260, 760, 770, 240, 780, 240, 260, 750, 760, 240, 780, 240, 260, 750, 760, 240, 770, 250, 760, 250, 760, 250, 270 }` (length 65)

### Burst 2 (press 2) — ⚠️ likely a decode glitch, do not use

- 384 raw values → split into 6 repeat sub-frames on ~4ms gaps, 5/6 kept (majority length 73)
- Timing (~490/1000-2000µs) is roughly double Burst 1's unit size, and the frame length (73) doesn't match any other capture of this remote.
- **Ruled out by cross-check**: the Speed Down captures below (same remote, same hardware, same session type) both come out at length 65 with ~250/750-780µs timing — matching Burst 1, not Burst 2. Since two different buttons independently agree with Burst 1's timing and disagree with Burst 2, Burst 2 is treated as a one-off capture artifact (CC1101 AGC settling, a dropped/merged edge, etc.), not a real second protocol variant. Kept here for the record in case it recurs.

---

## Remote A — Speed Down (`remoteA_speeddown_raw.txt`)

### Long press — ✅ trusted

- 384 raw values → 6 sub-frames on ~8.1ms gaps, 4/6 kept (length 65)
- Gaps: `-8122, -8123, -8099, -8128, -8112`

Unsigned: `{ 240, 760, 770, 240, 270, 770, 750, 240, 280, 730, 770, 250, 770, 250, 260, 750, 760, 240, 780, 240, 760, 250, 260, 760, 260, 740, 760, 250, 270, 730, 770, 240, 280, 750, 260, 750, 270, 740, 260, 760, 770, 230, 270, 750, 280, 750, 780, 240, 250, 750, 770, 240, 780, 240, 760, 250, 770, 250, 770, 250, 260, 750, 260, 770, 250 }` (length 65)

### Short press — ✅ trusted

- 257 raw values → 4 sub-frames on ~8.1ms gaps, 3/4 kept (length 65)
- Gaps: `-8107, -8089, -8080`

Unsigned: `{ 260, 750, 770, 250, 260, 750, 770, 240, 270, 740, 760, 240, 780, 240, 260, 750, 760, 240, 780, 240, 760, 250, 260, 750, 260, 750, 780, 240, 280, 750, 760, 240, 280, 740, 260, 760, 250, 750, 270, 740, 770, 240, 250, 770, 250, 750, 270, 750, 250, 770, 250, 770, 250, 770, 240, 770, 750, 250, 270, 770, 760, 240, 280, 750, 260 }` (length 65)

---

## Cross-check summary

| Capture | Length | Unit timing | Gap (ms) | Trust |
| ------- | ------ | ------------ | -------- | ----- |
| FANOFF press 1 | 65 | ~250/750-780µs | ~8.1 | ✅ |
| FANOFF press 2 | 73 | ~490/1000-2000µs | ~4.0 | ⚠️ outlier, likely glitch |
| Speed Down (long) | 65 | ~250/760-780µs | ~8.1 | ✅ |
| Speed Down (short) | 65 | ~250/750-780µs | ~8.1 | ✅ |

Three independent captures agree on **65 pulses, ~250/750-780µs units, ~8.1ms inter-frame gap** as Remote A's real timing family — consistent with the pre-CC1101 RCSwitch-era captures in `README.md` (which used the same ~260µs base delay), just far less noisy now. This is good evidence the CC1101 capture path itself is sound; the next step is confirming *transmit* works, and continuing to fill in the remaining buttons per `CAPTURE_PLAN.md`.

---

## Bit encoding — CONFIRMED (2026-08-04)

Decoding each 65-pulse sub-frame as 32 bits (drop the trailing 65th pulse; pair up the remaining 64 into 32 mark/space pairs, MSB-first): **bit = 1 if mark (carrier ON) > space (carrier OFF), else bit = 0.** This resolves `HYPOTHESES.md` H2.

```
fan_id     = bits 1-16
button_id  = bits 17-24
rolling    = bits 25-32
```

### Decoded results — Remote A

| Capture | Decoded 32-bit code | fan_id | button_id | rolling |
| ------- | -------------------- | ------ | --------- | ------- |
| Fan Off, session 1 | `0x56E50B6F` | `0x56E5` | `0x0B` | `0x6F` |
| Fan Off, session 2, press 1 | `0x56E50B18` | `0x56E5` | `0x0B` | `0x18` |
| Fan Off, session 2, press 2 | `0x56E50B2B` | `0x56E5` | `0x0B` | `0x2B` |
| Speed Down, **long** press | `0x56E5097C` | `0x56E5` | **`0x09`** | `0x7C` |
| Speed Down, **short** press | `0x56E5080A` | `0x56E5` | **`0x08`** | `0x0A` |
| Speed Up, long press | `0x56E50604` | `0x56E5` | `0x06` | `0x04` |
| Speed Up, short press | `0x56E50615` | `0x56E5` | `0x06` | `0x15` |

`fan_id = 0x56E5` on all 7 captures — matches the previously known constant exactly. `0x2B` and `0x6F` for Fan Off's rolling byte also exactly match rolling bytes already recorded in `README.md` from the old noisy RCSwitch-era data — strong cross-validation that the CC1101 capture + this decode rule are both correct.

### ~~Finding: Speed Down uses two different button IDs for tap vs. hold~~ — SUPERSEDED, see below

Original note (2026-08-04, ~20:00): Speed Down's long press decoded to `0x09`, short press to `0x08`, verified real (not noise) by checking every sub-frame individually. At the time this was attributed to a tap-vs-hold protocol distinction. **This interpretation has been superseded** — see the next section. The two presses were never controlled for starting speed level, and we now have direct evidence (below) that button_id tracks the *target speed level*, not press duration. The `0x08`/`0x09` difference was almost certainly just two different target levels reached by the tap vs. the hold, not a tap/hold flag in the protocol itself.

---

## Finding (2026-08-05): button_id encodes the target speed level, not a generic "step" command

Transmit confirmed working end-to-end (Home Assistant -> ESPHome -> CC1101 -> fan) as of 2026-08-04. First real-world test: the user reported no observable difference between Speed Down tap and hold — prompting a re-investigation with controlled starting speeds.

### New captures — Speed Up with known starting speed

| Capture | Transition | Decoded code | button_id | rolling |
| ------- | ---------- | -------------- | --------- | ------- |
| `remoteA_speedup_fromspeed1_raw.txt` | speed 1 -> 2 (single tap) | `0x56E5091A` | `0x09` | `0x1A` |
| `remoteA_speedup_twotaps_raw.txt`, tap 1 | speed 1 -> 2 (tap 1 of 2) | `0x56E5095E` | `0x09` | `0x5E` |
| `remoteA_speedup_twotaps_raw.txt`, tap 2 | speed 2 -> 3 (tap 2 of 2) | `0x56E50E6A` | `0x0E` | `0x6A` |
| `remoteA_speedup_raw.txt` (long+short, prior session) | already at speed 6 (max) | `0x56E50604` / `0x56E50615` | `0x06` | `0x04` / `0x15` |

**The `1 -> 2` transition reproduces `button_id = 0x09` exactly across two fully independent capture sessions.** That's strong evidence this isn't noise — button_id is tied to something about the specific transition, most likely the destination speed level (a "set speed to N" style command rather than a generic "increment" button).

### Does button_id fit an arithmetic function of the target level? No.

Checked level->id points `{2: 0x09, 3: 0x0E, 6: 0x06}` for any linear/offset relationship — the slope between every pair of points is different (5.0, -0.75, -2.67), so there's no consistent linear formula, and no other simple pattern (bit-reversal, complement, etc.) fit either. **Do not extrapolate codes for levels 1, 4, 5 — capture them directly.** The level->code mapping looks like an arbitrary lookup table (plausibly deliberate, to avoid single-bit-flip errors between adjacent levels), not a formula.

### Revised understanding

- `fan_id` (bits 1-16): constant `0x56E5`, confirmed.
- `button_id` (bits 17-24): most likely encodes **which absolute speed level the fan should end up at**, not "which physical button" in a generic sense. Fan Off's consistent `0x0B` across 3 captures fits this too — "off" is just another absolute state, same as levels 1-6.
- `rolling` (bits 25-32): still a genuine per-press rolling value, confirmed random/non-repeating across every capture regardless of button_id.
- **Action item — build the complete level table**: capture Speed Up from every known starting level (aim for all of 1->2, 2->3, 3->4, 4->5, 5->6) and Speed Down likewise (6->5, 5->4, 4->3, 3->2, 2->1), always noting the starting speed before each press. This directly gives the full 6 (or 7, including Off) absolute-state codes needed for a complete Home Assistant speed control, without needing to model tap/hold at all.
- Also re-examine whether Speed Down's `0x08` capture happened to correspond to a specific level once we have the fuller table — it may slot in as one of the yet-uncaptured levels rather than needing separate investigation.

---

## COMPLETE — Remote A speed level table (confirmed 2026-08-05)

Captured every remaining transition: Speed Up from 1 all the way to 6 (`remoteA_speedup_1to6_raw.txt`), then Speed Down from 6 all the way to 1 (`remoteA_speeddown_6to1_raw.txt`), noting the starting level before each press.

| Transition (up) | button_id | Transition (down) | button_id | Agree? |
| ---------------- | --------- | -------------------- | --------- | ------ |
| 1 -> 2 | `0x09` | 3 -> 2 | `0x09` | ✅ |
| 2 -> 3 | `0x0E` | 4 -> 3 | `0x0E` | ✅ |
| 3 -> 4 | `0x05` | 5 -> 4 | `0x05` | ✅ |
| 4 -> 5 | `0x07` | 6 -> 5 | `0x07` | ✅ |
| 5 -> 6 | `0x06` | (matches earlier "already at 6" capture) | `0x06` | ✅ |
| (only reachable going down) | — | 2 -> 1 | `0x08` | matches the original Speed Down "short press" capture from the very first session |

**Every target level's button_id agrees exactly whether reached by pressing Up or Down** — this fully confirms the hypothesis: button_id = absolute target speed level, via an arbitrary (non-arithmetic) per-level lookup table, completely independent of direction or press duration.

### Final level -> button_id table (Remote A)

| Speed level | button_id |
| ----------- | --------- |
| 1 | `0x08` |
| 2 | `0x09` |
| 3 | `0x0E` |
| 4 | `0x05` |
| 5 | `0x07` |
| 6 | `0x06` |
| Off | `0x0B` |

No arithmetic relationship between level and button_id (confirmed no consistent linear fit across any pair) — this is a fixed, arbitrary lookup table, most likely intentional (adjacent levels differ by more than one bit, reducing the chance a single corrupted bit selects a neighboring speed).

### Clean pulse arrays per level (unsigned, Arduino/RCSwitch style; index 0 = sync-adjacent pulse, not the true multi-ms preamble — see the ESPHome `transmit_raw` note in captures/README.md)

```
LEVEL 1: [260, 760, 770, 230, 270, 740, 760, 240, 280, 750, 760, 240, 780, 240, 260, 750, 760, 240, 780, 240, 760, 250, 260, 750, 260, 750, 780, 240, 270, 750, 750, 240, 280, 730, 270, 760, 270, 730, 270, 750, 760, 250, 270, 750, 260, 750, 270, 750, 260, 750, 250, 770, 770, 240, 770, 240, 780, 240, 260, 750, 260, 750, 770, 240, 280]
LEVEL 2: [260, 750, 760, 260, 260, 750, 770, 240, 250, 750, 770, 240, 780, 240, 260, 750, 760, 240, 780, 240, 760, 250, 260, 750, 260, 750, 770, 240, 280, 740, 760, 250, 270, 750, 250, 750, 270, 750, 250, 770, 770, 240, 250, 770, 250, 750, 780, 240, 260, 760, 250, 740, 780, 240, 750, 270, 760, 250, 260, 750, 260, 750, 260, 770, 250]
LEVEL 3: [270, 730, 790, 220, 260, 750, 770, 240, 270, 750, 750, 240, 780, 240, 260, 750, 760, 240, 780, 240, 760, 250, 260, 750, 260, 750, 760, 240, 270, 750, 760, 240, 280, 750, 270, 760, 270, 740, 270, 750, 770, 250, 760, 240, 760, 250, 260, 750, 260, 760, 760, 250, 270, 750, 250, 750, 760, 250, 260, 750, 260, 740, 260, 760, 260]
LEVEL 4: [270, 760, 760, 250, 260, 740, 780, 240, 280, 750, 780, 240, 780, 240, 260, 750, 760, 240, 780, 240, 760, 240, 260, 750, 260, 750, 780, 240, 280, 730, 770, 250, 260, 760, 270, 730, 270, 750, 260, 760, 260, 750, 760, 260, 250, 750, 770, 240, 260, 750, 770, 240, 270, 740, 780, 240, 280, 750, 250, 740, 780, 240, 280, 740, 280]
LEVEL 5: [270, 740, 770, 240, 270, 750, 780, 240, 280, 750, 750, 240, 780, 240, 260, 750, 760, 240, 780, 240, 760, 250, 260, 750, 260, 750, 780, 240, 270, 750, 760, 240, 280, 750, 260, 740, 280, 740, 270, 740, 270, 740, 770, 220, 770, 240, 780, 240, 760, 250, 760, 250, 770, 250, 260, 750, 760, 240, 280, 750, 760, 240, 780, 240, 260]
LEVEL 6: [280, 740, 770, 240, 280, 750, 750, 240, 280, 750, 760, 240, 780, 240, 260, 750, 760, 240, 780, 240, 760, 250, 260, 750, 260, 750, 770, 240, 280, 750, 750, 240, 280, 740, 270, 750, 270, 740, 260, 760, 260, 760, 760, 240, 760, 240, 280, 750, 260, 750, 770, 240, 770, 240, 770, 250, 260, 760, 270, 730, 770, 260, 770, 250, 270]
```

**Remote A speed control is now fully mapped.** Combined with Fan Off (`0x0B`, mapped earlier), Home Assistant can now address all 6 discrete speeds plus off.

---

## Remote B — first captures (2026-08-05)

| Capture | Decoded 32-bit code | fan_id | button_id | rolling |
| ------- | -------------------- | ------ | --------- | ------- |
| Fan Off | `0x56346B03` | `0x5634` | `0x6B` | `0x03` |
| Speed Down | `0x56346866` | `0x5634` | `0x68` | `0x66` |

Both match the pre-CC1101 RCSwitch-era data in `README.md` exactly — same cross-validation pattern as Remote A's first captures.

### Extrapolation from Remote A does NOT work — confirmed, not just assumed

Remote A's Fan Off is `0x0B`; its 6 speed levels live in `0x05`-`0x0E`. Remote B's Fan Off is `0x6B`, an entirely different value — if button_id followed a shared fixed protocol table across remotes, analogous buttons would carry the same button_id on both. They don't. **Each remote (or more likely each remote/fan pairing) has its own arbitrary button_id numbering.** This means Remote B needs the exact same full capture treatment as Remote A — there is no shortcut from Remote A's table. Specifically still needed for Remote B:

- Speed Up + the complete speed level table (1-6), captured both directions for cross-validation, exactly as done for Remote A
- Power, Light, F (forward), ≈ (breeze), R (reverse), 1H/4H/8H timers — zero coverage so far

---

## 2026-08-18 batch: Remote B speed-up sequence, F/R/≈ for both remotes

All captures below are **single-sample** (one press each) — not yet cross-validated with a matching down-transition or repeat presses the way Remote A's speed table was. Treat button_id values as provisional per `captures/README.md`.

### Remote B — Speed Up, 1->2->3->4->5->6 (6 presses starting from speed 1)

| Press | Transition | Decoded code | button_id | rolling |
| ----- | ---------- | -------------- | --------- | ------- |
| 1 | 1 -> 2 | `0x56346901` | `0x69` | `0x01` |
| 2 | 2 -> 3 | `0x56346E17` | `0x6E` | `0x17` |
| 3 | 3 -> 4 | `0x56346E53` | `0x6E` | `0x53` |
| 4 | 4 -> 5 | `0x5634656B` | `0x65` | `0x6B` |
| 5 | 5 -> 6 | `0x563467F0` | `0x67` | `0xF0` |
| 6 | already at 6 (max) | `0x5634660E` | `0x66` | `0x0E` |

**Anomaly (RESOLVED below)**: presses 2 and 3 (2->3 and 3->4) both decoded to button_id `0x6E`.

fan_id = `0x5634` on every capture, matching Remote B's previously known constant.

### RESOLVED (2026-08-18) — the collision was an off-by-one shift, not a real duplicate

Two direct, single-button re-captures — `remoteB_speed1_raw.txt` (Speed 1) and `remoteB_speed{4,6}_recapture_raw.txt` (Speed 4 and Speed 6, captured by directly pressing those specific speed buttons rather than replaying a hold sequence) — decoded to:

| Button pressed directly | Decoded code | button_id |
| ------------------------ | -------------- | --------- |
| Speed 1 | `0x56346855` | `0x68` |
| Speed 4 | `0x5634650D` | `0x65` |
| Speed 6 | `0x5634663D` | `0x66` |

Speed 6's direct capture (`0x66`) matches the original sequence's "already at max" press exactly — cross-validating that capture. But Speed 4's direct capture (`0x65`) does **not** match what the original sequence's press 3 ("3->4") decoded to (`0x6E`) — instead it matches what press 4 ("4->5") decoded to. The explanation: **press 3 in the original hold sequence never actually advanced the fan** (a dropped or bounced press on the physical remote, or the fan ignored a duplicate/too-fast command) — so the fan was still sitting at level 3 when "press 4" fired, meaning press 4 was actually a real `3->4` transition, and everything from there on was shifted one slot from how it was originally labeled.

### Final level -> button_id table (Remote B), cross-validated

| Speed level | button_id | Source |
| ----------- | --------- | ------ |
| 1 | `0x68` | Direct capture (`remoteB_speed1_raw.txt`) — also matches the very first "Speed Down (level unknown)" capture from the pre-table session |
| 2 | `0x69` | Original sequence, press 1 — unambiguous, no shift possible this early |
| 3 | `0x6E` | Original sequence, press 2 — unambiguous |
| 4 | `0x65` | Confirmed by direct re-capture; matches original sequence's press 4 once the shift is accounted for |
| 5 | `0x67` | Original sequence's press 5, after accounting for the shift (not independently re-captured, but it's the only remaining slot and fits the pattern exactly) |
| 6 | `0x66` | Confirmed by direct re-capture; matches original sequence's "already at max" press exactly |

Wired into `fan_controls/esphome_test.yaml` as `send_b_speed1` through `send_b_speed6`. Levels 1, 4, and 6 have independent direct-press confirmation; levels 2, 3, and 5 are still single-sample from the original hold sequence — a full 6->1 down-transition cross-check (like Remote A got) would raise confidence further but isn't required to use these in practice given how well the shift theory explains every data point.

### F / R / ≈ (breeze) — both remotes, one capture each

| Remote | Button | Decoded code | button_id | rolling |
| ------ | ------ | -------------- | --------- | ------- |
| A | F (forward) | `0x56E50244` | `0x02` | `0x44` |
| A | R (reverse) | `0x56E508D7` | `0x08` | `0xD7` |
| A | ≈ (breeze) | `0x56E50AE6` | `0x0A` | `0xE6` |
| B | F (forward) | `0x56346228` | `0x62` | `0x28` |
| B | R (reverse) | `0x563468BB` | `0x68` | `0xBB` |
| B | ≈ (breeze) | `0x56346ADF` | `0x6A` | `0xDF` |

**Two more collisions to flag, same caveat as above:**
- Remote A's R (`0x08`) matches Remote A's Speed Level 1 (`0x08`) exactly.
- Remote B's R (`0x68`) matches Remote B's original Speed Down capture (`0x68`) exactly.

Both are plausible as real collisions (R is a distinct physical function, unlikely to genuinely share a code with a speed level) or as decode noise on a single-sample capture. Needs a second independent press of R on each remote to confirm before trusting it.

Clean pulse arrays (unsigned, Arduino/RCSwitch style) for the F/R/≈ buttons, ready for `tools/parse_capture.py`-style `transmit_raw` use once cross-validated:

```
A F:    [260, 750, 760, 240, 260, 760, 760, 240, 250, 770, 770, 240, 750, 270, 260, 750, 760, 240, 780, 240, 760, 250, 260, 750, 260, 750, 780, 240, 250, 760, 760, 240, 250, 770, 250, 740, 280, 740, 250, 770, 250, 770, 250, 760, 750, 240, 270, 760, 250, 750, 780, 240, 250, 770, 250, 750, 280, 740, 770, 240, 250, 770, 250, 770, 250]
A R:    [270, 750, 770, 220, 260, 740, 780, 240, 280, 750, 760, 240, 780, 240, 260, 750, 760, 240, 780, 240, 760, 250, 260, 750, 260, 740, 780, 240, 280, 730, 770, 250, 270, 750, 260, 750, 270, 750, 260, 760, 760, 240, 270, 740, 270, 750, 250, 770, 770, 260, 780, 230, 280, 730, 760, 240, 280, 750, 770, 240, 780, 240, 760, 250, 270]
A wave: [270, 760, 760, 240, 260, 750, 780, 240, 250, 760, 760, 240, 770, 250, 260, 750, 760, 240, 780, 240, 760, 250, 260, 750, 260, 770, 760, 240, 280, 750, 760, 240, 280, 740, 260, 760, 260, 750, 250, 750, 770, 250, 260, 750, 780, 240, 260, 750, 760, 240, 780, 240, 760, 240, 260, 750, 260, 750, 780, 240, 750, 270, 260, 750, 260]
B F:    [270, 740, 780, 240, 260, 750, 760, 240, 280, 740, 760, 240, 780, 240, 260, 750, 260, 750, 260, 740, 780, 240, 780, 240, 260, 750, 760, 240, 280, 760, 270, 730, 270, 760, 760, 270, 760, 260, 250, 750, 260, 750, 260, 750, 770, 250, 270, 750, 270, 750, 270, 760, 770, 240, 250, 770, 760, 240, 280, 750, 260, 750, 270, 750, 270]
B R:    [270, 740, 790, 220, 280, 740, 760, 240, 280, 750, 760, 240, 780, 240, 260, 750, 260, 770, 250, 740, 780, 240, 780, 240, 260, 750, 760, 240, 280, 750, 250, 740, 280, 740, 780, 240, 780, 240, 260, 760, 760, 240, 270, 730, 270, 750, 270, 740, 770, 240, 270, 750, 760, 240, 780, 250, 770, 250, 260, 750, 760, 240, 780, 240, 260]
B wave: [280, 740, 780, 240, 270, 750, 760, 240, 280, 740, 760, 240, 780, 250, 260, 750, 260, 760, 260, 750, 770, 240, 780, 240, 260, 750, 760, 240, 280, 750, 260, 750, 270, 740, 780, 240, 780, 240, 260, 750, 760, 240, 280, 750, 760, 240, 280, 740, 760, 240, 780, 250, 260, 750, 760, 240, 780, 240, 760, 250, 770, 250, 760, 270, 270]
```
