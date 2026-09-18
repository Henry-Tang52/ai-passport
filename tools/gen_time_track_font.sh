#!/usr/bin/env bash
# Wrapper around the Python generator so the documented command stays one step.
set -euo pipefail
repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
exec python3 "${repo_root}/tools/gen_time_track_font.py" "$@"
