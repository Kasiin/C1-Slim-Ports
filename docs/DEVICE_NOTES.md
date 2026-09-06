# 设备与恢复说明

## 已观察到的硬件接口

- 屏幕：296×152，纯一位黑白，无灰阶。
- framebuffer：`/dev/epaper_lcd`，一帧 5624 字节，按 8 行为一页纵向打包。
- 键盘：`/dev/input/event0`（matrix keypad）与 `/dev/input/event1`（gpio keys）。
- CPU/ABI：little-endian MIPS32r2，Linux hard-float ABI。
- 内存：约 50 MiB；大文本应使用 `mmap` 或流式读取。
- 可写大容量区：`/storage`；系统应用和 launcher 位于只读根分区。

以上信息来自当前测试机，并不保证所有批次完全一致。

## 电子纸刷新策略

`fast_refresh_only=1` 可以避免运行中频繁全刷，但快速刷新会逐渐积累残影。本项目采用三层限制：

1. UI 没有定时动画、闪烁光标或无意义重绘；
2. framebuffer 只有在字节内容发生变化时才提交；
3. LavaX 动画可启用稳定帧合并，把连续中间帧压缩为最终画面。

应用退出前直接恢复保存的桌面帧，再恢复原刷新参数，因此不会为了清屏插入反色或空白帧。

## launcher 生命周期

正常运行自定义应用时不重启 `mpenMain`：启动桥对现有进程发送 `SIGSTOP`，应用退出后发送 `SIGCONT`。这避免了“快易典 / 正在初始化”的启动画面，也避免 Home/Back 的释放事件落到原厂桌面后误开设置。

## 备份与恢复

每一步集成都把替换前文件放入 `/storage/c1/recovery/<app>/`。恢复前先核对文件哈希和目标路径，然后停止原厂 UI、以读写方式重新挂载根分区、恢复 launcher 与图标，最后重新挂载为只读并启动 UI。

不要删除恢复目录，不要混用不同固件版本的 launcher，也不要关闭安装脚本中的哈希保护。
