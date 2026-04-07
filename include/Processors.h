// ============================================================
// Processors.h — Function declarations for each processing mode
// Sequential (baseline), OpenMP, MPI, GPU/CUDA
// ============================================================
#pragma once

#include "Entities.h"
#include <vector>

// --------------- Sequential (always available) ---------------
void updateZombiesSequential  (ZombieSwarm& z, const Player& p, float dt,
                                const std::vector<Obstacle>& obstacles);
void checkCollisionsSequential(BulletPool& b, ZombieSwarm& z, Player& p,
                                const std::vector<Obstacle>& obstacles);

// --------------- OpenMP (Week 3) ---------------
#ifdef USE_OPENMP
void updateZombiesOpenMP  (ZombieSwarm& z, const Player& p, float dt,
                            const std::vector<Obstacle>& obstacles);
void checkCollisionsOpenMP(BulletPool& b, ZombieSwarm& z, Player& p,
                            const std::vector<Obstacle>& obstacles);
#endif

// --------------- MPI (Week 3) ---------------
#ifdef USE_MPI
void updateZombiesMPI  (ZombieSwarm& z, const Player& p, float dt,
                         const std::vector<Obstacle>& obstacles);
void checkCollisionsMPI(BulletPool& b, ZombieSwarm& z, Player& p,
                         const std::vector<Obstacle>& obstacles);
#endif

// --------------- GPU / CUDA (Week 4) ---------------
#ifdef USE_CUDA
void checkCollisionsGPU(BulletPool& b, ZombieSwarm& z, Player& p,
                         const std::vector<Obstacle>& obstacles);
#endif
