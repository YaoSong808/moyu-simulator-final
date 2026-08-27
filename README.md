# 摸鱼模拟器最终版

**中文** | [English](README_EN.md)

一个轻量的 Windows 桌面小工具：启动后伪装成右下角广告弹窗，可以将其他桌面软件拖入其中，并在弹窗内继续点击、滚动、输入或播放内容。

## 功能特点

- 一键从启动页切换成右下角广告弹窗
- 直接拖动其他软件的标题栏，松手后自动嵌入
- 嵌入的软件保持可交互，无需截图或视频转播
- 检查窗口是否真正进入内容区；失败时自动还原，避免只剩空白区域
- 支持“还原窗口”，退出模拟器时也会自动恢复原窗口
- 原生 Win32 单文件程序，不联网、不读取窗口内容、不收集数据

## 下载

前往 [Releases](https://github.com/YaoSong808/moyu-simulator-final/releases/latest) 下载 `MoyuSimulatorFinal-v1.0.0-win-x64.exe`。下载后可自行重命名为 `摸鱼模拟器最终版.exe`，不影响运行。

系统要求：Windows 10/11，64 位。程序无需安装，下载后双击即可运行。

> 程序暂未使用商业代码签名证书。Windows 首次运行时如果显示 SmartScreen 提示，请核对下载来源后选择“更多信息 → 仍要运行”。

## 使用方法

1. 双击运行 `摸鱼模拟器最终版.exe`。
2. 点击“开始摸鱼”，窗口会变成右下角广告弹窗。
3. 抓住抖音、视频播放器或其他桌面软件的标题栏，将它拖到广告内容区后松手。
4. 嵌入成功后，可在广告弹窗内正常操作该软件。
5. 点击“还原窗口”可恢复成独立窗口；关闭模拟器也会自动还原。

如果目标软件以管理员身份运行，请右键模拟器并选择“以管理员身份运行”，让两个程序处于相同权限级别。

## 兼容性说明

程序使用 Windows 原生跨进程窗口父子关系实现嵌入，适用于常见 Win32、Electron 和多数桌面播放器窗口。以下类型可能受 Windows 或目标软件自身限制：

- DRM 保护的视频画面
- 独占全屏窗口
- 使用特殊硬件覆盖层的播放器
- 主动禁止跨进程嵌入的安全软件或沙盒程序
- 与模拟器权限级别不同的窗口

如果系统拒绝嵌入，模拟器会显示提示并恢复原窗口，不会故意保留空白占位。

## 从源码构建

需要 Windows 10/11、CMake 3.20+，以及 Visual Studio 2022 C++ 工具链或 MinGW-w64。

### Visual Studio 2022

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

生成文件位于 `build/Release/摸鱼模拟器最终版.exe`。

### MinGW-w64

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## 实现原理

程序通过 `SetWinEventHook` 监听系统窗口移动结束事件。当目标窗口在内容区松手后，使用 `SetParent` 建立原生子窗口关系，并更新窗口样式和尺寸。嵌入完成后还会再次检查父窗口与可见状态；还原或退出时恢复原始父窗口、样式及位置。

项目不使用屏幕录制、键盘记录、网络服务或 DLL 注入。

## 项目结构

```text
.
├── CMakeLists.txt
├── README.md
└── src
    ├── app.manifest
    ├── main.cpp
    └── resources.rc
```

## 免责声明

请在遵守所在单位制度和软件使用条款的前提下合理使用。本项目按现状提供，不保证所有采用自定义渲染技术的第三方窗口均可嵌入。

## 作者

**Yaosong808**

Copyright © 2026 Yaosong808. All rights reserved.
