# 🧟 Parallel Zombie Survival Game — Complete Roadmap

## 1️⃣ Project Overview

**Concept:**
A 2D top-down game where a player moves in a map and shoots zombies. Zombies relentlessly chase the player. The game ends when the player kills all zombies (Win) or the zombies reach the player (Loss).

**Scale & Performance (Horde Mode):**
To properly demonstrate parallel computing speedup, the game will feature a "Horde Mode" stress test rendering **10,000 to 50,000+ zombies**. At this scale, the sequential CPU implementation will bottleneck, allowing OpenMP, MPI, and GPU implementations to showcase massive performance gains.

**Parallel Technologies:**
*   **OpenMP (Shared Memory):** Parallelizes zombie movement and AI updates across local CPU cores.
*   **MPI (Distributed Memory):** Divides subsets of the zombie horde across multiple processes, which then send updated positions back to a master rendering process.
*   **GPU (CUDA/OpenCL):** Accelerates heavy mathematical operations like massive distance/collision checks between thousands of bullets and tens of thousands of zombies.

---

## 2️⃣ Technology Stack

| Component | Tool / Technology |
| :--- | :--- |
| **Language** | C++ |
| **Build System** | CMake (To handle linking C++, SDL2, MPI, and CUDA cleanly) |
| **IDE** | Visual Studio / VS Code |
| **Graphics** | SDL2 |
| **Parallel CPU** | OpenMP |
| **Distributed** | MPI (MS-MPI) |
| **GPU** | CUDA (or OpenCL) |
| **OS** | Windows |

---

## 3️⃣ Game Design

**Map:**
Small to medium 2D grid map (e.g., 800 x 600 or 1920 x 1080 window).
*   Walls and Obstacles
*   Spawn Zones
*   Unbounded rendering for large hordes

---

## 4️⃣ Game Entities & Data Layout

**Critical Optimization: Structure of Arrays (SoA)**
To maximize caching and parallelization efficiency for OpenMP and GPU execution, entity data will be stored as a Structure of Arrays (SoA) rather than an Array of Structures (AoS).

**Player:**
*   `x, y`, `speed`, `health`, `direction`, `weapon`
*   Controls: W A S D (Movement), Mouse (Aim), Left Click (Shoot)

**Zombies (SoA Format):**
```cpp
struct ZombieSwarm {
    float* x;
    float* y;
    float* speed;
    bool* isAlive;
    // ...
};
```
*   Behavior: Move toward the player, avoid walls, die upon bullet collision.

**Bullets (SoA Format):**
*   `x, y`, `direction`, `speed`, `isAlive`
*   Logic: Spawn on click, move forward, destroy zombie and self on collision.

---

## 5️⃣ Core Game Loop

To ensure physics and movement remain consistent regardless of how fast the parallel versions run compared to the sequential version, the loop uses **Delta Time**.

```cpp
while(gameRunning)
{
    float deltaTime = getDeltaTime();
    
    handleInput();
    updatePlayer(deltaTime);
    
    // Handled via Sequential, OpenMP, or MPI
    updateZombies(deltaTime); 
    
    updateBullets(deltaTime);
    
    // Handled via GPU / CUDA
    checkCollisions(); 
    
    renderGame();
}
```

---

## 6️⃣ Parallel Implementation Details

### 🔵 OpenMP (Zombie Movement)
Zombies move independently. Using `#pragma omp parallel for`, threads will divide the massive loop of updating `10,000+` zombies. Memory access will be fast due to the SoA layout.

### 🔴 MPI (Distributed Zombie Processing)
Using `mpiexec -n 4 game.exe`.
*   **P0 (Master):** Handles Player logic, rendering, and gathers data.
*   **P1 - P3 (Workers):** Each updates a subset of the zombies (e.g., P1 does 1-15,000, P2 does 15,001-30,000, etc.).
*   **Synchronization:** Uses `MPI_Gather` to bring all X/Y rendering coordinates back to P0 every frame for drawing.

### 🟢 GPU (CUDA Collision Detection)
Collision checks are an $O(B \times Z)$ problem. 100 bullets $\times$ 50,000 zombies = 5,000,000 checks per frame.
*   A CUDA kernel will map one thread per bullet-zombie pair (or one thread per zombie checking all active bullets).
*   Calculates Euclidean distances in parallel. If `distance < threshold`, mark zombie and bullet as dead.

---

## 7️⃣ Folder & Build Structure

```text
ZombieSurvivalGame/
│
├── CMakeLists.txt        <-- Build configuration for C++, SDL2, MPI, CUDA
├── src/
│   ├── main.cpp
│   ├── Game.cpp
│   ├── Entities.cpp      <-- Player, Zombies, Bullets
│   ├── sequential/
│   │   └── Update.cpp
│   ├── openmp/
│   │   └── OMP_Update.cpp
│   ├── mpi/
│   │   └── MPI_Update.cpp
│   └── gpu/
│       └── CUDA_Collisions.cu
├── include/
│   ├── Game.h
│   ├── Entities.h
│   ├── Processors.h
│   └── Utils.h
└── assets/
    ├── sprites/
    └── map/
```

---

## 8️⃣ Performance Comparison & Reporting

The final project report will feature a heavy emphasis on benchmarking. Metrics will be gathered by testing the "Horde Mode" with 10k, 25k, and 50k zombies.

**Expected Graph Comparisons:**
*   **Sequential vs. OpenMP (2/4/8 threads):** Demonstrates multi-core CPU scaling.
*   **Sequential vs. MPI (2/4 processes):** Demonstrates distributed processing overhead vs computation speed.
*   **CPU Collisions vs. GPU Collisions:** Demonstrates massive parallel throughput of CUDA.

Teachers love precise `ms` rendering graphs.

---

## 9️⃣ Development Milestones (1 Month Plan)

### 🗓️ Week 1 — Core Environment & Setup
*   **Tasks:**
    *   Set up CMakeLists.txt to handle MSVC, SDL2, OpenMP, MS-MPI, and CUDA.
    *   Initialize SDL2 Window and implement basic game loop structure.
    *   Implement Player movement (WASD + Mouse aim).
    *   Implement basic SoA structure for Entities.
*   ✅ **Milestone:** Basic game window + player moves + build system flawlessly compiling everything.

### 🗓️ Week 2 — Sequential Game Mechanics
*   **Tasks:**
    *   Implement Zombie spawner (Horde mode capabilities).
    *   Implement Sequential Zombie AI (chasing player, simple wall avoidance).
    *   Implement Bullet shooting and basic sequential collision detection.
    *   Implement entity death / despawning.
*   ✅ **Milestone:** Fully playable sequential game capable of stress-testing the CPU.

### 🗓️ Week 3 — CPU Parallelization (OpenMP & MPI)
*   **Tasks:**
    *   Integrate OpenMP for parallel zombie movement updates.
    *   Refactor main loop to support MPI initialization (`MPI_Init`, branching master vs worker nodes).
    *   Implement `MPI_Gather` to efficiently sync worker zombie positions down to the master renderer.
    *   Test via `mpiexec -n 4`.
*   ✅ **Milestone:** Game runs using OpenMP threads and across multiple MPI processes.

### 🗓️ Week 4 — GPU Acceleration & Optimization
*   **Tasks:**
    *   Write CUDA kernels to evaluate bullet-zombie distance collisions.
    *   Manage memory transfers (cudaMemcpy) between Host and Device efficiently.
    *   Implement a metrics/benchmarking overlay (FPS, update time ms, collision time ms).
    *   Gather data, formulate graphs, and write the final report.
*   ✅ **Milestone:** Complete project demonstrating all parallel paradigms seamlessly.
