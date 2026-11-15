#!/bin/bash
# ---------------------------------------------------------------------------
# CorridorKey 2 for Resolve - builds OpenEXR (with Imath) as static libraries
# From Creative Video Tips (CVT)  |  https://creativevideotips.com
# Licence of this script: CC BY-NC-SA 4.0 with the CorridorKey additional terms.
#
# The plugin reads and writes DWAB-compressed EXRs, so it links OpenEXR
# (BSD-3-Clause) and Imath (BSD-3-Clause) into itself: a plugin can't rely on
# the system's copies. Downloads pinned release archives, checks
# their SHA-256, builds static libraries into build/deps/install.
# Homebrew is ignored on purpose. Called by build.sh; skips if already built.
# ---------------------------------------------------------------------------
set -euo pipefail
cd "$(dirname "$0")/.."

DEPS="$PWD/build/deps"
PREFIX="$DEPS/install"
# Rebuilt when the build options change (the stamp names them).
STAMP="$PREFIX/.built-with-prefix-map"
[ -f "$PREFIX/lib/libOpenEXR-3_5.a" ] && [ -f "$STAMP" ] && exit 0
rm -rf "$PREFIX" "$DEPS/build-imath" "$DEPS/build-openexr"

IMATH_URL="https://github.com/AcademySoftwareFoundation/Imath/archive/refs/tags/v3.2.3.tar.gz"
IMATH_SHA="e10c12b3f21f45bf08e09d4215d9c7691368d747beebd840de0b6fefed2df9f8"
EXR_URL="https://github.com/AcademySoftwareFoundation/openexr/archive/refs/tags/v3.5.2.tar.gz"
EXR_SHA="85a291b9b8563fabef1aa3f6e912d744ce1d4b9fd5a8f978eda194cd51b15ae9"

mkdir -p "$DEPS/src"
if command -v sha256sum >/dev/null; then SHA256=sha256sum; else SHA256="shasum -a 256"; fi
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu)
# Linux: the plugin is the target. macOS: a development build, for the tests only.
if [ "$(uname)" = "Darwin" ]; then PLATFORM=(-DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 -DCMAKE_FIND_FRAMEWORK=NEVER); else PLATFORM=(); fi
fetch() {   # url sha name
    local f="$DEPS/src/$3.tar.gz"
    [ -f "$f" ] || curl --fail --silent --show-error --location --proto '=https' -o "$f" "$1"
    echo "$2  $f" | $SHA256 -c --quiet - || { echo "Checksum mismatch: $3"; rm -f "$f"; exit 1; }
    rm -rf "$DEPS/src/$3" && mkdir "$DEPS/src/$3"
    tar -xzf "$f" -C "$DEPS/src/$3" --strip-components 1
}
echo "== Building OpenEXR 3.5.2 and Imath 3.2.3 (static, once)"
fetch "$IMATH_URL" "$IMATH_SHA" imath
fetch "$EXR_URL" "$EXR_SHA" openexr

COMMON=(-DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_CXX_VISIBILITY_PRESET=hidden
        -DCMAKE_C_VISIBILITY_PRESET=hidden -DCMAKE_VISIBILITY_INLINES_HIDDEN=ON
        "-DCMAKE_IGNORE_PREFIX_PATH=/opt/homebrew;/usr/local" "${PLATFORM[@]}"
        -DCMAKE_INSTALL_PREFIX="$PREFIX"
        "-DCMAKE_C_FLAGS=-ffile-prefix-map=$HOME=~" "-DCMAKE_CXX_FLAGS=-ffile-prefix-map=$HOME=~")

cmake -S "$DEPS/src/imath" -B "$DEPS/build-imath" "${COMMON[@]}" \
      -DIMATH_INSTALL_PKG_CONFIG=OFF -DPYTHON=OFF > "$DEPS/imath.log"
cmake --build "$DEPS/build-imath" --target install -j "$JOBS" >> "$DEPS/imath.log"

cmake -S "$DEPS/src/openexr" -B "$DEPS/build-openexr" "${COMMON[@]}" \
      -DImath_DIR="$PREFIX/lib/cmake/Imath" -DOPENEXR_FORCE_INTERNAL_DEFLATE=ON \
      -DOPENEXR_FORCE_INTERNAL_OPENJPH=OFF -DOPENEXR_BUILD_TOOLS=OFF -DOPENEXR_INSTALL_TOOLS=OFF \
      -DOPENEXR_BUILD_EXAMPLES=OFF -DOPENEXR_INSTALL_PKG_CONFIG=OFF \
      -DOPENEXR_TEST_LIBRARIES=OFF -DOPENEXR_TEST_TOOLS=OFF -DOPENEXR_TEST_PYTHON=OFF \
      > "$DEPS/openexr.log"
cmake --build "$DEPS/build-openexr" --target install -j "$JOBS" >> "$DEPS/openexr.log"
cp "$DEPS/src/openexr/LICENSE.md" "$PREFIX/OpenEXR-LICENSE.md"
cp "$DEPS/src/imath/LICENSE.md" "$PREFIX/Imath-LICENSE.md"
cp "$DEPS/src/openexr/external/OpenJPH/LICENSE" "$PREFIX/OpenJPH-LICENSE.txt"
cp "$DEPS/src/openexr/external/deflate/COPYING" "$PREFIX/libdeflate-COPYING.txt"
touch "$STAMP"
echo "   OpenEXR built: $(ls "$PREFIX"/lib/*.a | xargs -n1 basename | tr '\n' ' ')"
