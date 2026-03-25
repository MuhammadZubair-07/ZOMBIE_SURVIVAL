// ============================================================
// openmp/OMP_Update.cpp — OpenMP-parallelised zombie updates
// WEEK 3 IMPLEMENTATION
// ============================================================

#ifdef USE_OPENMP

#include "Processors.h"
#include <omp.h>
#include <cmath>

// ===================== Zombie Update (OpenMP) =====================
// Each zombie is independent → perfect for #pragma omp parallel for.

void updateZombiesOpenMP(ZombieSwarm& z, const Player& p, float dt)
{
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < z.count; i++) {
        if (!z.isAlive[i]) continue;

        float dx   = p.x - z.x[i];
        float dy   = p.y - z.y[i];
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist > 1.0f) {
            z.x[i] += (dx / dist) * z.speed[i] * dt;
            z.y[i] += (dy / dist) * z.speed[i] * dt;
        }
    }
}

// ===================== Collision Detection (OpenMP) =====================
// Parallelise the outer bullet loop.  Each bullet checks all zombies.
// NOTE: writes to z.isAlive / z.health need care — a zombie could be
//       hit by two bullets in the same frame.  Using `critical` for the
//       zombie-side mutation keeps it correct without complex atomics.

void checkCollisionsOpenMP(BulletPool& b, ZombieSwarm& z, Player& p)
{
    // ---- Bullet vs Zombie ----
    #pragma omp parallel for schedule(dynamic, 16)
    for (int bi = 0; bi < b.count; bi++) {
        if (!b.isAlive[bi]) continue;

        for (int zi = 0; zi < z.count; zi++) {
            if (!z.isAlive[zi]) continue;

            float dx   = b.x[bi] - z.x[zi];
            float dy   = b.y[bi] - z.y[zi];
            float dist = sqrtf(dx * dx + dy * dy);

            if (dist < ZOMBIE_RADIUS + BULLET_RADIUS) {
                b.isAlive[bi] = false;
                #pragma omp critical
                {
                    z.health[zi]--;
                    if (z.health[zi] <= 0)
                        z.isAlive[zi] = false;
                }
                break;
            }
        }
    }

    // ---- Zombie vs Player (lightweight — keep sequential) ----
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

#endif // USE_OPENMP
