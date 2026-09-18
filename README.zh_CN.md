<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 用时记账（v0）

本仓库是 FoloToy AI Passport 的效率计时派生固件。设备上电后直接进入应用：
没有 demo 测试菜单，没有笔记、网络、蓝牙或语音。

## 功能

- 同一时间只跑一个计时器。标签固定为：实习 / 投递 / 生活 / 面试 / 学校 / 运动。
- **计时页**：当前标签、状态（未在记 / 计时中）和已用时间 `HH:MM:SS`。
- **今日页**：顶部「今日共 Xh Ym」，六行标签显示时长和占比条。`UP` / `DOWN`
  在「今日 / 昨天」之间切换。
- 进行中的会话以及今日/昨天各标签累计秒数写入 NVS。掉电后，下次开机按最后一次
  心跳时间自动结束该会话并保留记录。

电源键保持上游硬件电源键语义，固件不重新映射。

## 按键

| 操作 | 计时页 | 今日页 |
| --- | --- | --- |
| `UP` / `DOWN` 短按 | 仅在未计时时切换标签 | 切换今日 / 昨天 |
| `OK` 短按 | 开始或停止当前计时 | 忽略 |
| `OK` 长按 | 进入今日页 | 返回计时页 |

计时进行中不能改标签。

## 数据

应用在 NVS（`timetrack` / `state`）中维护本地秒时钟，不联网、不校准墙钟。
每 86400 个已保存秒滚动一天。运行中的会话大约每 10 秒写一次检查点，掉电后按
最后一次保存的时间戳收口。

## 构建与测试

本仓库没有 PC / QEMU 模拟器目标。请在主机和 ESP-IDF 5.5.3 固件构建上验证：

```bash
./tools/validate.sh --static
./tools/validate.sh --firmware
```

界面 16px 字库为 `assets/fonts/time_track_font_16.c`。改动
`main/time_track_text.h` 后执行 `python3 tools/gen_time_track_font.py` 重新生成。

## 源码位置

- `main/time_track_model.c`：可在主机测试的计时、日期与按键逻辑
- `main/time_track_store.c`：NVS 读写
- `main/time_track_ui.c`：深色 + 青绿界面，字号 16
- `main/main.c`：BSP 启动与按键任务
