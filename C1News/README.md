# C1News 每日简报 1.2.0

1.2.0 已于2026-09-05安装到设备。仅更新应用，1.1.0备份在 `/storage/c1/recovery/news/c1news.v1.1.0`；未重载桌面、未清空缓存。设备HTTPS刷新成功，少数派当前10篇均已补取公开正文（约3200至10300字符）。已检查15px正文显示、翻页、Home隔离及返回桌面。

1.2 新增：少数派从公开文章页面的可见正文区域补取内容（不登录、不调用受保护接口、不读取隐藏应用数据）；最多两个并行请求、单页18秒、补取阶段45秒上限；已有全文缓存复用，失败则保留摘要。更新后按R可补取少数派正文。60秒原本就是完整短讯，没有每条长文，阅读页标记“短讯”。

C1 Slim / MP-D261 原生独立程序，296×152 单色墨水屏。安装在 `/storage/c1news`，原厂桌面第四个图标（第二排第一个）显示“简报”，快捷键 D；第六个原厂 App 改为 A。终端 T、蛙蛙富翁 W、数独 S、自建 Z 均保留。

## 使用

- 栏目首页：方向键上下选择，Q/W/E 对应 60 秒、少数派、爱范儿；确认进入。
- 列表：上下选择，左右翻两条卡片，确认阅读。
- 正文：左右/上下翻页，确认下一页。
- Back/Esc 逐级返回，首页再按退出。N 回栏目首页。
- R 联网更新三个源，30 秒内不会重复请求。
- Home 不退出，不传给原厂桌面。

文章保留内容日期和来源。少数派优先补取公开页面正文，失败或受限时保留RSS摘要并明确标注。爱范儿优先使用 RSS 的 content:encoded 全文；不下载图片、视频，不绕过付费限制。60 秒是第三方汇编而非官方新闻供稿，短讯不会凭空扩写为长文。

## 数据源

- https://60s.viki.moe/v2/60s （公共实例限额，仅用于当前试用；长期可换服务实例）
- https://sspai.com/feed
- https://www.ifanr.com/feed

仅程序运行时联网，退出后无后台常驻进程。进入时缓存超过一小时才自动更新；也可以按 R。每个源独立请求，22 秒超时，最多 4 MiB 响应，出错保留旧缓存。当前保存每个源最近一期/一批文章，**没有七天历史浏览功能**。

缓存：`/storage/c1news/cache/{60s,sspai,ifanr}.json`，先校验再原子替换。后台下载完成后，只在返回栏目首页时更新内存列表，不打断正在读的文章。长文上限 30000 字符，超限明确标记。

## 屏幕与退出

- 大标题使用 Unifont 16px；按钮和列表保留14px界面字体。正文改为像素对齐的 WenQuanYi 15px字形：原字体1800单位em、100单位点阵网格，以18ppem渲染使每个点阵像素准确对应屏幕像素。正文宽272px，19px行距、另加6px段间距，分页不越过底部按钮。列表每页两条卡片，条目之间7px留白。
- 仅画面变化时写入，最少间隔 100ms，无动画、光标闪烁或周期性全刷。
- 沿用数独包装器：STOP 原 launcher，保存 5624 字节桌面帧；程序抓取两个输入设备；退出等待按键释放 150ms；写回桌面帧，恢复刷新设置，CONT 同一个 launcher。
- 不重启原 launcher，不发模拟按键恢复桌面。
- `fast_refresh_only=1`、`refresh_max=100` 只在应用内生效，退出恢复旧值。
- 私有 Mozilla CA 文件 `/storage/c1news/cacert.pem`，通过 SSL_CERT_FILE 使用；系统证书和 TLS 校验不变。来源 https://curl.se/docs/caextract.html ，MPL 2.0。

## 构建与测试

在工作区执行 `C1News/scripts/build.ps1`，依赖工作区 Go 工具链、Python312/Pillow、数独的 Unifont 和 assets/wqy14.ttf。字体生成脚本和全部源代码保留；Go 模块通过 go.sum 校验。x/net/html 使用 BSD-3-Clause，Unifont 沿用 GPL 字体例外。WenQuanYi 字体来自 https://github.com/AmusementClub/WenQuanYi-Bitmap-Song-TTF ，许可见 assets/WQY-LICENSE.txt（上游说明附带文档嵌入条款），原始TTF及生成脚本均保留。

桌面集成：先运行 tools/build_integration.py，再 scripts/deploy.ps1。安装器精确检查支持的 launcher、图标和上传文件哈希；安装前退出自定义程序。备份在 `/storage/c1/recovery/news`，包括集成前 launcher、第四图标和1.0试用程序。缓存不会被清空。只有安装时重载桌面，日常启动/退出不重载。

桌面补丁仅修改第四入口 on_click_suguo（0x70eb78）、对应名称、图标和快捷键表 `TWSAZD` → `TWSDZA`。从已有数独集成版本构建，不会覆盖终端、富翁、数独的补丁。

诊断命令（不会打开屏幕或输入设备）：

```
c1news --version
c1news --refresh-only CACHE_DIRECTORY
c1news --preview CACHE_DIRECTORY home OUTPUT.bin
c1news --preview CACHE_DIRECTORY article OUTPUT.bin
c1news --preview CACHE_DIRECTORY ifanr OUTPUT.bin
```

设备上更新诊断需 `SSL_CERT_FILE=/storage/c1news/cacert.pem`。正常运行必须经过 `/storage/c1news/launch-news.sh` 包装器。设置 C1NEWS_FRAME 可保存最后写屏帧到指定诊断路径，不额外刷屏。

2026-09-04 验证：Windows 单元测试（HTML/RSS/JSON、缓存、错误响应、分页无丢字/无空白尾页、导航、Home隔离、确定性渲染）通过；三个源在设备上 HTTPS 直连成功；真机确认正文翻页、阅读时 R 更新不改变当前帧、Home 不穿透、Back 退出状态0，launcher PID 24967 未变，前后桌面帧 SHA256 相同，刷新设置恢复0/30。网络掉线行为由错误处理和测试覆盖，没有为了测试关闭用户 Wi-Fi。

1.1.0 集成复测：D 从第四图标启动，桌面实际显示 D简报 / A单词；14px排版的无丢字、按钮宽度、正文边界、6px段间距测试通过，go vet通过。设备内完成选择爱范儿、进入正文、翻页、Home隔离、N回首页、Back退出，退出状态0，原桌面PID29363保持不变。原终端、富翁、数独二进制哈希与升级前一致。
