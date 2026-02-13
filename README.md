# Spectacle Patched

Language: English | [简体中文](README.zh-CN.md)

This repository is an Arch Linux packaging fork of [KDE Spectacle](https://invent.kde.org/plasma/spectacle).

The goal is to keep upstream Spectacle behavior, with only two fork-specific additions.

## Fork Features (Only Additions)

1. `Game Mode` (new): ignore all Spectacle hotkeys while enabled
- Adds a toggle in `Settings -> General`.
- Disabled by default.
- When enabled, Spectacle suppresses its hotkeys so games can use keys like `F1`-`F12` without interference.

2. `Copy Saved Image as File URI` (new clipboard option)
- Adds `Copy file URI to clipboard` in `Settings -> General -> After taking a screenshot`.
- Copies a proper URI (for example `file:///home/user/Pictures/Screenshots/shot.png`) instead of a plain path.
- Useful for apps that expect `text/uri-list`.

## Build and Install (Arch Linux)

```bash
cd /path/to/this/repo
makepkg -si
```

## Notes

- Package name: `spectacle-patched`
- Provides/conflicts with: `spectacle`
- Patches applied by `PKGBUILD`:
  - `copy-file-uri.patch`
  - `game-mode-shortcut-suppression.patch`

## Updating to New Upstream Release

1. Update `pkgver` in `PKGBUILD`.
2. Rebuild with `makepkg -si`.
3. Refresh patches if needed (`copy-file-uri.patch` and `game-mode-shortcut-suppression.patch`).

## Revert to Official Package

```bash
sudo pacman -S spectacle
```

## Upstream Project

- Upstream source: https://invent.kde.org/plasma/spectacle
- KDE bug tracker: https://bugs.kde.org/
- Contribution guide in this repo: `CONTRIBUTING.md`
