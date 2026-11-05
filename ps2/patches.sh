#!/bin/sh
# The PS2 changes of the submodules, kept as patches (the submodules stay at
# upstream's commits): ps2/patches/<name>.patch for 3rdparty/<name>
# (mainui/miniutl, a submodule of mainui: miniutl.patch).
#
#   ps2/patches.sh apply     applies each patch, unless it is applied already
#                            (the Makefile does this before building)
#   ps2/patches.sh export    writes each submodule's changes (git diff) to its
#                            patch, after editing them
set -e
cd "$(dirname "$0")/.."
SUBMODULES="library_suffix hlsdk-portable mainui mainui/miniutl"

case "$1" in
apply)
	for s in $SUBMODULES; do
		p="$PWD/ps2/patches/${s##*/}.patch"
		[ -s "$p" ] || continue
		if git -C "3rdparty/$s" apply --reverse --check "$p" 2>/dev/null; then
			continue # applied
		fi
		git -C "3rdparty/$s" apply "$p"
		echo "patches.sh: applied ${s##*/}.patch"
	done ;;
export)
	for s in $SUBMODULES; do
		p="ps2/patches/${s##*/}.patch"
		git -C "3rdparty/$s" diff --ignore-submodules=all > "$p"
		[ -s "$p" ] || rm -f "$p"
	done ;;
*)
	echo "usage: $0 apply|export" >&2
	exit 64 ;;
esac
