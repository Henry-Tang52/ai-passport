<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Time Ledger (v0)

Offline efficiency timer for this FoloToy AI Passport fork. The firmware boots
straight into the application: no demo test menu, no notes, no network, no
Bluetooth, and no voice.

## What it does

- One running timer at a time, using six fixed labels: Internship, Applications,
  Life, Interview, School, and Sport. The on-device UI shows the Simplified
  Chinese label set documented in the Chinese README.
- **Timer page**: current label, idle/running state, and elapsed `HH:MM:SS`.
- **Today page**: a `Xh Ym` day total plus six rows with duration and proportion
  bars. `UP` / `DOWN` switch the Today and Yesterday views.
- Open sessions and per-label Today/Yesterday totals persist in NVS. After
  power loss, the next boot closes the open session at the last heartbeat and
  keeps the record.

The dedicated hardware power button stays a hardware power control, as on the
upstream board. The firmware does not remap it.

## Keys

| Control | Timer page | Today page |
| --- | --- | --- |
| `UP` / `DOWN` short press | Change label only while idle | Switch Today / Yesterday |
| `OK` short press | Start or stop the single timer | Ignored |
| `OK` long press | Open the Today page | Return to the Timer page |

While a timer is running, `UP` / `DOWN` cannot change the label.

## Data

The application keeps a device-local second clock in NVS (`timetrack` /
`state`). It does not use the network or set a wall clock. Days roll every
86400 persisted seconds. A running session is checkpointed about every 10
seconds so a power loss can be closed at the last saved timestamp.

## Build and tests

This repository has no PC or QEMU simulator target. Validate on the host and
with an ESP-IDF 5.5.3 firmware build:

```bash
./tools/validate.sh --static
./tools/validate.sh --firmware
```

The 16px UI font is `assets/fonts/time_track_font_16.c`. Regenerate it with
`python3 tools/gen_time_track_font.py` after changing `main/time_track_text.h`.

## Source layout

- `main/time_track_model.c` — host-tested timer, day, and key logic
- `main/time_track_store.c` — NVS load/save
- `main/time_track_ui.c` — dark / teal screens at font size 16
- `main/main.c` — BSP bring-up and input task
