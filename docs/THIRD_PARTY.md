# 第三方资料与许可

本仓库原创代码默认使用根目录的 GPL-3.0。下列组件或数据保留各自许可：

- **C1ancher**：Terminal 的显示布局和键盘映射曾参考 fwz233-RE 的 GPL-3.0 项目；仓库只保留必要的分析/补丁工具和归属说明。
- **lavax_vm**：来自 `zhiyb/lavax_vm`，其 MIT 许可证保留在 `C1LavaX/port/third_party/lavax_vm/LICENSE`。
- **creack/pty、hinshun/vt10x**：Terminal 使用的 MIT 依赖，许可证随 vendored 源码保留。
- **GNU Unifont 16.0.01**：数独界面字形及部分生成字形来源。Unifont 采用 SIL OFL 1.1 与 GPL-2.0-or-later + GNU Font Embedding Exception 双重许可；许可证副本位于 `THIRD_PARTY_LICENSES/`。
- **文泉驿点阵宋体**：每日简报的 14/15 px 字形来源，原字体与上游许可说明保留在 `C1News/assets/`。
- **Fusion Pixel Font 12px**：圣经正文的主要字形来源，采用 SIL OFL 1.1；字体及各上游组件的许可副本保留在 `C1Bible/assets/fusion-pixel-12px/`。和合本中 Fusion 尚未覆盖的 15 个冷僻字使用文泉驿点阵宋体补齐。
- **中文和合本 CUVS**：圣经构建时从 `midvash/bible-data` 获取。该数据集把 1919 简体和合本标为 public domain，同时特别提示部分地区可能有额外权利主张；使用者应根据所在地判断。仓库不直接提交生成后的全文数据库。
- **新闻源**：每日简报运行时读取 60s、少数派和爱范儿公开接口/Feed；文章版权属于原作者与媒体，仓库不打包文章缓存。
- **蛙蛙富翁、魔塔及其他 LAVA 游戏资源**：本仓库不分发游戏程序和数据文件。运行时适配代码不代表拥有游戏内容版权，使用者需自行提供合法副本。
- **原厂固件与 launcher**：不在仓库中分发；所有补丁脚本都要求对使用者自己的设备文件进行哈希校验。

上游链接与更详细说明见各应用 README。若发现归属或许可信息有误，请通过 Issue 指出。
