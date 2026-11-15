#!/bin/bash
# ---------------------------------------------------------------------------
# CorridorKey 2 for Baselight - build
# From Creative Video Tips (CVT)  |  https://creativevideotips.com
# Licence: CC BY-NC-SA 4.0 with the CorridorKey additional terms (see LICENSE).
#
# Builds build/CorridorKey2.ofx.bundle and runs the tests.
#   ./build.sh            build + C++ tests + EXR check against Python/OpenCV
# Needs a C++17 compiler, cmake, curl and uv (see .github/workflows/build.yml).
# ---------------------------------------------------------------------------
set -euo pipefail
cd "$(dirname "$0")"

VERSION=$(sed -n 's/^VERSION = "\(.*\)"/\1/p' worker/ck2_worker.py)
CXX_VERSION=$(sed -n 's/.*kVersion = "\(.*\)";/\1/p' src/ck2_core.h)
[ "$VERSION" = "$CXX_VERSION" ] || { echo "Version mismatch: worker $VERSION, plugin $CXX_VERSION"; exit 1; }
echo "== CorridorKey 2 for Baselight $VERSION"

third_party/build_openexr.sh
cmake -S . -B build/cmake -DCMAKE_BUILD_TYPE=Release > /dev/null
LOG=$(cmake --build build/cmake -j "$(nproc 2>/dev/null || sysctl -n hw.ncpu)" 2>&1) || { echo "$LOG" | tail -80; exit 1; }
echo "$LOG" | grep -E '(src|tests)/.*(warning|error)' || true   # our own code only

B=build/CorridorKey2.ofx.bundle
# OFX bundle layout: Linux-x86-64 on Linux (the target); MacOS for a macOS
# development build, where only the tests matter.
if [ "$(uname)" = "Darwin" ]; then ARCHDIR=MacOS; else ARCHDIR=Linux-x86-64; fi
rm -rf "$B"
mkdir -p "$B/Contents/$ARCHDIR" "$B/Contents/Resources/worker"
cp build/cmake/CorridorKey2.ofx "$B/Contents/$ARCHDIR/"
cp worker/ck2_worker.py worker/ck2_progress.py "$B/Contents/Resources/worker/"
# Model setup, with its own copy of uv (MIT / Apache-2.0) so nothing else is needed.
# UV_BIN / UV_LICENSES override where uv and its licence files are found.
UV_BIN=${UV_BIN:-$(readlink -f "$(command -v uv)")}
UV_LICENSES=${UV_LICENSES:-$(dirname "$(dirname "$UV_BIN")")}
mkdir -p "$B/Contents/Resources/setup/bin" "$B/Contents/Resources/setup/wheels"
cp setup/ck2_setup.sh setup/install_model.sh setup/requirements-lock-linux.txt \
   setup/corridorkey_files.sha256 setup/weights.sha256 "$B/Contents/Resources/setup/"
chmod 755 "$B/Contents/Resources/setup/ck2_setup.sh" "$B/Contents/Resources/setup/install_model.sh"
cp setup/wheels/*.whl "$B/Contents/Resources/setup/wheels/"
cp "$UV_BIN" "$B/Contents/Resources/setup/bin/uv"
chmod 755 "$B/Contents/Resources/setup/bin/uv"
cp "$UV_LICENSES/LICENSE-MIT" "$B/Contents/Resources/setup/bin/uv-LICENSE-MIT"
cp "$UV_LICENSES/LICENSE-APACHE" "$B/Contents/Resources/setup/bin/uv-LICENSE-APACHE"
echo "   (bundled $("$UV_BIN" --version | cut -d' ' -f1-2))"
cp LICENSE THIRD_PARTY_NOTICES.md "$B/Contents/Resources/"
# The effect's icon in the effects list, named after the plugin ID.
cp installer/com.creativevideotips.corridorkey2.png "$B/Contents/Resources/"
cp third_party/openfx/LICENSE.md "$B/Contents/Resources/OpenFX-LICENSE.md"
cp build/deps/install/*-LICENSE.* build/deps/install/*-COPYING.* "$B/Contents/Resources/"
sed "s/@VERSION@/$VERSION/g" installer/Info.plist > "$B/Contents/Info.plist"
# Installed as root: everything must be readable (and folders/programs openable) by every user.
chmod -R u+rwX,go+rX,go-w "$B"
UNREADABLE=$(find "$B" ! -perm -o+r)
[ -z "$UNREADABLE" ] || { echo "Not readable by other users:"; echo "$UNREADABLE"; exit 1; }
if [ "$ARCHDIR" = "Linux-x86-64" ]; then
    # Only the two OFX entry points may be exported.
    EXPORTS=$(nm -D --defined-only "$B/Contents/$ARCHDIR/CorridorKey2.ofx" | awk '$2 ~ /[TW]/ {print $3}' | sort | tr '\n' ' ')
    [ "$EXPORTS" = "OfxGetNumberOfPlugins OfxGetPlugin " ] || { echo "Unexpected exports: $EXPORTS"; exit 1; }
else
    codesign --force --sign - "$B" > /dev/null 2>&1   # ad hoc
fi

echo "== Tests"
build/cmake/ck2_tests
PY=${CK2_PYTHON:-$HOME/CorridorKey/.venv/bin/python}
if [ -x "$PY" ]; then
    T=$(mktemp -d)
    build/cmake/ck2_tests write "$T/c.exr"
    "$PY" - "$T" build/cmake/ck2_tests <<'EOF'
import os, sys, subprocess
os.environ["OPENCV_IO_ENABLE_OPENEXR"] = "1"
import cv2, numpy as np
t, tool = sys.argv[1], sys.argv[2]
# 1. OpenCV reads what the plugin writes (staging frames).
img = cv2.imread(f"{t}/c.exr", cv2.IMREAD_UNCHANGED)[..., ::-1]
y, x = np.mgrid[:5, :7]
want = np.dstack([x * 0.1, y * 0.2, 1.5 + x - y]).astype(np.float32)
assert img.shape == (5, 7, 3) and np.abs(img - want).mean() < 0.02, "OpenCV read differs"
# 2. The plugin reads what the worker writes (half RGBA, uncompressed).
rgba = np.dstack([want, np.linspace(0, 1, 35).reshape(5, 7)]).astype(np.float32)
import OpenEXR   # the worker writes the cache with it (DWAB 45)
with OpenEXR.File({"compression": OpenEXR.DWAB_COMPRESSION, "type": OpenEXR.scanlineimage,
                   "dwaCompressionLevel": 45.0},
                  {c: np.ascontiguousarray(rgba[..., i].astype(np.float16)) for i, c in enumerate("RGBA")}) as f:
    f.write(f"{t}/w.exr")
out = subprocess.run([tool, "read", f"{t}/w.exr"], capture_output=True, text=True).stdout.split("\n")
assert out[0] == "7 5", out
for line in out[1:4]:
    i, *v = line.split(); i = int(i)
    exp = rgba.reshape(-1, 4)[i].astype(np.float16).astype(np.float32)
    got = [float(a) for a in v]
    assert abs(got[3] - exp[3]) < 1e-6, ("matte must be exact", line, exp)   # alpha lossless
    assert np.allclose(got[:3], exp[:3], atol=0.05), (line, exp)
print("EXR exchange with OpenCV: passed")
# 3. The job file the node writes is valid JSON with what the worker expects.
import json
j = json.loads(subprocess.run([tool, "jobjson"], capture_output=True, text=True).stdout)
assert j["model_dir"] == '/m "q"' and j["window"] == 24 and j["input_linear"] is True, j
assert j["hint"] == {"mode": "first", "dir": "/h", "strength": 0.3, "carry": True}, j["hint"]
assert j["output_names"] == ["a.exr", "b.exr"]
assert j["private"] is True, j
print("Job file: passed")
EOF
    rm -rf "$T"
else
    echo "(skipped the OpenCV check: no Python at $PY)"
fi
echo "== Built $B"
