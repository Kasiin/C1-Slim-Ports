# 让 root ADB 重启后永久保留

本教程是 [首次获取 root ADB](README.md) 的第二阶段。只有 `adb shell id` 已经返回 `uid=0(root)`，才可以执行这里的操作。

> 原始 root ADB 方案与脚本作者：[fwz233-RE](https://github.com/fwz233-RE)。永久化工具依据已验证固件的 USB 启动脚本制作，并保留安装前备份和卸载回滚路径。

## 重要风险

永久化后，C1-Slim 每次开机都会同时启用 ADB 与 MTP，而且当前固件的 ADB **没有主机认证**。任何能把数据线接到设备上的电脑都可以直接得到 root shell。

这不适合存放敏感数据或连接不可信电脑。公共充电口、陌生电脑和来历不明的 USB 主机都应视为高风险；只充电时建议使用物理断开数据线的数据屏蔽转接头。

工具只接受下面这个实机验证组合：

| 项目 | 已验证值 |
| --- | --- |
| 设备 | 快易典 C1-Slim / MP-D261 |
| device-tree | `ingenic,halley6_v20` |
| 固件 | `V0.82_MP-D261_20260713.170000` |
| 原厂 `S90usb` SHA-256 | `c2b278b283e9bf851461d9e8f6edfd207cec3b120585f0e091777d163562e965` |
| 修改后 `S90usb` SHA-256 | `626e4c5d600b543531337eb67220b0a7520d461211cc7ecafd36666d4f8905cb` |

哈希或型号不一致时脚本会拒绝执行。不要为其他固件删除校验、替换预期哈希或强行继续。

## 修改内容

原厂 `/etc/init.d/S90usb` 已经包含 ADB 启动调用，但在普通 `start` 分支中被注释。工具只进行一次精确变换：

```diff
-	#/etc/init.d/usb/adb	$1
+	/etc/init.d/usb/adb	$1
```

工具不会把原厂 `S90usb` 放进 GitHub。安装时会从使用者自己的设备临时读取文件、校验原厂 SHA-256、生成候选文件，再校验修改后 SHA-256。电脑临时副本在操作结束时删除。

写入前会把原文件分别备份到：

```text
/etc/init.d/S90usb.c1-original
/usr/data/c1/recovery/open-adb/S90usb.original
/storage/c1/recovery/open-adb/S90usb.original
```

恢复助手和哈希清单也会保存在后两个目录。写系统文件期间根文件系统才会临时重挂为可写；成功、失败或中断退出时都会尝试恢复为只读。目标文件和三个备份均经过 SHA-256 校验。

## 1. 开始前检查

保持设备供电稳定，只连接一台 ADB 设备，然后确认：

```powershell
adb devices
adb shell id
```

必须只有一行状态为 `device` 的设备，并看到：

```text
uid=0(root) gid=0(root)
```

如果还没有 root ADB，请先完整执行 [首次获取 root ADB](README.md)，不要直接运行永久化脚本。

## 2. 安装并冷启动验证

进入本目录，在 PowerShell 中执行：

```powershell
cd "C:\你的目录\C1-Slim-Ports\tools\root-adb"
.\install-persistent-adb.ps1 -Action Install -Reboot
```

若 PowerShell 执行策略阻止本地脚本，可以只对这一次调用使用：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File `
    .\install-persistent-adb.ps1 -Action Install -Reboot
```

脚本会依次完成：

1. 确认只连接一台设备且 ADB shell 为 root；
2. 核对 device-tree、根分区只读状态和 `/storage` 可写状态；
3. 核对设备上的原厂 `S90usb` SHA-256；
4. 从设备临时拉取原文件，只解除一处 ADB 启动注释；
5. 核对候选文件的固定 SHA-256；
6. 在系统分区、用户数据分区和 MTP 存储中创建原文件备份；
7. 原子替换启动脚本并重新将根文件系统设为只读；
8. 重启设备，等待同一个序列号以 root ADB 返回；
9. 验证 `adbd`、ADB/MTP USB functions、备份和文件哈希。

看到以下结果才算完整成功：

```text
open root ADB persistent state verified
Install completed.
```

设备重启后再人工确认一次：

```powershell
adb devices -l
adb shell id
```

## 3. 再次验证

以后可以只读验证当前文件和运行状态：

```powershell
.\install-persistent-adb.ps1 -Action Verify
```

同时进行一次冷启动验证：

```powershell
.\install-persistent-adb.ps1 -Action Verify -Reboot
```

本地运行证据保存在 `tools/root-adb/artifacts/`，该目录已被 `.gitignore` 排除，不会上传 GitHub。

## 4. 卸载并恢复原厂 MTP-only 启动

ADB 仍然可用时执行：

```powershell
.\install-persistent-adb.ps1 -Action Uninstall -Reboot
```

工具会同时校验三个备份，使用系统分区中的原始副本恢复 `/etc/init.d/S90usb`，确认原厂哈希并重新将根文件系统设为只读，然后重启。

重启后 ADB 消失是预期结果，电脑端无法再通过 ADB 自动完成最终验证。等待设备进入桌面后，确认 MTP 正常，并执行：

```powershell
adb kill-server
adb start-server
adb devices -l
```

正常情况下不应再列出这台 C1-Slim。以后需要调试时，可以重新使用临时 root ADB 流程。

卸载不会删除恢复目录和备份文件，避免在恢复完成后立即失去最后的副本。

## 5. 不使用电脑端包装器的验证与恢复

安装后，设备端助手位于：

```text
/storage/c1/recovery/open-adb/device-open-adb.sh
```

在已有 root ADB 的前提下，可以直接验证：

```powershell
adb shell "sh /storage/c1/recovery/open-adb/device-open-adb.sh verify c2b278b283e9bf851461d9e8f6edfd207cec3b120585f0e091777d163562e965 626e4c5d600b543531337eb67220b0a7520d461211cc7ecafd36666d4f8905cb"
```

或恢复原厂启动脚本：

```powershell
adb shell "sh /storage/c1/recovery/open-adb/device-open-adb.sh uninstall c2b278b283e9bf851461d9e8f6edfd207cec3b120585f0e091777d163562e965 626e4c5d600b543531337eb67220b0a7520d461211cc7ecafd36666d4f8905cb"
adb reboot
```

电脑端包装器会额外完成设备选择、型号核对、上传校验、自动重连及证据记录，正常情况下优先使用它。

## 6. 安装后 ADB 没有回来

先排除数据线、USB 端口和 Windows ADB 驱动问题。如果设备仍能正常进入桌面和联网，可以再次使用 [临时 root ADB 流程](README.md) 触发本次开机的 ADB，然后立即运行：

```powershell
.\install-persistent-adb.ps1 -Action Uninstall -Reboot
```

如果设备无法进入桌面、无法联网且没有 ADB，就不要反复重启或盲目写分区。此时只能通过已经验证过的 UART、恢复环境或完整系统恢复方案，把 `/etc/init.d/S90usb.c1-original` 恢复为 `/etc/init.d/S90usb`。没有经过验证的底层恢复手段时应停止操作。

## 文件说明

- `install-persistent-adb.ps1`：Windows 主机端安装、校验、重启验收和卸载包装器。
- `device-persistent-adb.sh`：上传到设备临时执行的事务助手；安装时会把副本写入恢复目录。
- `README.md`、`adb_admit_local.py`：首次获取当前开机周期 root ADB 的教程和工具。
