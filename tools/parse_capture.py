#!/usr/bin/env python3
"""
Turn ESPHome `remote_receiver` raw-dump log output into clean, noise-free
pulse arrays per button (CAPTURE_PLAN.md Step 6, automated).

Usage:
    python3 tools/parse_capture.py path/to/captured_log.txt
    python3 tools/parse_capture.py            # reads from stdin, Ctrl-D to end

Input: paste the ESPHome log output for one or more button presses unedited.
A single long-press burst is usually split by ESPHome across several log
lines, e.g.:

    [19:47:08.218][I][remote.raw:026]: Received Raw: 777, -240, 256, -763, ...
    [19:47:08.243][I][remote.raw:026]:   -748, 758, -244, 780, -244, 259, ...

Only the FIRST line of a burst contains the literal "Received Raw:" marker;
every following line until the next "Received Raw:" (or end of input) is a
wrapped continuation of the SAME burst and is reassembled accordingly. Each
"Received Raw:" marks the start of a new, independent burst (e.g. a separate
button press) and is processed/reported separately.

What it does, per burst:
1. Reassembles the full pulse array by stripping the "[time][level][tag]:"
   log prefix from every line and concatenating the numbers.
2. Splits the burst into repeat sub-frames wherever a pulse exceeds
   GAP_THRESHOLD_US — the real remote sends a header + repeats separated by
   multi-millisecond gaps (~4-11ms observed), which is far larger than any
   real bit pulse (~200-2000us), so this cleanly finds the frame boundaries.
3. Groups sub-frames by length and keeps only the majority-length group
   (mismatched lengths are usually a dropped edge / collision — discarded).
4. Takes the per-position median across the surviving sub-frames (median,
   not mean, so one noise spike doesn't skew the result).
5. Rounds to the nearest 10 microseconds.
6. Prints a signed `transmit_raw`-ready array (ESPHome convention: + = ON,
   - = OFF) and an unsigned array (RCSwitch/Arduino style), plus the gap
   durations found (useful for confirming/documenting burst structure).
"""

import re
import statistics
import sys

GAP_THRESHOLD_US = 3000
LOG_PREFIX_RE = re.compile(r"^.*?\]:\s*")
NUMBER_RE = re.compile(r"-?\d+")


def split_into_bursts(text):
    """Reassemble wrapped log lines into one flat int list per burst."""
    bursts = []
    current = None
    for raw_line in text.splitlines():
        if not raw_line.strip():
            continue
        starts_new_burst = "Received Raw" in raw_line
        content = LOG_PREFIX_RE.sub("", raw_line, count=1)
        if starts_new_burst:
            content = content.split("Received Raw:", 1)[-1]
            current = []
            bursts.append(current)
        if current is None:
            # Content before any "Received Raw:" marker — not part of a burst.
            continue
        current.extend(int(v) for v in NUMBER_RE.findall(content))
    return [b for b in bursts if len(b) >= 4]


def split_on_gaps(burst):
    """Split one long burst into repeat sub-frames at large inter-frame gaps."""
    frames = []
    gaps = []
    current = []
    for v in burst:
        if abs(v) > GAP_THRESHOLD_US:
            gaps.append(v)
            if len(current) >= 4:
                frames.append(current)
            current = []
        else:
            current.append(v)
    if len(current) >= 4:
        frames.append(current)
    return frames, gaps


def majority_length_frames(frames):
    if not frames:
        return []
    lengths = [len(f) for f in frames]
    mode_length = statistics.mode(lengths)
    kept = [f for f in frames if len(f) == mode_length]
    dropped = len(frames) - len(kept)
    if dropped:
        print(f"# Discarded {dropped} sub-frame(s) with a different pulse "
              f"count (kept {len(kept)} of length {mode_length})",
              file=sys.stderr)
    return kept


def median_template(frames):
    length = len(frames[0])
    template = []
    for i in range(length):
        values = [f[i] for f in frames]
        med = statistics.median(values)
        rounded = round(med / 10) * 10
        if rounded == 0:
            rounded = 10 if med >= 0 else -10
        template.append(int(rounded))
    return template


def process_burst(burst, index):
    print(f"\n{'=' * 60}")
    print(f"# Burst {index}: {len(burst)} raw values captured")

    frames, gaps = split_on_gaps(burst)
    if gaps:
        print(f"# Inter-frame gaps found ({len(gaps)}): {gaps}")
    print(f"# Split into {len(frames)} repeat sub-frame(s)")

    if not frames:
        print("# No usable sub-frames (all shorter than 4 pulses) — skipping.",
              file=sys.stderr)
        return

    kept = majority_length_frames(frames)
    if len(kept) < 2:
        print(f"# Only {len(kept)} usable sub-frame(s) — hold the button "
              "longer for more repeats before trusting this result.",
              file=sys.stderr)

    template = median_template(kept)

    print(f"\n# Sub-frames used: {len(kept)} / {len(frames)}")
    print("\n# transmit_raw (ESPHome, signed, + = ON / - = OFF):")
    print("code:")
    for v in template:
        print(f"  - {v}")

    unsigned = [abs(v) for v in template]
    print("\n# Unsigned pulse array (Arduino/RCSwitch style, sign implied "
          "by position — index 0 is the carrier-OFF sync gap per the H1 fix):")
    print("{ " + ", ".join(str(v) for v in unsigned) + " }")
    print(f"Length: {len(unsigned)}")


def main():
    if len(sys.argv) > 1:
        with open(sys.argv[1]) as f:
            text = f.read()
    else:
        print("Paste the ESPHome log lines (one or more button presses), "
              "then press Ctrl-D:", file=sys.stderr)
        text = sys.stdin.read()

    bursts = split_into_bursts(text)
    if not bursts:
        print("No pulse data found in input. Expecting lines containing "
              "'Received Raw: 123, -456, ...' (and their wrapped "
              "continuation lines).", file=sys.stderr)
        sys.exit(1)

    print(f"# Found {len(bursts)} burst(s) (each treated as one button "
          "press/capture — review each separately, they may be different "
          "buttons or even different remotes)", file=sys.stderr)

    for i, burst in enumerate(bursts, 1):
        process_burst(burst, i)


if __name__ == "__main__":
    main()
