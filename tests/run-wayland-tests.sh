#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-2.0-or-later
# Use a private compositor and desktop entry; never inject into the user's session.
set -euo pipefail
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
test_binary=$(realpath "${1:-$script_dir/../build-dev/bin/gui_workflow_test}")
scratch=$(mktemp -d /tmp/spectacle-wayland-test.XXXXXX)
trap 'rm -rf -- "$scratch"' EXIT
mkdir -p "$scratch/runtime" "$scratch/share/applications" "$scratch/cache" "$scratch/config"
karousel_dir="${SPECTACLE_TEST_KAROUSEL_DIR:-${XDG_DATA_HOME:-$HOME/.local/share}/kwin/scripts/karousel}"
if [[ -d "$karousel_dir" ]]; then
    mkdir -p "$scratch/share/kwin/scripts"
    cp -a -- "$karousel_dir" "$scratch/share/kwin/scripts/karousel"
    cat > "$scratch/config/kwinrc" <<'CONFIG'
[Plugins]
karouselEnabled=true
[Script-karousel]
tiledDesktops=.*
CONFIG
    export SPECTACLE_TEST_KAROUSEL=1
fi
chmod 700 "$scratch/runtime"
wayland-scanner client-header /usr/share/plasma-wayland-protocols/fake-input.xml "$scratch/fake-input.h"
wayland-scanner private-code /usr/share/plasma-wayland-protocols/fake-input.xml "$scratch/fake-input.c"
cc -I"$scratch" "$script_dir/WaylandInput.c" "$scratch/fake-input.c" -lwayland-client -lm -o "$scratch/input"
cat > "$scratch/share/applications/input.desktop" <<DESKTOP
[Desktop Entry]
Type=Application
Name=Spectacle isolated input test
Exec=$scratch/input
NoDisplay=true
X-KDE-Wayland-Interfaces=org_kde_kwin_fake_input
DESKTOP
export XDG_RUNTIME_DIR="$scratch/runtime"
export XDG_DATA_HOME="$scratch/share"
export XDG_CACHE_HOME="$scratch/cache"
export XDG_CONFIG_HOME="$scratch/config"
export QT_QPA_PLATFORM=wayland
export SPECTACLE_TEST_WAYLAND_INPUT="$scratch/input"
dbus-run-session -- kwin_wayland --virtual --width 1280 --height 900 --output-count 2 --scale 1.25 --no-lockscreen \
    --no-global-shortcuts --no-kactivities --socket spectacle-test \
    --exit-with-session "$test_binary compositorDragAndClose compositorScreenAndWindowType"
