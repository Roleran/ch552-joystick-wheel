# CH552 Joystick Wheel

本仓库的固件用 PlatformIO 编译。需要已安装 [PlatformIO Core](https://platformio.org/)，命令是 `pio`。`HidKeyboardMouse` 是原作者用 Arduino（CH55xduino）写的参考代码，不要对它执行 `pio run`。

## 拉取和编译

```powershell
git clone https://github.com/Roleran/ch552-joystick-wheel.git
cd ".\ch552-joystick-wheel\firmware\ch552e-v1"
pio run
cd ".\ch552-joystick-wheel\firmware\ch552g-v2"
pio run
```

`pio run` 用 SDCC 把对应目录的 C 代码编成 Intel HEX，并检查程序没有写进 CH552 的 bootloader 区（`0x3800` 以上）。成功后的烧录文件：

- v1：`firmware/ch552e-v1/ch552e-v1.hex`
- v2：`firmware/ch552g-v2/ch552g-v2.hex`

`.pio` 是编译中间文件，已被 `.gitignore` 忽略，不要提交。

## 目录

- `firmware/ch552e-v1`：CH552E 单摇杆，PlatformIO 工程。
- `firmware/ch552g-v2`：CH552G 双摇杆，PlatformIO 工程。
- `firmware/软件架构.md`：给开发者看的实现说明。
- `HidKeyboardMouse`：原作者代码，只供对照。入口是 `HidKeyboardMouse.ino`，USB 协议在 `src/userUsbHidKeyboardMouse`。旁边的 `HidKeyboardMouse.ino.ch552.hex` 是 Arduino 编出来的，不是本仓库的 PlatformIO 产物。

## 使用说明

电脑把设备识别成一个 USB 鼠标键盘，不需要安装驱动。v1 是 CH552E 单摇杆，v2 是 CH552G 双摇杆。

## v1：单摇杆

上电默认是滚轮，灯为蓝色。摇杆按下一次切到鼠标，灯变为琥珀色。再按一次回到滚轮。只在按下的瞬间切换，按住不连跳。

滚轮时，上下决定滚动方向，左右只加快速度。只往左右扳不会滚动。越偏离中心越快：刚离开中心约 180ms 滚一格，只打满上下约 72ms，上下和左右都打满约 36ms。中心附近一小段没有动作。

鼠标时，摇杆移动指针，约每 16ms 更新一次。上下方向和屏幕一致；如果实际方向反了，需要改固件里的方向符号。

## v2：双摇杆和四个方向键

摇杆 1 一直是滚轮，快慢和 v1 相同。它的按下键只切换摇杆 2 那个键的功能。

摇杆 2 一直移动鼠标，上下和左右与 v1 的鼠标模式一致。它的按下键默认是鼠标左键，按住不放会一直按着；摇杆 1 切换之后，这个键变成回车，按住发送、松开抬起。切换时如果这个键还按着，会先松开，避免左键或回车卡住。这版没有灯，切换没有指示。

四个机械键始终是键盘方向键，不随模式改变：上、下、左、右。可以同时按住。

## 烧录

用 WCHISPTool，芯片选 CH552。

- v1 打开 `firmware/ch552e-v1/ch552e-v1.hex`。CH552E 的 V33 是第 10 脚，P3.6 是第 6 脚。
- v2 打开 `firmware/ch552g-v2/ch552g-v2.hex`。CH552G 的 V33 是第 16 脚，P3.6 是第 12 脚。

用 4.7k–10k 电阻把 V33 和 P3.6 短接，插上 USB，下载，去掉短接后再插一次。摇杆的 5V 要和芯片的 5V 接在一起。