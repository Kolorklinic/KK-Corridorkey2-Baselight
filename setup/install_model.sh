#!/bin/bash
# CorridorKey 2 for Baselight - opened in a terminal window by Setup > Install Model.
# Can also be run by hand: bash /usr/OFX/Plugins/CorridorKey2.ofx.bundle/Contents/Resources/setup/install_model.sh
clear
bash "$(dirname "$0")/ck2_setup.sh" "$@"
echo
read -n 1 -s -r -p "Press any key to close this window."
