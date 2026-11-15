#!/bin/bash
# ---------------------------------------------------------------------------
# CorridorKey 2 for Baselight - install the plugin (asks for your password)
# By KolorKlinic, based on CorridorKey 2 for Resolve by Creative Video Tips.
# Licence: CC BY-NC-SA 4.0 with the CorridorKey additional terms (see LICENSE).
#
# Run it from the unpacked release (or after ./build.sh). On Baselight TWO/X,
# install on the node: FilmLight shares /usr/OFX/Plugins with the UI host.
# Quit Baselight first.
# ---------------------------------------------------------------------------
set -euo pipefail
cd "$(dirname "$0")"
B=CorridorKey2.ofx.bundle
[ -d "$B" ] || B=build/CorridorKey2.ofx.bundle
[ -d "$B" ] || { echo "No CorridorKey2.ofx.bundle here. Unpack the release, or build it: ./build.sh"; exit 1; }
DEST=${OFX_INSTALL_DIR:-/usr/OFX/Plugins}
sudo mkdir -p "$DEST"
sudo rm -rf "$DEST/CorridorKey2.ofx.bundle"
sudo cp -R "$B" "$DEST/"
echo "Installed CorridorKey 2 for Baselight into $DEST."
echo "Restart Baselight, add CorridorKey 2 (KolorKlinic) as an OFX effect, open Setup and press Install Model."
