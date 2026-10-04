# Reliable Capture Plan

**Goal**: Establish ground truth for the exact RF signal each remote button sends, without noise artifacts.

---

## Why Current Captures Are Unreliable

### RCSwitch limitations

- Software interrupt decoder: each pulse edge fires a CPU interrupt, which introduces 20–80 µs jitter per edge
- RCSwitch only reports the decoded 32-bit value and the per-bit delay averaged across the frame — raw `getReceivedRawdata()` reflects this jitter
- The receiver's ISR can miss edges during Serial.print() calls (Serial holds a mutex), silently corrupting the timing buffer for that frame
- `timings[0]` is the sync gap duration (carrier OFF); if we replay it as carrier ON, polarity is inverted

### What we have now

- Single-frame captures with ~50–100 µs per-pulse variation
- Raw pulse array printed AFTER burst ends via `rf.getReceivedRawdata()` — this returns data from the **last** decoded frame in the burst, not the first
- No capture of the 33-bit repeat frames (RCSwitch sees them as separate events or drops them)
- No way to tell if a captured value is the real code or a noise artifact

---

## Step 1: Add ESPHome remote_receiver

ESPHome's `remote_receiver` uses the ESP32's hardware **RMT peripheral** (Remote Control Transceiver). It captures pulse edges in hardware with ~1 µs resolution, no CPU interrupt jitter.

Add to `fan_controls/esphome_test.yaml`:

```yaml
remote_receiver:
  - id: my_rx
    pin:
      number: GPIO27
      inverted: false
    dump: raw
    tolerance: 25%
    buffer_size: 10kb
    filter: 100us
    idle: 10ms
```

This dumps every received raw pulse sequence to the ESPHome log in µs. Connect the receiver module DATA pin to GPIO27.

**Update — CC1101 hardware:** now that the receiver ESP32 has a CC1101 wired in (see README.md "CC1101 Migration Plan"), use `fan_controls/esphome_capture_cc1101.yaml` instead of `esphome_capture.yaml`. Same `remote_receiver`/`dump: raw` mechanism, but the data pin is CC1101's GDO2 (wired to GPIO2) instead of a plain FS1000A-class receiver module, and it's preceded by an `spi:` + `cc1101:` block to configure the chip into async OOK receive mode. Flash it and confirm the log stream still shows plausible pulse trains before moving to Step 2 — if the log is silent or full of garbage, check the SPI wiring (GND/VCC/SCK/MISO/MOSI/CSN) before assuming a protocol problem.

```bash
cd fan_controls
python3 -m esphome run esphome_capture_cc1101.yaml --device /dev/cu.YOUR_ESP_PORT
```

---

## Step 2: Long-Press Capture Protocol

A long-press hold sends 7–13 **identical frames** in rapid succession. This gives multiple samples of the same code so you can identify stable pulses vs noise.

**Procedure per button:**

1. Hold the button for ~2 seconds while watching the ESPHome log
2. The log will print the raw pulse sequence for each frame in the burst
3. Collect at least 5 frames for the same button press
4. Compare the pulse sequences: stable pulses will be consistent across frames; noise will vary

**What to look for:**

- Pulses that are identical across all frames → ground truth timings
- Pulses that vary by >50 µs across frames → noise, use median of all samples
- Frames that look completely different → discard (collision or noise spike)

---

## Step 3: Cross-Validate Captures

Before trusting a captured pulse sequence, verify it is self-consistent:

1. Add a `remote_transmitter.transmit_raw` button in the YAML with the captured pulses
2. Flash and transmit the captured sequence from the emitter ESP32
3. Watch the receiver ESP32 log — it should decode the same 32-bit value as the original remote
4. If the receiver decodes a different value or nothing → the capture has a polarity or timing error

**ESPHome transmit_raw polarity convention**: positive values = carrier ON (mark), negative values = carrier OFF (space).

Check which direction to start:

- If `timings[0]` from RCSwitch is the sync LOW gap (~10850 µs), the first value in `transmit_raw` must be **negative** (e.g., `-10897`)
- If the receiver decodes nothing, flip all signs and retry

---

## Step 4: Capture the Full Burst Structure

The real remote sends `[32-bit header] + gap + [33-bit repeat × 6]`, not just a 32-bit frame repeated.

With ESPHome `remote_receiver` set to `dump: raw`, the log will show the full raw pulse train for an entire button press — including the gap between header and repeats, and all 6 repeat frames.

**What to record:**

For each button:

1. Full raw pulse sequence (header + gap + 6 repeats)
2. Timing of the inter-frame gap between header and first repeat
3. Whether the repeat frames differ from the header (they should have bit 32 set to 1)

---

## Step 5: Per-Button Capture Matrix

The physical remote (see `remote.jpeg`, both Remote A and Remote B are the same "P&B FAN" model) has **11 buttons**. Capture data for **every** button on **both** remotes before attempting a full Home Assistant mapping. Clean CC1101 captures are saved in `captures/` (see `captures/parsed_results.md` for the denoised pulse arrays and decoded 32-bit codes) — cross-checked across 3 independent captures at **65 pulses, ~250/750-780µs units, ~8.1ms inter-frame gap** for Remote A.

**RESOLVED (2026-08-05) — button_id encodes the absolute target speed level, NOT a tap/hold distinction.** Confirmed with a complete cross-validated table: captured every up-transition (1->2->3->4->5->6) and every down-transition (6->5->4->3->2->1), and each target level's button_id matches exactly regardless of direction (e.g. both `1->2` and `3->2` give `0x09` for level 2). Full table in `captures/parsed_results.md` "COMPLETE — Remote A speed level table". **Remote A's speed control (all 6 levels + off) is fully mapped and wired into `fan_controls/esphome_test.yaml`.**

**Capture protocol for the remaining buttons (Power, Light, F, ≈, R, 1H/4H/8H) and all of Remote B**: these aren't speed-level buttons, so the tap/hold question is still open for them — capture both a tap and a hold until proven otherwise there too, same as originally planned.

| Remote | Button (as printed) | Known button ID | Status |
| ------ | -------------------- | ---------------- | --------------------------------------------------- |
| A      | FANOFF (fan off)     | `0x0B`            | ✅ Captured clean (CC1101), see `captures/parsed_results.md` |
| A      | Speed 1              | `0x08`            | ✅ Complete — cross-validated via 2->1 transition, matches original Speed Down short-press capture |
| A      | Speed 2              | `0x09`            | ✅ Complete — cross-validated via both 1->2 and 3->2 |
| A      | Speed 3              | `0x0E`            | ✅ Complete — cross-validated via both 2->3 and 4->3 |
| A      | Speed 4              | `0x05`            | ✅ Complete — cross-validated via both 3->4 and 5->4 |
| A      | Speed 5              | `0x07`            | ✅ Complete — cross-validated via both 4->5 and 6->5 |
| A      | Speed 6              | `0x06`            | ✅ Complete — cross-validated via 5->6 and the original "already at max" capture |
| A      | Power                 | ❌ unconfirmed    | Long press raw, 3 short presses                    |
| A      | Light                 | ❌ unconfirmed    | Long press raw, 3 short presses                    |
| A      | F (forward)           | `0x02` (⚠️ 1 sample) | Captured once, see `captures/parsed_results.md` — needs 2 more presses to confirm |
| A      | ≈ (natural/breeze)    | `0x0A` (⚠️ 1 sample) | Captured once — needs 2 more presses to confirm |
| A      | R (reverse)           | `0x08` (⚠️ 1 sample, collides with Speed Level 1) | Captured once — button_id matches Speed Level 1 exactly, unconfirmed if real collision or noise; needs re-capture |
| A      | 1H (timer)            | ❌ unconfirmed    | Long press raw, 3 short presses                    |
| A      | 4H (timer)            | ❌ unconfirmed    | Long press raw, 3 short presses                    |
| A      | 8H (timer)            | ❌ unconfirmed    | Long press raw, 3 short presses                    |
| B      | FANOFF (fan off)      | `0x6B`            | ✅ Captured clean (CC1101), matches old data exactly |
| B      | Speed down (level unknown) | `0x68`       | ✅ Captured once (CC1101) — level not noted at capture time; redo with known starting level like Remote A |
| B      | Speed levels 1-6      | ✅ complete, wired into YAML | All 6 levels in `fan_controls/esphome_test.yaml` (`send_b_speed1`-`send_b_speed6`): 1=`0x68`, 2=`0x69`, 3=`0x6E`, 4=`0x65`, 5=`0x67`, 6=`0x66`. The apparent 2->3/3->4 collision found in the first up-sequence capture was resolved via direct re-captures of Speed 1, 4, and 6 — it was an off-by-one shift caused by one dropped press, not a real collision. See `captures/parsed_results.md` "RESOLVED" section. Levels 2, 3, 5 are still single-sample; a 6->1 down-transition pass would raise confidence further but isn't required. |
| B      | Power                 | ❌ unconfirmed    | Long press raw, 3 short presses                    |
| B      | Light                 | ❌ unconfirmed    | Long press raw, 3 short presses                    |
| B      | F (forward)           | `0x62` (⚠️ 1 sample) | Captured once, see `captures/parsed_results.md` — needs 2 more presses to confirm |
| B      | ≈ (natural/breeze)    | `0x6A` (⚠️ 1 sample) | Captured once — needs 2 more presses to confirm |
| B      | R (reverse)           | `0x68` (⚠️ 1 sample, collides with old Speed Down capture) | Captured once — button_id matches the earlier `remoteB_speeddown_raw.txt` capture exactly, unconfirmed if real collision or noise; needs re-capture |
| B      | 1H (timer)            | ❌ unconfirmed    | Long press raw, 3 short presses                    |
| B      | 4H (timer)            | ❌ unconfirmed    | Long press raw, 3 short presses                    |
| B      | 8H (timer)            | ❌ unconfirmed    | Long press raw, 3 short presses                    |

For the 3 short presses of each button: record the full 32-bit code each time. The lower byte will change (rolling code), but the upper 24 bits (fan ID + button ID) must be constant. Any variation in the upper 24 bits = noise artifact, discard.

**Do not assume button IDs from `esphome-bldc-ceil-fan`'s documented table** (`0x09`=off, `0x0F`-`0x13`=speeds, etc.) — that table is for a different reference fan and already conflicts with ours (our `0x09` is Speed Down, not Off). Every button ID on our remotes must be captured directly.

---

## Step 6: Extract Timing Ground Truth

From the long-press captures, the same frame repeats 7–13 times. For each pulse position:

1. Collect the duration across all identical frames
2. Take the **median** (not mean — the mean is pulled by noise spikes)
3. Round to nearest 50 µs to get a clean template

This gives you a single canonical pulse array for each button that represents the actual remote timing with noise removed.

**Use `tools/parse_capture.py` to automate this** instead of doing it by hand: paste the raw `Received Raw: ...` lines from the ESPHome log for one button's long-press burst, and it aligns frames by length, takes the per-position median, rounds to the nearest 10 µs, and prints both an unsigned pulse array (for `emitter.ino`/`receiver.ino`-style code) and a signed `transmit_raw` array ready to paste into `esphome_test.yaml`. See `tools/README.md` for usage.

---

## Checklist

- [x] Add `remote_receiver` to YAML and flash (`esphome_capture.yaml` / `esphome_capture_cc1101.yaml`)
- [x] Verify receiver dumps raw pulses in ESPHome log (confirmed on CC1101, 2026-08-04)
- [x] Capture Fan OFF long press for Remote A (2 long presses captured; 1 clean, 1 flagged as a likely glitch — see `captures/parsed_results.md`)
- [ ] Capture Fan OFF long press for Remote B (aim for 7+ frames)
- [x] Extract median timing template from each long press capture (`tools/parse_capture.py`, now handles multi-line ESPHome log wrapping + splits bursts into repeat sub-frames automatically)
- [x] Capture full burst structure (header + repeats) to confirm repeat format — confirmed 6 sub-frames per burst with ~8.1ms gaps (Remote A)
- [x] Capture 3 short presses per button to verify rolling code pattern — done for Remote A Speed Down (long + short both length 65, same timing family as Fan OFF)
- [x] Capture F, ≈, R for Remote A (single-sample each, unconfirmed — see `captures/parsed_results.md`)
- [x] Capture F, ≈, R for Remote B (single-sample each, unconfirmed)
- [x] Capture Remote B Speed Up 1->6 sequence and resolve the 2->3/3->4 collision via direct re-captures — Remote B speed table now complete, see `captures/parsed_results.md`
- [ ] Capture Power, Light, 1H, 4H, 8H for both remotes — currently zero coverage
- [ ] Re-capture F/≈/R (both remotes) with 2+ more presses each to confirm — the R/Speed-1 button_id collision on both remotes looks systematic, not noise, but is still unconfirmed against the real fan
- [ ] Cross-validate by re-transmitting and checking decoder output — blocked on TX-side CC1101 config (GDO0), not yet built
- [ ] Repeat everything above for Remote B (only Fan OFF/Speed Down IDs known from old noisy data — needs a full fresh CC1101 capture pass, not reused from README)
