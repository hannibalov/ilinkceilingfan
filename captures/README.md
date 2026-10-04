# captures/

Raw ESPHome `remote_receiver` log output, saved verbatim (unedited) per
capture session, plus the cleaned-up results extracted from them. This is
the durable record CAPTURE_PLAN.md's per-button matrix is built from —
don't rely on chat scrollback for this data.

## Files

| File | Remote | Button | Captured | Hardware | Notes |
| ---- | ------ | ------ | -------- | -------- | ----- |
| `remoteA_fanoff_raw.txt` | A | FANOFF | 2 long presses, 2026-08-04 19:47 | CC1101 (`esphome_capture_cc1101.yaml`) | Burst 1 (65 pulses, ~250/750-780µs, 8.1ms gaps) is clean and matches the family's known timing. **Burst 2 (73 pulses, ~490/1980µs) is very likely a decode glitch, not a real second protocol** — see `parsed_results.md`. |
| `remoteA_speeddown_raw.txt` | A | Speed down | 1 long press + 1 short press, 2026-08-04 19:52-19:53 | CC1101 (`esphome_capture_cc1101.yaml`) | Both bursts are clean, 65 pulses, ~250/750-780µs — confirms Burst 1 above (not Burst 2) is the real family timing. **Superseded interpretation, see `parsed_results.md`**: the button_id difference found here (`0x08` vs `0x09`) is now believed to reflect the *target speed level* reached, not a tap/hold distinction — the two presses weren't controlled for starting speed. |
| `remoteA_speedup_raw.txt` | A | Speed up | 1 long + 1 short press, both while already at speed 6 (max), 2026-08-04 19:56-19:57 | CC1101 | Both decode to button_id `0x06` — consistent with "target level = 6" once we know the level-encoding hypothesis, not a tap/hold artifact. |
| `remoteA_speedup_fromspeed1_raw.txt` | A | Speed up | 1 tap, fan at speed 1 -> 2, 2026-08-04 20:19 | CC1101 | button_id `0x09` |
| `remoteA_speedup_twotaps_raw.txt` | A | Speed up | 2 taps back-to-back, fan at speed 1 -> 2 -> 3, 2026-08-04 20:20 | CC1101 | First tap (1->2) `0x09` — matches the independent capture above exactly. Second tap (2->3) `0x0E`. |
| `remoteA_speedup_1to6_raw.txt` | A | Speed up | 5 presses, fan at speed 1 all the way to 6, 2026-08-05 20:25-20:26 | CC1101 | Completes the level table with speed down below — see `parsed_results.md` "COMPLETE — Remote A speed level table". |
| `remoteA_speeddown_6to1_raw.txt` | A | Speed down | 5 presses, fan at speed 6 all the way to 1, 2026-08-05 20:27-20:28 | CC1101 | Every level's button_id matches the Speed Up capture exactly — level table fully cross-validated. |
| `remoteB_fanoff_raw.txt` | B | FANOFF | 1 press, 2026-08-05 20:31 | CC1101 | `0x56346B03` — matches old pre-CC1101 README data exactly. |
| `remoteB_speeddown_raw.txt` | B | Speed down | 1 press, 2026-08-05 20:32 | CC1101 | `0x56346866` — matches old pre-CC1101 README data exactly. **Button_id (`0x68`) does not match any Remote A button_id** — confirms each remote has its own arbitrary button_id numbering; Remote A's table cannot be reused for Remote B. |
| `remoteB_speedup_1to6_raw.txt` | B | Speed up | 1st of 6 presses, fan starting at speed 1, 2026-08-18 20:38 | ESPHome logger (not CC1101 capture yaml) | Transition 1->2, button_id `0x69`. Single sample only — not yet cross-validated via a matching down-transition like Remote A's table was. |
| `remoteB_speedup_2_raw.txt` | B | Speed up | 2nd of 6 presses, 2026-08-18 20:39 | ESPHome logger | Transition 2->3, button_id `0x6E`. |
| `remoteB_speedup_3_raw.txt` | B | Speed up | 3rd of 6 presses, 2026-08-18 20:39 | ESPHome logger | Nominally "3->4", decoded button_id `0x6E` — **same as the 2->3 capture above.** **RESOLVED**: this press never actually advanced the fan (dropped/bounced press) — see `parsed_results.md` "RESOLVED — the collision was an off-by-one shift". |
| `remoteB_speed1_raw.txt` | B | Speed 1 (direct press) | 1 press, 2026-08-18 21:23 | ESPHome logger | `0x56346855` — button_id `0x68`. Matches the original pre-table "Speed Down (level unknown)" capture exactly. |
| `remoteB_speed4_recapture_raw.txt` | B | Speed 4 (direct press) | 1 press, 2026-08-18 21:24 | ESPHome logger | `0x5634650D` — button_id `0x65`. Direct re-capture that resolved the 2->3/3->4 collision above — see `parsed_results.md`. |
| `remoteB_speed6_recapture_raw.txt` | B | Speed 6 (direct press) | 1 press, 2026-08-18 21:24 | ESPHome logger | `0x5634663D` — button_id `0x66`. Matches the original "already at max" capture from the first up-sequence exactly. |
| `remoteB_speedup_4_raw.txt` | B | Speed up | 4th of 6 presses, 2026-08-18 20:39 | ESPHome logger | Transition 4->5, button_id `0x65`. |
| `remoteB_speedup_5_raw.txt` | B | Speed up | 5th of 6 presses, 2026-08-18 20:39 | ESPHome logger | Transition 5->6, button_id `0x67`. |
| `remoteB_speedup_6_raw.txt` | B | Speed up | 6th of 6 presses, fan already at speed 6 (max), 2026-08-18 20:40 | ESPHome logger | button_id `0x66`. |
| `remoteA_F_raw.txt` | A | F (forward) | 1 press, 2026-08-18 20:42 | ESPHome logger | `0x56E50244` — button_id `0x02`, rolling `0x44`. Single sample only. |
| `remoteA_R_raw.txt` | A | R (reverse) | 1 press, 2026-08-18 20:42 | ESPHome logger | `0x56E508D7` — button_id `0x08`, rolling `0xD7`. **Button_id matches Remote A's Speed Level 1 exactly** — unconfirmed whether this is a real collision or a decode artifact; needs a second independent capture to check. |
| `remoteA_wave_raw.txt` | A | ≈ (breeze/natural) | 1 press, 2026-08-18 20:43 | ESPHome logger | `0x56E50AE6` — button_id `0x0A`, rolling `0xE6`. Single sample only. |
| `remoteB_F_raw.txt` | B | F (forward) | 1 press, 2026-08-18 20:43 | ESPHome logger | `0x56346228` — button_id `0x62`, rolling `0x28`. Single sample only. |
| `remoteB_R_raw.txt` | B | R (reverse) | 1 press, 2026-08-18 20:44 | ESPHome logger | `0x563468BB` — button_id `0x68`, rolling `0xBB`. **Button_id matches Remote B's Speed Down capture (`0x68`) exactly** — same open question as Remote A's R/Level-1 collision above. |
| `remoteB_wave_raw.txt` | B | ≈ (breeze/natural) | 1 press, 2026-08-18 20:44 | ESPHome logger | `0x56346ADF` — button_id `0x6A`, rolling `0xDF`. Single sample only. |

**Caveat on the 2026-08-18 batch above**: these were captured from the ESPHome `logger` component's raw dump (see the user-supplied `[remote.raw:0xx]` log lines), not necessarily via `esphome_capture_cc1101.yaml` — same underlying `remote_receiver`/`dump: raw` mechanism and decode rule, but each button only has **one** capture so far. Per `CAPTURE_PLAN.md`'s own protocol, 3 short presses per button are needed to confirm the rolling-code pattern and rule out noise before treating a button_id as ground truth. Treat every button_id in this batch as provisional until re-confirmed.

## parsed_results.md

Output of `tools/parse_capture.py` against each raw file above — the
denoised `transmit_raw` / unsigned pulse arrays ready to use, plus the
per-burst diagnostics (gap durations, discarded sub-frames).

## Adding a new capture

1. Flash `fan_controls/esphome_capture_cc1101.yaml` (or `esphome_capture.yaml`
   if still on the old receiver module) to the receiver ESP32.
2. Hold one button on one remote for ~2s, copy the full `Received Raw: ...`
   log block (including its wrapped continuation lines) into a new file
   here named `remote{A,B}_{button}_raw.txt`.
3. Run `python3 tools/parse_capture.py captures/remote{A,B}_{button}_raw.txt`
   and append the result to `parsed_results.md`.
4. Update the status column for that button in `CAPTURE_PLAN.md`'s per-button
   matrix.
