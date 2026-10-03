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
- `ocr-pin-smart-regions.patch`

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

## OCR, pinned screenshots, and suggested regions

- **Extract Text** recognizes the screenshot or selected region, copies the text, and opens an editable result window. Empty results and unavailable language data are reported. “Close after extraction” keeps successful results in the notification; errors and empty results still open the result window.
- The default backend is local PP-OCRv5: one recognition model supports Simplified Chinese, Traditional Chinese, English, and Japanese together. Models ship with the Arch package; recognition does not upload screenshots or download models. Tesseract remains available through explicit language choices in the export menu and result window. Settings → General → OCR configures those manual Tesseract choices.
- Tesseract language data is searched in `TESSDATA_PREFIX`, then `~/.local/share/spectacle/tessdata/`, then standard system directories. A custom directory must contain all models needed for recognition. Official [tessdata_fast](https://github.com/tesseract-ocr/tessdata_fast) models are recommended.
- Click **Pin Screenshot** or press `Ctrl+Shift+P` in the region overlay to pin the selected image. The viewer also has a pin button. Multiple pins are supported. Drag within the current screen, scroll to zoom, press `Ctrl+C` to copy the original, and double-click or press `Esc` to close. The context menu restores actual size. Wayland pins use LayerShell to stay above ordinary windows.
- Point at content and press `Tab` to cycle from smaller to larger suggested regions; `Shift+Tab` cycles backward. The overlay shows the candidate index. Selections remain manually adjustable. Background OpenCV analysis finds borders and content blocks, with the current screen as a fallback. This is a local image heuristic, not semantic object recognition or application widget inspection.
- OCR and pinning bypass ordinary screenshot auto-save/copy/quit actions.

`ocr-pin-smart-regions.patch` applies these changes to the package's upstream tarball. Regression coverage includes `ocr_test`, `region_detector_test`, and `gui_workflow_test`; recognition integration cases require the relevant language packs.

### 6.7.5-3 regression checks

- Wayland pin movement now commits surface state. Double-click and Escape dismiss a pin; newly created pins request focus after the capture overlay closes.
- The result window can retry using the same original image, including manual language overrides.
- `multilingual_ocr_test` checks mixed scripts, 16-pixel text, and dark backgrounds. Set `SPECTACLE_OCR_ASSETS` to a directory containing the packaged `vendor/` and `models/` assets; the installed default is `/usr/share/spectacle/ocr`.
- `tests/run-wayland-tests.sh` exercises compositor-delivered drag/double-click/Escape input in a private KWin session.
- Pinned RapidOCR and headless OpenCV wheels are private to the helper; system C++ OpenCV is unchanged. URLs and SHA-256 checksums are in `PKGBUILD`; licenses are documented in `src/Ocr/THIRD_PARTY_NOTICES`.

### 6.7.5-4: English recognition and multi-monitor pins

- Latin-only English lines are refined with the PP-OCRv5 English model. Mixed Chinese/Japanese lines retain the multilingual decoder. English regression cases include 14-pixel text and count whitespace and punctuation errors.
- Pins use KWin's `utility` window type and are explicitly bound to the captured output before mapping. Karousel's normal-window tiling rules do not adopt them; user tiling configuration is unchanged.
- The native compositor test checks output, position, size and window type on two outputs at 150% and 125% scaling. The runner loads the installed Karousel into its private session when available; `SPECTACLE_TEST_KAROUSEL_DIR` can override its location. It requires `kscreen-doctor`.

### 6.7.5-5: Preserve OCR line layout

- Detector fragments on the same visual baseline are grouped into one line before recognition and export. English command arguments are recognized as a complete line.
- Output retains relative indentation and paragraph gaps; distant columns remain separate. Regression tests cover fragmented command boxes and an actual screenshot with syntax colors and spaced parameters.

### 6.7.5-6: Content-aware suggested regions

- Detect each output independently so multi-monitor downscaling does not erase small regions. The fast pass recognizes low-contrast panels and reserves space for larger panels in busy screenshots.
- Background local text detection adds borderless lines, paragraphs, code areas and larger contiguous blocks without recognizing the text. Candidates stay within their output.
- Precise text regions replace overlapping coarse fragments. Tab/Shift+Tab still cycles by size, and later detection results preserve the user's current selection.
- Regression coverage includes clutter, low-contrast panels, small regions on multiple outputs, and borderless text blocks.
