# Spectacle Patched

语言：简体中文 | [English](README.md)

这个仓库是 [KDE Spectacle](https://invent.kde.org/plasma/spectacle) 的 Arch Linux 打包分支。它跟随上游 Spectacle，并维护一组很小的本地工作流补丁。

## 这个 fork 改了什么

默认行为尽量保持上游一致。当前维护的差异是：

1. `游戏模式`
- 在 `设置 -> 常规` 中新增开关。
- 默认关闭。
- 启用后，Spectacle 会抑制自身全局快捷键，避免和游戏中的 `F1`-`F12` 等按键冲突。

2. `将已保存图片以文件 URI 复制到剪贴板`
- 在 `设置 -> 常规 -> 截图后` 中新增 `复制文件 URI 到剪贴板`。
- 复制的是规范 URI（例如 `file:///home/user/Pictures/Screenshots/shot.png`），而不是纯路径。
- 适用于需要 `text/uri-list` 的应用。

3. `恢复上次矩形截图区域`
- 在矩形区域选择界面按 `R`，恢复到上次确认截图的矩形区域。
- 只会选中区域，不会立即截图，因此仍可在确认前微调。
- 保存的是带小数的逻辑坐标，避免高 DPI 或分数缩放环境下因为取整产生 1-2 像素漂移。
- 空区域或已完全离开当前屏幕范围的历史区域会被忽略。

## 补丁列表

`PKGBUILD` 会在上游 tarball 上应用这些补丁：

- `copy-file-uri.patch`
- `game-mode-shortcut-suppression.patch`
- `restore-last-selection-rect.patch`

## 构建与安装（Arch Linux）

```bash
cd /path/to/this/repo
makepkg -si
```

## 说明

- 包名：`spectacle-patched`
- `provides/conflicts`：`spectacle`

## 上游版本更新时

1. 在 `PKGBUILD` 里更新 `pkgver`。
2. 重新执行 `makepkg -si`。
3. 如上游改到了相同代码路径，刷新补丁列表。

## 恢复官方包

```bash
sudo pacman -S spectacle
```

## 上游项目

- 上游仓库：https://invent.kde.org/plasma/spectacle
- KDE 缺陷跟踪：https://bugs.kde.org/
- 本仓库贡献说明：`CONTRIBUTING.md`
