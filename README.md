# Smart Traffic Management

## Project Overview

A C++ desktop simulation of traffic on a nine-intersection city grid. It includes traffic signals, vehicle routing, and ambulance priority.

## Technologies Used

- C++17
- Raylib 6.2 for graphics
- CMake for project configuration and builds
- PowerShell and VBScript for Windows launch setup

## Requirements

- 64-bit Windows
- 64-bit MinGW-w64 compiler from WinLibs
- Raylib files included in `raylib\`
- CMake 3.16 or newer and Ninja to rebuild the application

## Setup & Configuration

Open PowerShell in the project folder. The Raylib headers, library, and DLL are in `raylib\include` and `raylib\lib`. The launch command below adds the MinGW and Raylib folders to `PATH` for that PowerShell session.

## How to Run

If you have not built the application yet, run these commands from the project folder:

```powershell
$compiler = "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\g++.exe"
cmake -S . -B build-current-x64 -G Ninja "-DCMAKE_CXX_COMPILER=$compiler"
cmake --build build-current-x64 --config Release
```

Then launch it from the project folder:

```powershell
$env:Path = "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin;$PWD\raylib\lib;$env:Path"; .\build-current-x64\smart_traffic.exe
```

## Run Using Launcher

Double-click **Smart Traffic Management** on the Desktop to launch without a terminal window. The shortcut runs `launch-smart-traffic.vbs`, which sets the required `PATH` for the application. The executable must be present at `build-current-x64\smart_traffic.exe`.
