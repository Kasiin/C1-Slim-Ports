# C1 Slim 圣经

为 296×152 单色墨水屏制作的离线《圣经》和合本阅读器。

- 66 卷、1189 章、31021 节完整正文
- 目录、章节网格、断点续读
- 实体键盘拼音全文搜索（例如 `yesu`、`endian`）
- 只在画面变化时局部刷新；Home 被应用吞掉，Back 逐级返回/退出
- 原厂桌面第六格，快捷键 H

## 操作

- 目录：方向键选卷，Q/W 跳旧约/新约，Enter 选章，C 续读，F 搜索
- 选章：方向键移动，Enter 阅读，Back 回目录
- 阅读：左右翻页，PageUp/PageDown 换章，D 回目录，F 搜索，Back 回选章
- 搜索：字母输入拼音，Backspace 删除，Enter 搜索；结果中 Enter 跳到经文

正文构建自 `midvash/bible-data` 的 CUVS 数据集。源文件 SHA-256 固定为
`d08201e63895cebd335f8d9673b68d3c7642e108ebd4988ff59ed8453d88d30d`。

构建与安装：

```powershell
.\C1Bible\scripts\build.ps1
.\C1Bible\scripts\deploy.ps1
```
=