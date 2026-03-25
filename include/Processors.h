// ============================================================
// Processors.h — Function declarations for each processing mode
// Sequential (Week 1-2), OpenMP (Week 3), MPI (Week 3), GPU (Week 4)
// ============================================================
#pragma once

#include "Entities.h"

// --------------- Sequential (always available) ---------------
void updateZombiesSequential  (ZombieSwarm& z, const Player& p, float dt);
void checkCollisionsSequential(BulletPool& b, ZombieSwarm& z, Player& p);

// --------------- OpenMP (Week 3) ---------------
#ifdef USE_OPENMP
void updateZombiesOpenMP  (ZombieSwarm& z, const Player& p, float dt);
void checkCollisionsOpenMP(BulletPool& b, ZombieSwarm& z, Player& p);
#endif

// --------------- MPI (Week 3) ---------------
#ifdef USE_MPI
void updateZombiesMPI  (ZombieSwarm& z, const Player& p, float dt);
void checkCollisionsMPI(BulletPool& b, ZombieSwarm& z, Player& p);
#endif

// --------------- GPU / CUDA (Week 4) ---------------
#ifdef USE_CUDA
void checkCollisionsGPU(BulletPool& b, ZombieSwarm& z, Player& p);
#endif
