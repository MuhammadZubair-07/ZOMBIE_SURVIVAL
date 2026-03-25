// ============================================================
// mpi/MPI_Update.cpp — MPI-distributed zombie processing
// WEEK 3 IMPLEMENTATION (stub — fill in during Week 3)
// ============================================================

#ifdef USE_MPI

#include "Processors.h"
#include <mpi.h>
#include <cmath>

// ===================== Zombie Update (MPI) =====================
//
// Strategy:
//   P0 (master)  — owns the Player, renders everything.
//   P1..Pn       — each updates a slice of the zombie array.
//
// Every frame:
//   1. P0 broadcasts player position to all workers.
//   2. Each process updates its assigned zombies.
//   3. MPI_Gatherv brings updated x/y arrays back to P0.
//
// TODO (Week 3): Implement the MPI logic below.

void updateZombiesMPI(ZombieSwarm& z, const Player& p, float dt)
{
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Broadcast player position
    float playerPos[2] = { p.x, p.y };
    MPI_Bcast(playerPos, 2, MPI_FLOAT, 0, MPI_COMM_WORLD);

    // Determine this process's slice
    int perProc = z.count / size;
    int start   = rank * perProc;
    int end     = (rank == size - 1) ? z.count : start + perProc;

    // Update assigned zombies
    for (int i = start; i < end; i++) {
        if (!z.isAlive[i]) continue;

        float dx   = playerPos[0] - z.x[i];
        float dy   = playerPos[1] - z.y[i];
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist > 1.0f) {
            z.x[i] += (dx / dist) * z.speed[i] * dt;
            z.y[i] += (dy / dist) * z.speed[i] * dt;
        }
    }

    // Gather X positions back to P0
    MPI_Allgather(
        MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
        z.x, perProc, MPI_FLOAT,
        MPI_COMM_WORLD);

    // Gather Y positions back to P0
    MPI_Allgather(
        MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
        z.y, perProc, MPI_FLOAT,
        MPI_COMM_WORLD);
}

void checkCollisionsMPI(BulletPool& b, ZombieSwarm& z, Player& p)
{
    // For now, only rank 0 does collision detection after gathering.
    // In a full implementation, distribute bullet checks across ranks too.
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        // Reuse sequential collision for correctness
        for (int bi = 0; bi < b.count; bi++) {
            if (!b.isAlive[bi]) continue;
            for (int zi = 0; zi < z.count; zi++) {
                if (!z.isAlive[zi]) continue;
                float dx   = b.x[bi] - z.x[zi];
                float dy   = b.y[bi] - z.y[zi];
                float dist = sqrtf(dx * dx + dy * dy);
                if (dist < ZOMBIE_RADIUS + BULLET_RADIUS) {
                    b.isAlive[bi] = false;
                    z.health[zi]--;
                    if (z.health[zi] <= 0) z.isAlive[zi] = false;
                    break;
                }
            }
        }

        for (int i = 0; i < z.count; i++) {
            if (!z.isAlive[i]) continue;
            float dx   = p.x - z.x[i];
            float dy   = p.y - z.y[i];
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < PLAYER_RADIUS + ZOMBIE_RADIUS) {
                if (p.invincibleTimer <= 0.0f) {
                    p.health -= 10;
                    p.invincibleTimer = 0.5f;
                }
            }
        }
    }

    // Broadcast updated alive status to all ranks
    MPI_Bcast(z.isAlive, z.count, MPI_C_BOOL, 0, MPI_COMM_WORLD);
    MPI_Bcast(z.health,  z.count, MPI_INT,    0, MPI_COMM_WORLD);
}

#endif // USE_MPI
