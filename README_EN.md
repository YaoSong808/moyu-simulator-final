# Moyu Simulator Final Edition

[中文](README.md) | **English**

A lightweight Windows desktop utility that disguises itself as a bottom-right advertisement popup. Drag another desktop application's window into the popup and continue clicking, scrolling, typing, or playing media inside it.

## Features

- Switches from the welcome screen to an advertisement-style popup with one click
- Embeds another application when you drag its title bar into the content area and release
- Keeps the embedded application interactive without screen capture or video streaming
- Verifies that the target window was actually embedded and restores it on failure instead of leaving a blank area
- Provides a Restore Window button and automatically restores the window when the simulator exits
- Native, single-file Win32 application with no network access, content reading, or telemetry

## Download

Go to the [latest release](https://github.com/YaoSong808/moyu-simulator-final/releases/latest) and download `MoyuSimulatorFinal-v1.0.0-win-x64.exe`.

System requirements: 64-bit Windows 10 or Windows 11. No installation is required.

> This application is not signed with a commercial code-signing certificate. On first launch, Windows SmartScreen may display a warning. Verify the download source, select **More info**, and then choose **Run anyway** if you trust it.

## Usage

1. Run `MoyuSimulatorFinal-v1.0.0-win-x64.exe`.
2. Click **开始摸鱼** to turn the application into a bottom-right advertisement popup.
3. Grab the title bar of Douyin, a media player, or another desktop application and drag it into the popup's content area.
4. Release the mouse button. Once embedded, the application remains fully interactive inside the popup.
5. Click **还原窗口** to restore it as an independent window. Closing the simulator also restores it automatically.

If the target application runs as administrator, right-click the simulator and select **Run as administrator** so both applications use the same privilege level.

## Compatibility

The simulator uses native Windows cross-process parent-child window relationships. It works with common Win32 applications, Electron applications, and most desktop media players. Windows or the target application may prevent embedding for:

- DRM-protected video surfaces
- Exclusive full-screen windows
- Players that use special hardware overlay surfaces
- Security or sandboxed applications that explicitly block cross-process embedding
- Windows running at a different privilege level from the simulator

When Windows rejects an embedding attempt, the simulator displays a message and restores the original window instead of intentionally leaving an empty placeholder.

## Building from Source

You need 64-bit Windows 10 or 11, CMake 3.20 or newer, and either the Visual Studio 2022 C++ toolchain or MinGW-w64.

### Visual Studio 2022

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The executable will be generated at `build/Release/摸鱼模拟器最终版.exe`.

### MinGW-w64

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## How It Works

The simulator uses `SetWinEventHook` to listen for the end of system window-move operations. When a target window is released over the content area, it uses `SetParent` to establish a native child-window relationship and updates the target's style and dimensions. It then verifies the parent and visibility state. Restoring or exiting reinstates the original parent, style, and position.

The project does not use screen recording, keylogging, network services, or DLL injection.

## Project Structure

```text
.
├── CMakeLists.txt
├── README.md
├── README_EN.md
└── src
    ├── app.manifest
    ├── main.cpp
    └── resources.rc
```

## Disclaimer

Use this software responsibly and comply with your workplace policies and the terms of third-party applications. The project is provided as-is and cannot guarantee compatibility with every custom-rendered application window.

## Author

**Yaosong808**

Copyright © 2026 Yaosong808. All rights reserved.
