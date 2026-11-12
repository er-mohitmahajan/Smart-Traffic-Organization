# Smart Traffic Management System

A C++17 and raylib simulation for a college Data Structures project. Nine road
intersections form a two-way city grid. Vehicles use congestion-aware routes,
wait in per-lane queues, and are served by either density-based or fixed-time
signals. An ambulance can request a destination and receive an automatically
overridden, Dijkstra-routed path.

## Features

- Nine clickable intersections connected by two-way roads.
- A city-style view with broad asphalt roads, dashed markings, colored cars,
  approach traffic lights, live junction labels, and a route glow for ambulances.
- Vehicles travel in a consistent road lane, brake behind a lead car, and
  accelerate when their signal serves their lane.
- Smart signals rank queued and detected approach traffic plus waiting-time
  aging with the custom max-heap. Opposite directions on the selected axis run
  together, with a yellow clearance phase before a conflicting axis turns green.
- Smart green times are bounded between 5 and 20 seconds; fixed mode uses
  10-second axis phases and ignores live traffic and emergency preemption.
- Congestion-aware road costs are refreshed every 0.5 seconds and used by
  custom-min-heap Dijkstra for routing and ambulance rerouting.
- An ambulance preempts its approach plus upcoming route junctions after a
  yellow clearance. It flashes red/blue, and completed junctions return to
  normal smart control.
- Press `C` to run a 60-second comparison: 30 seconds in smart mode followed
  by 30 seconds in fixed mode. Per-mode cleared-vehicle and wait statistics
  remain visible while toggling modes.
- Dashboard with FPS, active vehicle count, spawn rate, adjustable vehicle
  speed, signal state, queue lengths, heap priorities, emergency route, and
  mode statistics.
- Click the dashboard's `-` and `+` speed buttons to adjust all vehicle speeds,
  including vehicles already on the roads. Speed ranges from `0.5x` to `2.0x`
  in `0.1x` increments and resets to `1.0x`.
- A standalone console test for the custom queue, heaps, graph, and Dijkstra.

## Controls

| Key / action | Effect |
| --- | --- |
| `SPACE` | Pause or resume |
| `T` | Toggle smart-density and fixed-timer modes |
| `E` | Send an ambulance to the selected destination |
| `+` / `-` | Increase or decrease the random spawn rate |
| `R` | Reset the simulation |
| `C` | Run a new 60-second comparison: 30 seconds smart, then 30 seconds fixed |
| Click dashboard `-` / `+` | Decrease or increase vehicle speed by `0.1x` |
| Click a junction | Inspect its queues and heap; set the ambulance destination |

Vehicle speed can be adjusted between `0.5x` and `2.0x`; the dashboard shows
the current multiplier. This multiplier applies to active and newly spawned
vehicles. The `+` / `-` keyboard keys continue to adjust the vehicle spawn
rate, not vehicle speed.

The center junction is the initial ambulance destination. If an ambulance is
already active, another one is not spawned until it reaches its destination.

## Data structures and complexity

| Structure / algorithm | Where it is used | Complexity |
| --- | --- | --- |
| Adjacency-list graph (`Graph`) | Stores the 3x3 junction network, road travel times, and live congestion. | Space `O(V + E)`; neighbor traversal `O(degree)` |
| Custom linked queue (`Queue<T>`) | FIFO waiting vehicles for each incoming lane; emergency insertion at the front. | Enqueue, dequeue, and front insertion `O(1)`; traversal `O(n)` |
| Custom binary max-heap (`MaxHeap<T>`) | Ranks incoming signal lanes by `queue + 0.5 * detectedApproach + 0.2 * waitSeconds`. Heap rebuild is `O(n)`; top `O(1)`; push/pop `O(log n)`. |
| Custom binary min-heap (`MinHeap<T>`) | Selects the next closest junction in Dijkstra's algorithm. | Top `O(1)`; push/pop `O(log V)` |
| Dijkstra with path reconstruction | Routes normal traffic and ambulances using base travel time plus congestion. | `O((V + E) log V)` with this adjacency list and min-heap |

The core routing and scheduling do not use `std::priority_queue`, `std::map`,
`std::unordered_map`, or `std::queue`.

## Build and run: Windows with MinGW / w64devkit

Use a **64-bit MinGW-w64 compiler** and a Raylib 5.x or compatible MinGW build.
The MinGW Raylib package should provide `raylib/include/raylib.h` and
`raylib/lib/libraylib.a` (the layout already used by this project).

From PowerShell in the project directory, set the compiler and Raylib paths:

```powershell
$env:Path = "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin;$PWD\raylib\lib;$env:Path"
```

With w64devkit, open its terminal and add the w64devkit `bin` directory to
`PATH`; add the directory containing `raylib.dll` to `PATH` when launching.

Exact build command (run in the project directory):

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic main.cpp Graph.cpp Dijkstra.cpp Vehicle.cpp TrafficSignal.cpp Simulation.cpp -Iraylib/include -Lraylib/lib -lraylib -lopengl32 -lgdi32 -lwinmm -o smart_traffic.exe
```

Run the app:

```powershell
.\smart_traffic.exe
```

Run the console tests without opening a graphics window:

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic -I. tests/data_structure_tests.cpp Graph.cpp Dijkstra.cpp -o data_structure_tests.exe
.\data_structure_tests.exe
```

Run the end-to-end simulation and emergency-routing test without a graphics
window:

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic -I. tests/simulation_tests.cpp Graph.cpp Dijkstra.cpp Vehicle.cpp TrafficSignal.cpp Simulation.cpp -o simulation_tests.exe
.\simulation_tests.exe
```

The GUI is linked against the static Raylib archive. Keep the 64-bit MinGW
runtime available on `PATH` when launching a build made by MinGW.

## Build with Visual Studio

Install Visual Studio 2022 with the **Desktop development with C++** workload,
then install the x64 Raylib package with vcpkg:

```powershell
vcpkg install raylib:x64-windows
```

Configure and build from a Visual Studio Developer PowerShell (replace the
toolchain path with your vcpkg installation):

```powershell
cmake -S . -B build -A x64 -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Launch `build\Release\smart_traffic.exe`. If the raylib DLL is not on `PATH`,
copy the matching x64 `raylib.dll` beside the executable.

## CMake

`CMakeLists.txt` builds the GUI executable and a separate console test target.
It uses an installed Raylib CMake package (for example vcpkg); on Windows with
MinGW it can also use the project-local `raylib/` package.

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Source layout

- `main.cpp` - Raylib window, city rendering, dashboard, and input.
- `Graph.h/.cpp` - Dynamic-array adjacency list and congestion-aware road edges.
- `MaxHeap.h` / `MinHeap.h` - Custom binary heaps.
- `Queue.h` - Owning linked queue for each incoming lane.
- `Vehicle.h/.cpp` - Vehicle state and mode-comparison statistics.
- `TrafficSignal.h/.cpp` - Per-junction lane queues, signal timing, and priority heap.
- `TrafficConfig.h` - Road, vehicle, detector, signal, spawn-rate, and speed tuning constants.
- `Dijkstra.h/.cpp` - Congestion-aware shortest paths and reconstruction.
- `Simulation.h/.cpp` - Network, vehicles, spawning, routing, and emergency control.
- `tests/data_structure_tests.cpp` - Queue, heap, graph, and Dijkstra console tests.
- `tests/simulation_tests.cpp` - Network, mode, and emergency simulation test.

## Possible extensions

- Pedestrian crossings with walk phases and pedestrian request queues.
- Accident events that temporarily close roads and trigger rerouting.
- Rain, rush-hour, or configurable peak-traffic modes.
- Turning lanes, vehicle classes, and adaptive emergency dispatch.
- Saveable simulation runs and charts comparing smart and fixed signals.

# v2

## Project Overview

A C++ desktop application that simulates traffic signals, vehicles, and emergency routing across a nine-intersection city grid.

## Technologies Used

- C++17
- Raylib 6.2 for graphics
- CMake for project configuration and builds
- PowerShell and VBScript for Windows setup and launching

## Requirements

- 64-bit Windows
- 64-bit MinGW-w64 compiler from WinLibs
- CMake 3.16 or newer and Ninja to build
- Raylib files provided in `raylib\`

## Setup & Configuration

Open PowerShell in the project folder. The Raylib headers, library, and DLL are in `raylib\include` and `raylib\lib`. Before building, configure CMake with the MinGW compiler:

```powershell
$compiler = "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\g++.exe"
cmake -S . -B build-current-x64 -G Ninja "-DCMAKE_CXX_COMPILER=$compiler"
cmake --build build-current-x64 --config Release
```

## How to Run

Run this command from the project folder:

```powershell
$env:Path = "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin;$PWD\raylib\lib;$env:Path"; .\build-current-x64\smart_traffic.exe
```

## Run Using Launcher

Double-click the **Smart Traffic Management** Desktop shortcut to launch without a terminal window. The shortcut runs `launch-smart-traffic.vbs`, which configures the required `PATH`. The executable must exist at `build-current-x64\smart_traffic.exe`.
