// ============================================================
// sequential/Update.cpp — Sequential zombie AI & collision
// This is the BASELINE that OpenMP / MPI / GPU will beat.
// ============================================================

#include "Processors.h"
#include <cmath>

// ===================== Zombie Update =====================
// Each zombie moves directly toward the player.
// O(N) per frame — straightforward sequential loop.

void updateZombiesSequential(ZombieSwarm& z, const Player& p, float dt)
{
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

// ===================== Collision Detection =====================
// Bullet–Zombie:  O(B * Z)   — the hot path that GPU will accelerate
// Zombie–Player:  O(Z)

void checkCollisionsSequential(BulletPool& b, ZombieSwarm& z, Player& p)
{
    // ---- Bullet vs Zombie ----
    for (int bi = 0; bi < b.count; bi++) {
        if (!b.isAlive[bi]) continue;

        for (int zi = 0; zi < z.count; zi++) {
            if (!z.isAlive[zi]) continue;

            float dx   = b.x[bi] - z.x[zi];
            float dy   = b.y[bi] - z.y[zi];
            float dist = sqrtf(dx * dx + dy * dy);

            if (dist < ZOMBIE_RADIUS + BULLET_RADIUS) {
                b.isAlive[bi] = false;         // bullet consumed
                z.health[zi]--;
                if (z.health[zi] <= 0)
                    z.isAlive[zi] = false;     // zombie killed
                break;                         // bullet can only hit one zombie
            }
        }
    }

    // ---- Zombie vs Player ----
    for (int i = 0; i < z.count; i++) {
        if (!z.isAlive[i]) continue;

        float dx   = p.x - z.x[i];
        float dy   = p.y - z.y[i];
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist < PLAYER_RADIUS + ZOMBIE_RADIUS) {
            if (p.invincibleTimer <= 0.0f) {
                p.health -= 10;
                p.invincibleTimer = 0.5f;      // 0.5 s invincibility window
            }
        }
    }
}
