# tools/

## parse_capture.py

Converts a burst of raw ESPHome `remote_receiver` log lines into a single
clean pulse array (median timing across frames, noise discarded). See
`CAPTURE_PLAN.md` Step 6 for the underlying method.

```bash
# From a saved log excerpt:
python3 tools/parse_capture.py capture_fanA_off.txt

# Or paste directly (Ctrl-D when done):
python3 tools/parse_capture.py
```

Capture one button per run: flash `fan_controls/esphome_capture.yaml`, hold
a single button for ~2s, copy the `Received Raw: ...` lines for that burst
out of the ESPHome log (`esphome logs esphome_capture.yaml`), and feed them
to this script. Repeat per button, per remote — see the capture matrix in
`CAPTURE_PLAN.md`.
