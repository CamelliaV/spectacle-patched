# Spectacle Patched

Language: English | [简体中文](README.zh-CN.md)

This repository is an Arch Linux packaging fork of [KDE Spectacle](https://invent.kde.org/plasma/spectacle). It tracks upstream Spectacle and carries a small patch set for local workflow changes.

## What This Fork Changes

The fork keeps upstream behavior by default. Its maintained differences are:

1. `Game Mode`
- Adds a `Settings -> General` toggle.
- Disabled by default.
- When enabled, Spectacle suppresses its global shortcuts so games can use keys such as `F1`-`F12` without interference.

2. `Copy Saved Image as File URI`
- Adds `Copy file URI to clipboard` under `Settings -> General -> After taking a screenshot`.
- Copies a URI such as `file:///home/user/Pictures/Screenshots/shot.png` instead of a plain path.
- Intended for apps that consume `text/uri-list`.

3. `Restore Last Rectangular Region`
- In rectangular region selection, press `R` to restore the last accepted region.
- The region is only selected, not captured immediately, so it can still be adjusted before confirming.
- The saved region keeps fractional logical coordinates, so high-DPI or fractional-scale setups do not drift by rounding.
- Empty or fully off-screen saved regions are ignored.

4. `Current Screen for Empty Selections`
- In rectangular capture, accepting an empty selection with Enter, double-click, or the confirmation action captures the screen under the pointer instead of the entire virtual desktop.
- Explicit selections (including cross-screen regions), `R` restoration, and entire-desktop capture keep their existing behavior.

## Patch Set

`PKGBUILD` applies these patches on top of the upstream tarball (currently Plasma 6.7.5):

- `copy-file-uri.patch`
- `game-mode-shortcut-suppression.patch`
- `restore-last-selection-rect.patch`
- `empty-selection-current-screen.patch`

The former `geometry-inverted-bounds.patch` was dropped: upstream 6.7.x ships the same fix
(`Geometry::rectBounded` no longer uses the asserting `std::clamp`).

## Build and Install (Arch Linux)

```bash
cd /path/to/this/repo
makepkg -si
```

## Notes

- Package name: `spectacle-patched`
- Provides/conflicts with: `spectacle`

## Updating to New Upstream Release

1. Update `pkgver` in `PKGBUILD`.
2. Rebuild with `makepkg -si`.
3. Refresh the patch set if upstream touched the same code paths.

## Revert to Official Package

```bash
sudo pacman -S spectacle
```

## Upstream Project

- Upstream source: https://invent.kde.org/plasma/spectacle
- KDE bug tracker: https://bugs.kde.org/
- Contribution guide in this repo: `CONTRIBUTING.md`
