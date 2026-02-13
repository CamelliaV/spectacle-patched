# Spectacle Patched

语言：简体中文 | [English](README.md)

这个仓库是 [KDE Spectacle](https://invent.kde.org/plasma/spectacle) 的 Arch Linux 打包分支。

目标是尽量保持上游行为不变，仅加入两个分支特性。

## 分支特性（仅这两项新增）

1. `游戏模式`（新增）：启用后忽略 Spectacle 全部快捷键
- 在 `设置 -> 常规` 中新增开关。
- 默认关闭。
- 启用后，Spectacle 会抑制自身快捷键，避免和游戏中的 `F1`-`F12` 等按键冲突。

2. `将已保存图片以文件 URI 复制到剪贴板`（新增剪贴板选项）
- 在 `设置 -> 常规 -> 截图后` 中新增 `复制文件 URI 到剪贴板`。
- 复制的是规范 URI（例如 `file:///home/user/Pictures/Screenshots/shot.png`），而不是纯路径。
- 适用于需要 `text/uri-list` 的应用。

## 构建与安装（Arch Linux）

```bash
cd /path/to/this/repo
makepkg -si
```

## 说明

- 包名：`spectacle-patched`
- `provides/conflicts`：`spectacle`
- `PKGBUILD` 中应用的补丁：
  - `copy-file-uri.patch`
  - `game-mode-shortcut-suppression.patch`

## 上游版本更新时

1. 在 `PKGBUILD` 里更新 `pkgver`。
2. 重新执行 `makepkg -si`。
3. 如补丁应用失败，刷新补丁（`copy-file-uri.patch` 与 `game-mode-shortcut-suppression.patch`）。

## 恢复官方包

```bash
sudo pacman -S spectacle
```

## 上游项目

- 上游仓库：https://invent.kde.org/plasma/spectacle
- KDE 缺陷跟踪：https://bugs.kde.org/
- 本仓库贡献说明：`CONTRIBUTING.md`
