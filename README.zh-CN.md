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

4. `空选区只截当前屏幕`
- 矩形区域截图未拖选时，回车、双击或确认只截鼠标所在屏幕，不再合并整个多屏桌面。
- 手动框选（包括跨屏选区）、`R` 恢复选区和“整个桌面”截图保持原行为。

## 补丁列表

`PKGBUILD` 会在上游 tarball（当前为 Plasma 6.7.5）上应用这些补丁：

- `copy-file-uri.patch`
- `game-mode-shortcut-suppression.patch`
- `restore-last-selection-rect.patch`
- `empty-selection-current-screen.patch`
- `ocr-pin-smart-regions.patch`

原 `geometry-inverted-bounds.patch` 已删除：上游 6.7.x 自带同样的修复
（`Geometry::rectBounded` 不再使用会触发断言的 `std::clamp`）。

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

## OCR、钉图与智能选区

- **OCR**：工具栏“提取文字”会识别当前截图或选区，复制文本并显示可编辑的结果窗口。空结果和缺失的语言包会给出提示。开启“提取后关闭”时，成功后只保留通知；失败或未发现文字仍会显示结果窗口。
- **中英日混合识别**：默认使用本地 PP-OCRv5，单个模型同时识别简中、繁中、英文和日文，避免多个 Tesseract 模型互相干扰。模型随 Arch 包安装，运行时不下载模型、不上传截图。原 Tesseract 引擎保留在导出菜单及结果窗口的手动语言选项中，设置页中的语言选择适用于手动模式。
- **Tesseract 用户语言包目录**：优先使用 `TESSDATA_PREFIX`，其次是 `~/.local/share/spectacle/tessdata/`，再查找系统数据目录。自定义目录应包含需要使用的全部 `.traineddata` 模型。推荐官方 [tessdata_fast](https://github.com/tesseract-ocr/tessdata_fast) 模型。
- **钉住截图**：框选后点击图钉按钮或按 `Ctrl+Shift+P`。查看器中也有图钉按钮。可同时钉住多张截图，在当前屏幕内拖动、滚轮缩放，`Ctrl+C` 复制原图，双击或 `Esc` 关闭；右键菜单可以恢复原始大小。Wayland 使用 LayerShell 置顶。
- **智能选区**：进入矩形截图后，将鼠标放在目标内容上，按 `Tab` 从小到大切换候选，`Shift+Tab` 反向切换。顶部显示候选序号；确认前仍可拖动或用方向键微调。检测在后台运行，使用本地 OpenCV 分析边框和内容块，包含当前屏幕作为后备候选。它不读取应用控件树，也不是语义目标识别；复杂或无边框画面仍可能需要手动框选。
- OCR 和钉图不触发普通截图的自动保存、复制图片和保存后退出流程，避免打断识别或覆盖识别出的文本。

这些变更通过 `ocr-pin-smart-regions.patch` 应用到打包源码。开发目录和打包源码分别构建验证；新增 `ocr_test`、`region_detector_test`、`gui_workflow_test` 覆盖多语言识别、初始化、语言切换、空结果、候选坐标和钉图基本交互。识别集成测试需要对应语言包。

### 6.7.5-3 修复验证

- 钉图的 Wayland 位置变更会提交到合成器，支持拖动、双击关闭、获得焦点后 Esc 关闭，以及新钉图出现后直接 Esc 关闭。
- 结果窗口可对同一张原图重新识别，也可临时指定 Tesseract 语言；重试不会读取后来拍摄的另一张图。
- `multilingual_ocr_test` 使用包含中英日和繁中的混排段落，覆盖 16 像素小字和深色背景，并检查文字错误率。模型资产目录可通过 `SPECTACLE_OCR_ASSETS` 指定；已安装本包时使用 `/usr/share/spectacle/ocr`。
- `tests/run-wayland-tests.sh` 在独立 KWin 会话内测试真实合成器输入，包括拖动后的双击命中与 Esc；不会向当前桌面注入输入。
- RapidOCR 和无界面的 Python OpenCV 使用固定版本的私有库，避免影响系统 C++ OpenCV。依赖及模型的来源、SHA-256 在 `PKGBUILD` 中，许可证说明在 `src/Ocr/THIRD_PARTY_NOTICES`。

### 6.7.5-4：英文识别与多屏钉图

- 默认识别会为纯英文行使用 PP-OCRv5 英文专用模型，改善单词空格、大小写、标点和小字识别；中日混排行继续使用多语言模型。英文回归测试包含 14 像素小字，检查空格和标点，不再只比较去空格后的字符。
- 钉图使用 KWin 的 `utility` 类型，不作为普通窗口进入 Karousel 的平铺规则；不修改用户的 Karousel 配置。
- 钉图在创建时显式绑定截图所在输出，使用该输出的逻辑坐标定位。双屏 150% / 125% 缩放测试直接核对 KWin 报告的输出名称、坐标、尺寸和窗口类型。
- `tests/run-wayland-tests.sh` 会在独立测试环境加载本机已安装的 Karousel；也可用 `SPECTACLE_TEST_KAROUSEL_DIR` 指定其目录。测试需要 `kscreen-doctor`。

### 6.7.5-5：保留 OCR 原始行结构

- 根据文字框的基线和位置合并同一视觉行，不再把每个识别框直接输出成一行。英文命令的多个参数会作为完整一行重新识别。
- 输出保留相对缩进和段落间距；相距较远的文字列不合并。新增几何排版测试，以及带语法颜色、分散文字框的命令截图回归测试。

### 6.7.5-6：补充智能选区的内容块

- 每块显示器独立检测，避免把整个高分屏桌面缩小后漏掉小内容。快速边框检测增加低对比度面板支持，并保留大面板，避免被大量小轮廓挤出候选列表。
- 后台使用本地文字检测模型补充无边框的文字行、段落、代码区域和连续内容块，不执行文字识别。候选只在所属屏幕内生成。
- 精确文字区域会替换重叠的粗略碎块；`Tab / Shift+Tab` 继续切换从小到大的候选。后台结果到达时保留当前选区，不会打断手动调整。
- 新增拥挤画面、大面板、浅色圆角面板、多屏小区域和无边框段落的回归测试。
