// ============================================================
// openmp/OMP_Update.cpp — OpenMP-parallelised zombie updates
//
// BUG FIXED: Replaced #pragma omp critical with #pragma omp atomic.
// omp critical forces every thread to queue up one-by-one on every
// bullet hit — completely killing parallel performance.
// omp atomic uses a single CPU instruction (LOCK XADD) with no queue.
//
// Also added OPENMP_MIN_ZOMBIES threshold: for very small counts the
// thread overhead is larger than the computation, so we fall back to
// the sequential inner-loop but still dispatch the parallel region.
// ============================================================

#ifdef USE_OPENMP

#include "Processors.h"
#include <omp.h>
#include <cmath>

// Only use full parallelism above this threshold
static const int OPENMP_MIN_ZOMBIES = 50;

void updateZombiesOpenMP(ZombieSwarm& z, const Player& p, float dt,
                          const std::vector<Obstacle>& obstacles)
{
    const float px = p.x, py = p.y;

    // For small zombie counts — thread overhead > computation benefit.
    // OpenMP threads are persistent (not re-created each frame), but
    // the barrier+fork synchronization still costs ~0.1-0.5 ms.
    if (z.count < OPENMP_MIN_ZOMBIES) {
        // Sequential fallback for small counts
        for (int i = 0; i < z.count; i++) {
            if (!z.isAlive[i]) continue;
            float dx   = px - z.x[i];
            float dy   = py - z.y[i];
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

    // Parallel path: each thread handles a contiguous chunk (schedule=static)
    // Each zombie reads its OWN array indices — zero write conflicts here.
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < z.count; i++) {
        if (!z.isAlive[i]) continue;

        float dx   = px - z.x[i];
        float dy   = py - z.y[i];
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist > 1.0f) {
            float invDist = 1.0f / dist;
            float nextX = z.x[i] + dx * invDist * z.speed[i] * dt;
            float nextY = z.y[i] + dy * invDist * z.speed[i] * dt;

            // Obstacles: read-only, safe to access from all threads simultaneously
            for (const auto& obs : obstacles) {
                if (obs.intersects(nextX, z.y[i], ZOMBIE_RADIUS)) nextX = z.x[i];
                if (obs.intersects(z.x[i], nextY, ZOMBIE_RADIUS)) nextY = z.y[i];
                if (obs.intersects(nextX, nextY,  ZOMBIE_RADIUS)) { nextX = z.x[i]; nextY = z.y[i]; }
            }

            z.x[i] = nextX;  // each thread writes to a different index — no conflict
            z.y[i] = nextY;
        }
    }
}

void checkCollisionsOpenMP(BulletPool& b, ZombieSwarm& z, Player& p,
                            const std::vector<Obstacle>& obstacles)
{
    const float touch2 = (ZOMBIE_RADIUS + BULLET_RADIUS) * (ZOMBIE_RADIUS + BULLET_RADIUS);

    // schedule(dynamic,8): bullets that miss skip fast; bullets that scan many
    // zombies take longer. Dynamic balances this uneven work across threads.
    #pragma omp parallel for schedule(dynamic, 8)
    for (int bi = 0; bi < b.count; bi++) {
        if (!b.isAlive[bi]) continue;

        // Obstacle check — read-only, thread-safe
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

                // -------------------------------------------------------
                // FIX: omp atomic instead of omp critical
                //
                // omp critical   = mutex lock, all threads queue up here
                //                  (one thread at a time — kills parallelism)
                //
                // omp atomic     = single CPU LOCK instruction (LOCK XADD)
                //                  no queue, ~5ns, threads don't wait
                // -------------------------------------------------------
                int newHealth;
                #pragma omp atomic capture
                { newHealth = z.health[zi]; z.health[zi]--; }

                // newHealth holds the OLD value before decrement.
                // If it was 1, health is now 0 → kill the zombie.
                // isAlive is only ever written false (never back to true),
                // so a race condition here is harmless (both threads set false).
                if (newHealth <= 1)
                    z.isAlive[zi] = false;

                break;
            }
        }
    }

    // Zombie vs Player: sequential — single shared player state
    const float touchPlayer2 = (PLAYER_RADIUS + ZOMBIE_RADIUS) * (PLAYER_RADIUS + ZOMBIE_RADIUS);
    for (int i = 0; i < z.count; i++) {
        if (!z.isAlive[i]) continue;
        float dx = p.x - z.x[i];
        float dy = p.y - z.y[i];
        if (dx * dx + dy * dy < touchPlayer2) {
            if (p.invincibleTimer <= 0.0f) {
                p.health -= 10;
                p.invincibleTimer = 0.5f;
            }
        }
    }
}

#endif // USE_OPENMP
