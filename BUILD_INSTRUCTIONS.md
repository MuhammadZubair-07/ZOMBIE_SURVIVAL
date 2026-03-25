# Build & Run Instructions

## Prerequisites

| Tool          | Version         | Notes |
|---------------|-----------------|-------|
| Visual Studio | 2019 or 2022    | With "Desktop development with C++" workload |
| CMake         | 3.20+           | Bundled with VS or install from cmake.org |
| vcpkg         | latest          | Easiest way to get SDL2 |
| MS-MPI        | latest (Week 3) | https://learn.microsoft.com/en-us/message-passing-interface/microsoft-mpi |
| CUDA Toolkit  | 12.x (Week 4)   | https://developer.nvidia.com/cuda-downloads |

---

## Step 1 — Install vcpkg & SDL2

```powershell
# Clone vcpkg (one-time)
cd C:\
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install SDL2 for 64-bit
.\vcpkg install sdl2:x64-windows

# (Optional) integrate with Visual Studio
.\vcpkg integrate install
```

---

## Step 2 — Build the Project

```powershell
cd "C:\Users\ZUBAIR\Desktop\PDC ZOMBIE GAME"
mkdir build
cd build

# Configure (point CMake at vcpkg toolchain)
cmake .. -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake"

# Build (Release for best performance)
cmake --build . --config Release
```

The executable will be placed in `bin/Release/ZombieSurvivalGame.exe`.

---

## Step 3 — Run

```powershell
cd "C:\Users\ZUBAIR\Desktop\PDC ZOMBIE GAME\bin\Release"
.\ZombieSurvivalGame.exe
```

### In-Game Controls

| Key / Input       | Action                                 |
|-------------------|----------------------------------------|
| **W A S D**       | Move player                            |
| **Mouse**         | Aim direction                          |
| **Left Click**    | Shoot                                  |
| **H**             | Spawn 10,000 more zombies (Horde Mode) |
| **R**             | Restart game                           |
| **1**             | Switch to Sequential mode              |
| **2**             | Switch to OpenMP mode                  |
| **3**             | Switch to MPI mode                     |
| **4**             | Switch to GPU mode                     |
| **ESC**           | Quit                                   |

---

## Building with OpenMP (Week 3)

OpenMP is enabled by default in CMakeLists.txt. MSVC supports it natively — no extra install needed.

```powershell
cmake .. -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" -DENABLE_OPENMP=ON
cmake --build . --config Release
```

---

## Building with MPI (Week 3)

1. Install **MS-MPI** (runtime + SDK).
2. Rebuild with MPI flag:

```powershell
cmake .. -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" -DENABLE_MPI=ON
cmake --build . --config Release
```

3. Run with multiple processes:

```powershell
mpiexec -n 4 .\bin\Release\ZombieSurvivalGame.exe
```

---

## Building with CUDA (Week 4)

1. Install **CUDA Toolkit 12.x**.
2. Rebuild with CUDA flag:

```powershell
cmake .. -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" -DENABLE_CUDA=ON
cmake --build . --config Release
```

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| `SDL.h not found` | Make sure vcpkg's toolchain file path is correct in the `cmake` command |
| `SDL2.dll not found` at runtime | Copy `SDL2.dll` from `vcpkg/installed/x64-windows/bin/` next to the `.exe` |
| OpenMP has no effect | Ensure you're building with MSVC (not MinGW/Clang) |
| MPI won't link | Verify MS-MPI SDK is installed and `mpiexec` is in PATH |
