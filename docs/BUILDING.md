# 构建与安装

## 已验证环境

- Windows 10/11 与 PowerShell 7
- Python 3.12，Pillow
- Go 1.25（Terminal、每日简报）
- Zig 0.14.1（`mipsel-linux-musleabi` 静态交叉编译）
- Android platform-tools / `adb`
- 一台已授权 ADB、与本仓库记录的 MP-D261 固件相符的 C1 Slim

建议先安装依赖：

```powershell
python -m pip install Pillow pypinyin==0.55.0
```

各 PowerShell 构建脚本默认从 `PATH` 查找 `go`、`zig`、`python` 和 `adb`，也可以通过脚本参数传入完整路径。

## 推荐构建顺序

桌面补丁是按设备最终布局逐步叠加的，顺序不能随意交换：

1. Terminal（第 1 格，`T`）
2. 蛙蛙富翁（第 2 格，`W`）
3. 数独（第 3 格，`S`；自建改 `Z`）
4. 每日简报（第 4 格，`D`）
5. 魔塔（第 5 格，`M`；禁用自动更新检查）
6. 圣经（第 6 格，`H`）

后续脚本会检查前一步 launcher 的 SHA-256。不要用早期安装器覆盖后期版本。

## 可独立构建的程序

### Terminal

```powershell
cd C1Terminal
$env:GOOS='linux'; $env:GOARCH='mipsle'; $env:GOMIPS='hardfloat'; $env:CGO_ENABLED='0'
go build -trimpath -ldflags '-s -w -buildid=' -o build/c1term ./cmd/c1term
```

### C1LavaX

```powershell
.\C1LavaX\port\scripts\build-c1lavax.ps1
```

### 数独

```powershell
.\C1Sudoku\scripts\build.ps1
```

### 每日简报

```powershell
.\C1News\scripts\build.ps1
```

### 圣经

```powershell
python -m pip install -r .\C1Bible\requirements.txt
.\C1Bible\scripts\build.ps1
```

圣经构建器会从 `midvash/bible-data` 下载固定版本的 CUVS SQLite，校验 SHA-256 后生成适合小内存设备的只读映射数据库。可用环境变量 `C1BIBLE_SOURCE` 指定已下载的源文件。

## 私有文件

为了避免分发原厂程序或来源不明的游戏资源，集成构建需要使用者自行准备：

```text
private/
  mpenMain.terminal
  ic_desktop_ccyj.png
  mpenMain.pre-mota
  LVM.bin
  wawa-rich/
    Lava/蛙蛙大富翁.lav
    LavaData/RichMap.dat
    LavaData/RichPic.dat
```

这些路径已被 `.gitignore` 排除。也可用 `C1_RICH_LAUNCHER_BASE`、`C1_RICH_ICON_BASE`、`C1_LVM_FONT`、`C1_RICH_GAME` 和 `C1_LAUNCHER_BASE` 指向其他私有位置。

## 安装原则

安装器会：

1. 校验上传文件、当前 launcher 和图标哈希；
2. 在 `/storage/c1/recovery/` 保存恢复副本；
3. 把大文件放在 `/storage`，只把短启动桥放在 `/usr/data/<快捷键>`；
4. 临时停止原厂 UI，替换经过校验的 launcher 和图标；
5. 重新启动 UI 并执行自检。

在不完全理解脚本和设备分区前，请不要运行安装步骤。单独编译、测试和查看预览不会修改设备系统分区。
