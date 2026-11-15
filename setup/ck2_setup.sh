#!/bin/bash
# ---------------------------------------------------------------------------
# CorridorKey 2 for Baselight - model setup
#
# From Creative Video Tips (CVT)
# Created by Chadwick  |  https://creativevideotips.com
# An unofficial plugin around CorridorKey 2.0 by Corridor Digital.
# Licence: CC BY-NC-SA 4.0 with the CorridorKey additional terms (see LICENSE).
#
# What it does (into ~/CorridorKey unless --folder says otherwise):
#   1. Asks you to accept the CorridorKey licence.
#   2. Downloads CorridorKey 2.0's code from GitHub at the tested commit and
#      checks every file against the list made when it was tested.
#   3. Builds a private Python for it (Python 3.12 and pinned packages).
#   4. Downloads the weights (about 5 GB: CorridorKey's own and the three
#      foundation models it uses) and checks them the same way.
#   5. Loads the model once on the NVIDIA GPU to confirm it works.
# Safe to run again: finished steps are checked and skipped.
#
# Options:
#   --folder DIR          install somewhere else (then set Setup > Model
#                         Folder and Python in the node to match)
#   --weights-from DIR    copy the weights from an existing CorridorKey 2.0
#                         folder (for example on the NAS) instead of
#                         downloading them
#   --skip-licence-prompt only for automated tests of this script
# ---------------------------------------------------------------------------
set -u

COMMIT="485f5e9ab759ead6138cc990776184b59d6ea971"
CODE_URL="https://codeload.github.com/nikopueringer/CorridorKey2.0/tar.gz/$COMMIT"
HERE="$(cd "$(dirname "$0")" && pwd)"
WORKER="$HERE/../worker/ck2_worker.py"
FOLDER="$HOME/CorridorKey"
WEIGHTS_FROM=""
ASK_LICENCE=1

while [ $# -gt 0 ]; do
    case "$1" in
        --folder) FOLDER="$2"; shift 2 ;;
        --weights-from) WEIGHTS_FROM="$2"; shift 2 ;;
        --skip-licence-prompt) ASK_LICENCE=0; shift ;;
        *) echo "Unknown option: $1"; exit 2 ;;
    esac
done

CODE="$FOLDER/CorridorKey2.0"
PY="$FOLDER/.venv/bin/python"
UV="$HERE/bin/uv"
say()  { printf '\n== %s\n' "$*"; }
fail() { printf '\nSTOPPED: %s\n' "$*"; exit 1; }

say "CorridorKey 2 for Resolve - model setup"
echo "  Folder: $FOLDER"
[ "$(uname -s)" = "Linux" ] && [ "$(uname -m)" = "x86_64" ] || fail "this needs Linux on x86_64."
LOCK="$HERE/requirements-lock-linux.txt"     # PyTorch 2.9.1 for CUDA 12.8, as CorridorKey 2.0 is tested
GPU="the NVIDIA GPU"
command -v nvidia-smi >/dev/null && echo "  GPU: $(nvidia-smi --query-gpu=name,driver_version,memory.total --format=csv,noheader | head -1)"
if command -v sha256sum >/dev/null; then SHA256=sha256sum; else SHA256="shasum -a 256"; fi
mkdir -p "$FOLDER" || fail "can't create $FOLDER"
chmod 700 "$FOLDER"
FREE_GB=$(df -BG --output=avail "$FOLDER" | awk 'NR==2 {gsub("G",""); print $1}')
[ -d "$CODE/weights" ] || [ "$FREE_GB" -ge 15 ] || fail "need about 15 GB free (have ${FREE_GB} GB)."
[ -x "$UV" ] || fail "the plugin's uv is missing ($UV). Reinstall the plugin."

# --- 1. licence ----------------------------------------------------------------
if [ $ASK_LICENCE -eq 1 ] && [ ! -f "$FOLDER/.licence-accepted" ]; then
    cat <<'EOF'

CorridorKey 2.0 is made by Corridor Digital and comes under the Corridor Key
Licence: CC BY-NC-SA 4.0 with additional terms. In short:
  - You may use it, including for commercial projects; what you make is yours.
  - No reselling, repackaging or paid services built on it.
  - If you work for an organisation with more than US $20 million a year in
    revenue, each user needs a CorridorKey seat from https://corridorkey.com.
  - The foundation models it uses keep their own licences (NVIDIA Open Model
    License for C-RADIOv4, MIT for MoGe-2, CC BY 4.0 for RVM).
Full text: https://github.com/nikopueringer/CorridorKey2.0/blob/main/LICENSE
This plugin is unofficial and not made or endorsed by Corridor Digital.
EOF
    printf '\nType yes to accept the CorridorKey licence and continue: '
    read -r ANSWER
    [ "$ANSWER" = "yes" ] || fail "licence not accepted; nothing was downloaded."
    date '+%Y-%m-%d %H:%M' > "$FOLDER/.licence-accepted"
fi

# --- 2. CorridorKey's code -----------------------------------------------------
verify_code() { (cd "$CODE" && $SHA256 -c --quiet "$HERE/corridorkey_files.sha256") > /dev/null 2>&1; }
if [ -d "$CODE" ] && verify_code; then
    say "CorridorKey code: already in place and checked"
else
    say "Downloading CorridorKey 2.0's code (commit ${COMMIT:0:7})"
    TMP=$(mktemp -d)
    curl --fail --silent --show-error --location --proto '=https' -o "$TMP/code.tar.gz" "$CODE_URL" \
        || fail "download failed. Check the internet connection."
    tar -xzf "$TMP/code.tar.gz" -C "$TMP" || fail "the download is damaged."
    NEW="$TMP/CorridorKey2.0-$COMMIT"
    (cd "$NEW" && $SHA256 -c --quiet "$HERE/corridorkey_files.sha256") \
        || fail "the downloaded code doesn't match the tested version."
    # Keep weights already there; replace only the code.
    if [ -d "$CODE" ]; then
        for d in weights vfms; do [ -d "$CODE/$d" ] && rm -rf "$NEW/$d" && mv "$CODE/$d" "$NEW/$d"; done
        rm -rf "$CODE"
    fi
    mv "$NEW" "$CODE" && rm -rf "$TMP"
    echo "  Checked $(wc -l < "$HERE/corridorkey_files.sha256" | tr -d ' ') files."
fi

# --- 3. Python -----------------------------------------------------------------
say "Python environment"
export UV_PYTHON_INSTALL_DIR="$FOLDER/python"
export UV_CACHE_DIR="$FOLDER/.uv-cache"
if [ ! -x "$PY" ]; then
    "$UV" venv -q --python 3.12 --python-preference only-managed "$FOLDER/.venv" \
        || fail "could not set up Python 3.12."
fi
"$UV" pip install -q --python "$PY" --torch-backend cu128 -r "$LOCK" "$HERE"/wheels/*.whl \
    || fail "package install failed."
"$PY" -c "import torch; assert torch.cuda.is_available()" 2>/dev/null \
    || fail "PyTorch can't use $GPU. Check the NVIDIA driver (nvidia-smi); 570 or later is recommended."
rm -rf "$UV_CACHE_DIR"                      # downloaded packages, no longer needed
echo "  Python $("$PY" -c 'import sys; print(sys.version.split()[0])'), PyTorch $("$PY" -c 'import torch; print(torch.__version__)')"

# --- 4. weights ----------------------------------------------------------------
verify_weights() { (cd "$CODE" && $SHA256 -c --quiet "$HERE/weights.sha256") > /dev/null 2>&1; }
if verify_weights; then
    say "Weights: already in place and checked"
else
    if [ -n "$WEIGHTS_FROM" ]; then
        say "Copying the weights from $WEIGHTS_FROM"
        for d in weights vfms; do
            rsync -a --exclude '.cache' --exclude '__pycache__' "$WEIGHTS_FROM/$d/" "$CODE/$d/" \
                || fail "copy failed."
        done
    else
        say "Downloading the weights (about 5 GB; this takes a while)"
        HF_HUB_DISABLE_TELEMETRY=1 "$PY" "$CODE/scripts/download_weights.py" \
            || fail "weight download failed. Run this again to resume."
    fi
    say "Checking the weights"
    (cd "$CODE" && $SHA256 -c --quiet "$HERE/weights.sha256") \
        || fail "the weights don't match the tested version."
fi

# --- 5. check --------------------------------------------------------------------
say "Loading the model on $GPU (a few seconds)"
RESULT=$("$PY" "$WORKER" check "$CODE" 2> "$FOLDER/setup-check.log" | tail -1)
case "$RESULT" in
    *'"ok": true'*) ;;
    *) fail "the model didn't load. Details: $FOLDER/setup-check.log" ;;
esac
"$PY" - "$RESULT" <<'EOF'
import json, sys
r = json.loads(sys.argv[1])
print(f"  {r['mac']}: {r['gpu_gb']} GB for the GPU, precision {r['precision']}, "
      f"window up to {r['hd_window']} frames at HD.")
EOF
printf '{"setup": "%s", "commit": "%s"}\n' "$(date '+%Y-%m-%d %H:%M')" "$COMMIT" > "$FOLDER/ck2_setup.json"

say "Done. CorridorKey 2 is ready."
if [ "$FOLDER" != "$HOME/CorridorKey" ]; then
    echo "  In the node's Setup, set Model Folder to $CODE"
    echo "  and Python to $PY"
fi
