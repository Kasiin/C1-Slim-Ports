# C1-Slim 获取 root ADB

本目录提供一个 Windows 单文件工具，用于在本人拥有或已获明确授权的快易典 C1-Slim / MP-D261 上开启当前开机周期的 root ADB。

> 原始方案与脚本作者：[fwz233-RE](https://github.com/fwz233-RE)。本仓库在保留作者署名的基础上整理了安装步骤，并在公开版中移除了样机 MAC、序列号和 HTTP 明文日志。

## 适用范围与风险

已实机验证的组合如下：

| 项目 | 已验证值 |
| --- | --- |
| 设备 | 快易典 C1-Slim / MP-D261 |
| 固件 | `V0.82_MP-D261_20260713.170000` |
| 主机 | Windows 10/11 x64 |
| Python | 3.12 |
| 结果 | `adb shell id` 返回 `uid=0(root) gid=0(root)` |

其他固件和硬件修订版不保证适用。此方法会绕过设备向厂商服务请求 ADB 放行的在线判断，本地返回一次成功结果；它不刷写固件，也不自动写入设备根文件系统。

root ADB 没有普通用户权限边界，误删文件、写错分区或改坏启动脚本都可能导致设备无法启动。取得权限后应先做只读检查和备份。在没有完整分区备份及可靠恢复办法前，不要写 eMMC、修改分区表或强行跳过固件哈希检查。

## 1. 准备电脑

需要：

- 一台带 Wi-Fi、可开启“移动热点”的 Windows 10/11 x64 电脑；
- Python 3.12；
- [Android SDK Platform-Tools](https://developer.android.com/tools/releases/platform-tools)；
- 支持数据传输的 USB 线；
- 本目录中的 `adb_admit_local.py` 和 `requirements.txt`。

先在普通 PowerShell 中确认 Python 与 ADB：

```powershell
py -3.12 --version
adb version
```

进入本目录，建立独立 Python 环境并安装依赖：

```powershell
cd "C:\你的目录\C1-Slim-Ports\tools\root-adb"
py -3.12 -m venv .venv
.\.venv\Scripts\python.exe -m pip install --upgrade pip
.\.venv\Scripts\python.exe -m pip install -r .\requirements.txt
```

`pydivert` 会使用 WinDivert 驱动，脚本还需要监听本机 TCP 443 并添加临时防火墙规则，因此实际运行脚本时必须使用管理员 PowerShell。

## 2. 让设备连接电脑热点

1. 打开 Windows“设置 → 网络和 Internet → 移动热点”。
2. 将网络频带设为 **2.4 GHz**，设置临时热点名称和强密码，然后开启热点。
3. 在 C1-Slim 上连接这个热点。不要让设备继续连接家庭路由器。
4. 调试时最好只让 C1-Slim 连接该热点；若还有其他客户端，记下 Windows 热点页面显示的 C1-Slim IPv4 地址。
5. 用数据线连接 C1-Slim 与电脑。

不要给热点网卡手动添加厂商服务器的公网 IP，也不要修改系统 DNS 或 `hosts`。本工具使用 WinDivert 做临时、精确范围的本机转发，不需要这些旧式设置。

## 3. 启动工具

右键 PowerShell，选择“以管理员身份运行”，再执行：

```powershell
cd "C:\你的目录\C1-Slim-Ports\tools\root-adb"
.\.venv\Scripts\python.exe .\adb_admit_local.py
```

当 C1-Slim 是热点唯一客户端时，工具会自动发现它。若提示找到多个客户端，使用热点页面显示的设备地址：

```powershell
.\.venv\Scripts\python.exe .\adb_admit_local.py --device-ip 192.168.137.123
```

也可以指定设备 MAC：

```powershell
.\.venv\Scripts\python.exe .\adb_admit_local.py --device-mac AA-BB-CC-DD-EE-FF
```

地址和 MAC 只是格式示例，不要照抄。查看所有参数：

```powershell
.\.venv\Scripts\python.exe .\adb_admit_local.py --help
```

正常启动会依次出现类似信息：

```text
FIREWALL_ADDED ...
HTTPS_READY ...
REDIRECT_READY ...
READY open About-device and press keyboard Enter 10 times within 5 seconds
```

只有看到 `READY` 后，才继续操作设备。

## 4. 在设备上触发隐藏入口

1. 打开 C1-Slim 的“关于设备”。
2. 将焦点停在显示静态版本信息的项目上。
3. 在约 5 秒内快速、完整地按下并松开 `Enter` 10 次。

固件按按键松开事件计数，所以每次都要完整按下并松开。`C → S → Enter/OK` 进入的是 GCTest，不是 ADB 入口。

成功时电脑端会显示：

```text
TLS_OK ...
HTTP_REQUEST ... path='/v1/pens/<redacted>' ...
APPROVED ... response_status='success'
FIREWALL_REMOVED ...
STOPPED cleanup_complete=true ...
```

随后设备会重新建立 USB gadget，Windows 可能短暂提示 USB 断开并重新连接，这是正常现象。默认模式批准一次后会自动退出并清理；不要在它显示清理完成前强制关闭窗口。

## 5. 验证 root ADB

等待 USB 重新枚举后，在 PowerShell 中执行：

```powershell
adb kill-server
adb start-server
adb devices -l
adb shell id
```

成功判据是设备状态为 `device`，并且最后一条命令包含：

```text
uid=0(root) gid=0(root)
```

可以继续做只读确认：

```powershell
adb shell uname -a
adb shell mount
```

不要把某个固定的 `MagicPen-*` 序列号当作成功条件，每台设备的序列号不同。

## 6. ADB 是否会永久保留

此步骤只放行当前开机周期的 ADB。停止电脑端工具不会立刻关闭已经枚举的 ADB，但原厂启动流程在设备重启后通常只启用 MTP，因此 ADB 预期会恢复为关闭状态。

让 ADB 开机常驻需要另行修改设备的 USB 启动脚本，属于持久化系统修改，不是本工具的一部分。只有在已经验证 root、备份原文件、核对固件哈希并准备好恢复方法后才应考虑。

## 7. 清理与隐私

程序退出时会关闭 WinDivert、HTTPS 监听和临时证书，并删除本次创建的防火墙规则。元数据日志写到同目录的 `adb-admit.log`，该文件被仓库忽略；公开版不会记录 HTTP 头、Cookie、请求体、设备标识或查询参数值。

如进程被强制结束，可在管理员 PowerShell 检查本工具的临时规则：

```powershell
Get-NetFirewallRule -Name 'C1Slim-AdbAdmit-*' -ErrorAction SilentlyContinue
```

确认它确实是本工具遗留后再删除：

```powershell
Get-NetFirewallRule -Name 'C1Slim-AdbAdmit-*' -ErrorAction SilentlyContinue |
    Remove-NetFirewallRule -ErrorAction SilentlyContinue
```

不要删除不认识的防火墙规则或网络地址。

## 8. 常见问题

### `active mobile hotspot not found`

先确认移动热点已经开启。若自动发现失败，查看网卡和 IPv4 地址：

```powershell
Get-NetAdapter -IncludeHidden
Get-NetIPAddress -AddressFamily IPv4 | Sort-Object InterfaceIndex
```

找到热点网关及接口编号后显式指定：

```powershell
.\.venv\Scripts\python.exe .\adb_admit_local.py `
    --hotspot-ip 192.168.137.1 --interface-index 23
```

`23` 只是示例，必须换成自己电脑上的值。

### `no device found` 或 `multiple hotspot clients found`

确认设备连接的是电脑热点。断开其他热点客户端，或把 Windows 热点页面中的 C1-Slim IPv4 传给 `--device-ip`。

### TCP 443 被占用

查看占用程序：

```powershell
Get-NetTCPConnection -LocalPort 443 -State Listen |
    Select-Object LocalAddress, LocalPort, OwningProcess
```

关闭对应的本地 Web 服务、代理或调试程序后重试，不要随意结束系统进程。

### 看不到 `REDIRECT_READY`

确认 PowerShell 以管理员身份运行、依赖安装成功，并检查安全软件是否阻止 WinDivert。VPN、代理和其他抓包软件也可能与转发冲突。

### 已显示 `READY`，设备操作后却没有请求

确认设备仍连接电脑热点、焦点位于正确的版本信息项目，并在约 5 秒内完成 10 次完整的 Enter 按下/松开。若 DNS 解析或固件接口已经改变，此已验证方案可能不再适用，不要盲目扫描厂商服务器。

### `adb devices` 为空、`offline` 或 `unauthorized`

重新插拔数据线并重启 ADB server；确认数据线支持数据传输，并在 Windows 设备管理器中检查 ADB 接口驱动。`unauthorized` 时查看设备屏幕是否出现主机授权提示。

## 工作原理摘要

已验证固件的隐藏入口会向 `api.mpen.com.cn` 发送 `adbAdmit` HTTPS 请求。工具只拦截所选热点客户端发往该主机当前 IPv4、TCP 443 的流量，只接受严格匹配的 GET 路径和查询字段，并在电脑本机返回固件预期的成功 JSON。由于该固件客户端没有正确校验证书链和主机名，本机临时自签名证书能够完成 TLS。

工具不会修改 DNS、`hosts`、设备 CA、固件或设备文件，也不会访问或探测其他厂商接口。
