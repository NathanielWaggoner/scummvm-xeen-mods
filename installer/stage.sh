#!/bin/bash
# Refreshes installer/stage from the current build: a stripped exe, the DLLs it needs and
# the licence text. Run from an MSYS2 MINGW64 shell, then compile xeen-mods.iss with ISCC.
cd "$(dirname "$0")/.." || exit 1
mkdir -p installer/stage
strip -o installer/stage/scummvm.exe build/scummvm.exe
ldd build/scummvm.exe | grep -i mingw64 | awk '{print $3}' | sort -u | while read -r f; do cp "$f" installer/stage/; done
cp COPYING installer/stage/COPYING.txt
ls installer/stage | wc -l
