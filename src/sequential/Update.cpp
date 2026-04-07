// ============================================================
// sequential/Update.cpp — Sequential zombie AI & collision
// This is the BASELINE that OpenMP / MPI / GPU will beat.
// ============================================================

#include "Processors.h"
#include <cmath>

// ===================== Zombie Update =====================
void updateZombiesSequential(ZombieSwarm& z, const Player& p, float dt,
                              const std::vector<Obstacle>& obstacles)
{
    for (int i = 0; i < z.count; i++) {
        if (!z.isAlive[i]) continue;

        float dx   = p.x - z.x[i];
        float dy   = p.y - z.y[i];
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist > 1.0f) {
            float invDist = 1.0f / dist;
            float nextX = z.x[i] + dx * invDist * z.speed[i] * dt;
            float nextY = z.y[i] + dy * invDist * z.speed[i] * dt;

            for (const auto& obs : obstacles) {
                if (obs.intersects(nextX, z.y[i], ZOMBIE_RADIUS)) nextX = z.x[i];
                if (obs.intersects(z.x[i], nextY, ZOMBIE_RADIUS)) nextY = z.y[i];
                if (obs.intersects(nextX, nextY, ZOMBIE_RADIUS))  { nextX = z.x[i]; nextY = z.y[i]; }
            }

            z.x[i] = nextX;
            z.y[i] = nextY;
        }
    }
}

// ===================== Collision Detection =====================
void checkCollisionsSequential(BulletPool& b, ZombieSwarm& z, Player& p,
                                const std::vector<Obstacle>& obstacles)
{
    // ---- Bullet vs Obstacles & Zombies ----
    for (int bi = 0; bi < b.count; bi++) {
        if (!b.isAlive[bi]) continue;

        // Obstacle check
        bool hitObstacle = false;
        for (const auto& obs : obstacles) {
            if (obs.intersects(b.x[bi], b.y[bi], BULLET_RADIUS)) {
                b.isAlive[bi] = false;
                hitObstacle = true;
                break;
            }
        }
        if (hitObstacle) continue;

        // Zombie check
        for (int zi = 0; zi < z.count; zi++) {
            if (!z.isAlive[zi]) continue;
            float dx   = b.x[bi] - z.x[zi];
            float dy   = b.y[bi] - z.y[zi];
            if (dx * dx + dy * dy < (ZOMBIE_RADIUS + BULLET_RADIUS) * (ZOMBIE_RADIUS + BULLET_RADIUS)) {
                b.isAlive[bi] = false;
                z.health[zi]--;
                if (z.health[zi] <= 0) z.isAlive[zi] = false;
                break;
            }
        }
    }

    // ---- Zombie vs Player ----
    const float touchDist2 = (PLAYER_RADIUS + ZOMBIE_RADIUS) * (PLAYER_RADIUS + ZOMBIE_RADIUS);
    for (int i = 0; i < z.count; i++) {
        if (!z.isAlive[i]) continue;
        float dx = p.x - z.x[i];
        float dy = p.y - z.y[i];
        if (dx * dx + dy * dy < touchDist2) {
            if (p.invincibleTimer <= 0.0f) {
                p.health -= 10;
                p.invincibleTimer = 0.5f;
            }
        }
    }
}
