// ============================================================
// mpi/MPI_Update.cpp — MPI-distributed zombie processing
//
// BUG FIXED: Added single-process early exit.
// When the game is run normally (not via mpiexec), MPI_Comm_size = 1.
// The old code still called MPI_Bcast + MPI_Allgather even with 1 rank,
// adding hundreds of microseconds of MPI overhead for zero benefit.
//
// FIX: If size==1, just run sequential — no MPI calls needed.
// When run with mpiexec -n 4, each rank genuinely processes 1/4 zombies.
//
// HOW TO RUN WITH TRUE MPI PARALLELISM:
//   mpiexec -n 4 "e:\PDC ZOMBIE GAME\bin\ZombieSurvivalGame.exe"
// ============================================================

#ifdef USE_MPI

#include "Processors.h"
#include <mpi.h>
#include <cmath>

void updateZombiesMPI(ZombieSwarm& z, const Player& p, float dt,
                       const std::vector<Obstacle>& obstacles)
{
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // -------------------------------------------------------
    // FIX: Single-process early exit
    // Without mpiexec, size==1. All MPI calls still execute but
    // do nothing useful — they just add latency. Skip them.
    // -------------------------------------------------------
    if (size == 1) {
        // Identical to sequential — no MPI overhead
        for (int i = 0; i < z.count; i++) {
            if (!z.isAlive[i]) continue;
            float dx   = p.x - z.x[i];
            float dy   = p.y - z.y[i];
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist > 1.0f) {
                float inv = 1.0f / dist;
                float nx  = z.x[i] + dx * inv * z.speed[i] * dt;
                float ny  = z.y[i] + dy * inv * z.speed[i] * dt;
                for (const auto& obs : obstacles) {
                    if (obs.intersects(nx, z.y[i], ZOMBIE_RADIUS)) nx = z.x[i];
                    if (obs.intersects(z.x[i], ny, ZOMBIE_RADIUS)) ny = z.y[i];
                    if (obs.intersects(nx, ny,  ZOMBIE_RADIUS))  { nx = z.x[i]; ny = z.y[i]; }
                }
                z.x[i] = nx;
                z.y[i] = ny;
            }
        }
        return;
    }

    // -------------------------------------------------------
    // True multi-process path (mpiexec -n N):
    //
    // Rank 0 owns and broadcasts player position to all ranks.
    // Each rank computes movement for its slice of the zombie array.
    // MPI_Allgather collects all updated positions back to every rank.
    // Rank 0 (the rendering process) now has all updated positions.
    // -------------------------------------------------------

    // Step 1: Broadcast player position from rank 0 to all workers
    float playerPos[2] = { p.x, p.y };
    MPI_Bcast(playerPos, 2, MPI_FLOAT, 0, MPI_COMM_WORLD);

    // Step 2: Partition zombies — each rank gets its own slice
    int perProc = z.count / size;
    int start   = rank * perProc;
    int end     = (rank == size - 1) ? z.count : start + perProc;

    // Step 3: Each rank independently updates its slice (no communication)
    for (int i = start; i < end; i++) {
        if (!z.isAlive[i]) continue;

        float dx   = playerPos[0] - z.x[i];
        float dy   = playerPos[1] - z.y[i];
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist > 1.0f) {
            float invDist = 1.0f / dist;
            float nextX = z.x[i] + dx * invDist * z.speed[i] * dt;
            float nextY = z.y[i] + dy * invDist * z.speed[i] * dt;

            for (const auto& obs : obstacles) {
                if (obs.intersects(nextX, z.y[i], ZOMBIE_RADIUS)) nextX = z.x[i];
                if (obs.intersects(z.x[i], nextY, ZOMBIE_RADIUS)) nextY = z.y[i];
                if (obs.intersects(nextX, nextY,  ZOMBIE_RADIUS)) { nextX = z.x[i]; nextY = z.y[i]; }
            }

            z.x[i] = nextX;
            z.y[i] = nextY;
        }
    }

    // Step 4: Gather all updated positions back — every rank gets the full array
    MPI_Allgather(
        MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
        z.x, perProc, MPI_FLOAT,
        MPI_COMM_WORLD);

    MPI_Allgather(
        MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
        z.y, perProc, MPI_FLOAT,
        MPI_COMM_WORLD);
}

void checkCollisionsMPI(BulletPool& b, ZombieSwarm& z, Player& p,
                         const std::vector<Obstacle>& obstacles)
{
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Single-process: no MPI overhead, just run sequential
    if (size == 1) {
        const float touch2 = (ZOMBIE_RADIUS + BULLET_RADIUS) * (ZOMBIE_RADIUS + BULLET_RADIUS);
        for (int bi = 0; bi < b.count; bi++) {
            if (!b.isAlive[bi]) continue;
            bool hitObs = false;
            for (const auto& obs : obstacles) {
                if (obs.intersects(b.x[bi], b.y[bi], BULLET_RADIUS)) {
                    b.isAlive[bi] = false; hitObs = true; break;
                }
            }
            if (hitObs) continue;
            for (int zi = 0; zi < z.count; zi++) {
                if (!z.isAlive[zi]) continue;
                float dx = b.x[bi] - z.x[zi];
                float dy = b.y[bi] - z.y[zi];
                if (dx * dx + dy * dy < touch2) {
                    b.isAlive[bi] = false;
                    z.health[zi]--;
                    if (z.health[zi] <= 0) z.isAlive[zi] = false;
                    break;
                }
            }
        }
        const float tp2 = (PLAYER_RADIUS + ZOMBIE_RADIUS) * (PLAYER_RADIUS + ZOMBIE_RADIUS);
        for (int i = 0; i < z.count; i++) {
            if (!z.isAlive[i]) continue;
            float dx = p.x - z.x[i], dy = p.y - z.y[i];
            if (dx*dx + dy*dy < tp2) {
                if (p.invincibleTimer <= 0.0f) { p.health -= 10; p.invincibleTimer = 0.5f; }
            }
        }
        return;
    }

    // Multi-process: only master (rank 0) does collision detection
    // (collision is tricky to distribute — it needs full bullet+zombie data)
    if (rank == 0) {
        const float touch2 = (ZOMBIE_RADIUS + BULLET_RADIUS) * (ZOMBIE_RADIUS + BULLET_RADIUS);

        for (int bi = 0; bi < b.count; bi++) {
            if (!b.isAlive[bi]) continue;

            bool hitObs = false;
            for (const auto& obs : obstacles) {
                if (obs.intersects(b.x[bi], b.y[bi], BULLET_RADIUS)) {
                    b.isAlive[bi] = false;
                    hitObs = true;
                    break;
                }
            }
            if (hitObs) continue;

            for (int zi = 0; zi < z.count; zi++) {
                if (!z.isAlive[zi]) continue;
                float dx = b.x[bi] - z.x[zi];
                float dy = b.y[bi] - z.y[zi];
                if (dx * dx + dy * dy < touch2) {
                    b.isAlive[bi] = false;
                    z.health[zi]--;
                    if (z.health[zi] <= 0) z.isAlive[zi] = false;
                    break;
                }
            }
        }

        const float touchP2 = (PLAYER_RADIUS + ZOMBIE_RADIUS) * (PLAYER_RADIUS + ZOMBIE_RADIUS);
        for (int i = 0; i < z.count; i++) {
            if (!z.isAlive[i]) continue;
            float dx = p.x - z.x[i];
            float dy = p.y - z.y[i];
            if (dx * dx + dy * dy < touchP2) {
                if (p.invincibleTimer <= 0.0f) {
                    p.health -= 10;
                    p.invincibleTimer = 0.5f;
                }
            }
        }
    }

    // Broadcast updated zombie alive/health state from rank 0 to all ranks
    MPI_Bcast(z.isAlive, z.count, MPI_C_BOOL, 0, MPI_COMM_WORLD);
    MPI_Bcast(z.health,  z.count, MPI_INT,    0, MPI_COMM_WORLD);
}

#endif // USE_MPI
