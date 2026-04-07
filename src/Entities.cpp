// ============================================================
// Entities.cpp — Allocation, deallocation, spawn, update
// for Player, ZombieSwarm (SoA), and BulletPool (SoA)
// ============================================================

#include "Entities.h"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>

// ========================= Obstacle =========================

bool Obstacle::intersects(float cx, float cy, float radius) const
{
    // Find the closest point on the rectangle to the circle center
    float closestX = cx;
    float closestY = cy;

    if (closestX < x)         closestX = x;
    else if (closestX > x + width)  closestX = x + width;

    if (closestY < y)         closestY = y;
    else if (closestY > y + height) closestY = y + height;

    float dx = cx - closestX;
    float dy = cy - closestY;
    return (dx * dx + dy * dy) < (radius * radius);
}

// ========================= Player =========================

void Player::init(float startX, float startY)
{
    x = startX;
    y = startY;
    speed           = PLAYER_SPEED;
    health          = 100;
    aimAngle        = 0.0f;
    moveUp = moveDown = moveLeft = moveRight = false;
    shooting        = false;
    shootCooldown   = SHOOT_COOLDOWN;
    shootTimer      = 0.0f;
    invincibleTimer = 0.0f;
}

void Player::update(float dt, int screenW, int screenH,
                    const std::vector<Obstacle>& obstacles)
{
    // Compute movement direction
    float dx = 0.0f, dy = 0.0f;
    if (moveUp)    dy -= 1.0f;
    if (moveDown)  dy += 1.0f;
    if (moveLeft)  dx -= 1.0f;
    if (moveRight) dx += 1.0f;

    // Normalize diagonal movement
    float len = sqrtf(dx * dx + dy * dy);
    if (len > 0.0f) { dx /= len; dy /= len; }

    float nextX = x + dx * speed * dt;
    float nextY = y + dy * speed * dt;

    // Obstacle collision (AABB sliding)
    for (const auto& obs : obstacles) {
        if (obs.intersects(nextX, y, PLAYER_RADIUS))
            nextX = x;
        if (obs.intersects(x, nextY, PLAYER_RADIUS))
            nextY = y;
        if (obs.intersects(nextX, nextY, PLAYER_RADIUS)) {
            nextX = x;
            nextY = y;
        }
    }

    x = nextX;
    y = nextY;

    // Clamp to screen bounds
    if (x < PLAYER_RADIUS)                               x = PLAYER_RADIUS;
    if (y < PLAYER_RADIUS)                               y = PLAYER_RADIUS;
    if (x > static_cast<float>(screenW) - PLAYER_RADIUS) x = static_cast<float>(screenW) - PLAYER_RADIUS;
    if (y > static_cast<float>(screenH) - PLAYER_RADIUS) y = static_cast<float>(screenH) - PLAYER_RADIUS;

    // Cooldowns
    if (shootTimer      > 0.0f) shootTimer      -= dt;
    if (invincibleTimer > 0.0f) invincibleTimer -= dt;
}

// ========================= ZombieSwarm (SoA) =========================

void ZombieSwarm::allocate(int cap)
{
    capacity = cap;
    count    = 0;
    x       = new float[cap]();
    y       = new float[cap]();
    speed   = new float[cap]();
    health  = new int[cap]();
    isAlive = new bool[cap]();
}

void ZombieSwarm::deallocate()
{
    delete[] x;       x       = nullptr;
    delete[] y;       y       = nullptr;
    delete[] speed;   speed   = nullptr;
    delete[] health;  health  = nullptr;
    delete[] isAlive; isAlive = nullptr;
    count    = 0;
    capacity = 0;
}

void ZombieSwarm::spawnAt(int index, float spawnX, float spawnY, float spd, int hp)
{
    x[index]       = spawnX;
    y[index]       = spawnY;
    speed[index]   = spd;
    health[index]  = hp;
    isAlive[index] = true;
}

void ZombieSwarm::spawnWave(int num, int screenW, int screenH,
                             const std::vector<Obstacle>& obstacles)
{
    float cx = screenW / 2.0f;
    float cy = screenH / 2.0f;

    int spawned = 0;
    int attempts = 0;
    const int maxAttempts = num * 20;

    while (spawned < num && count < capacity && attempts < maxAttempts) {
        attempts++;

        // Spawn near edges (four quadrant bands)
        float sx, sy;
        int edge = rand() % 4;
        if (edge == 0) { // top
            sx = 10.0f + static_cast<float>(rand() % (screenW - 20));
            sy = 10.0f + static_cast<float>(rand() % 60);
        } else if (edge == 1) { // bottom
            sx = 10.0f + static_cast<float>(rand() % (screenW - 20));
            sy = static_cast<float>(screenH - 70 + rand() % 60);
        } else if (edge == 2) { // left
            sx = 10.0f + static_cast<float>(rand() % 60);
            sy = 10.0f + static_cast<float>(rand() % (screenH - 20));
        } else { // right
            sx = static_cast<float>(screenW - 70 + rand() % 60);
            sy = 10.0f + static_cast<float>(rand() % (screenH - 20));
        }

        // Must be at least 150px from player start
        float distFromCenter = sqrtf((sx - cx)*(sx - cx) + (sy - cy)*(sy - cy));
        if (distFromCenter < 150.0f) continue;

        // Don't spawn inside an obstacle
        bool blocked = false;
        for (const auto& obs : obstacles) {
            if (obs.intersects(sx, sy, ZOMBIE_RADIUS + 4.0f)) {
                blocked = true;
                break;
            }
        }
        if (blocked) continue;

        float spd = 35.0f + static_cast<float>(rand() % 35);  // 35–70 px/s
        int   hp  = 1 + rand() % 3;                            // 1–3 hp
        spawnAt(count, sx, sy, spd, hp);
        count++;
        spawned++;
    }
}

int ZombieSwarm::aliveCount() const
{
    int alive = 0;
    for (int i = 0; i < count; i++)
        if (isAlive[i]) alive++;
    return alive;
}

// ========================= BulletPool (SoA) =========================

void BulletPool::allocate(int cap)
{
    capacity = cap;
    count    = 0;
    x       = new float[cap]();
    y       = new float[cap]();
    dirX    = new float[cap]();
    dirY    = new float[cap]();
    speed   = new float[cap]();
    isAlive = new bool[cap]();
}

void BulletPool::deallocate()
{
    delete[] x;       x       = nullptr;
    delete[] y;       y       = nullptr;
    delete[] dirX;    dirX    = nullptr;
    delete[] dirY;    dirY    = nullptr;
    delete[] speed;   speed   = nullptr;
    delete[] isAlive; isAlive = nullptr;
    count    = 0;
    capacity = 0;
}

void BulletPool::spawn(float startX, float startY, float dX, float dY, float spd)
{
    // Reuse a dead slot first
    for (int i = 0; i < count; i++) {
        if (!isAlive[i]) {
            x[i] = startX; y[i] = startY;
            dirX[i] = dX;  dirY[i] = dY;
            speed[i] = spd; isAlive[i] = true;
            return;
        }
    }
    // Otherwise append
    if (count < capacity) {
        x[count] = startX; y[count] = startY;
        dirX[count] = dX;  dirY[count] = dY;
        speed[count] = spd; isAlive[count] = true;
        count++;
    }
}

void BulletPool::update(float dt, int screenW, int screenH)
{
    for (int i = 0; i < count; i++) {
        if (!isAlive[i]) continue;
        x[i] += dirX[i] * speed[i] * dt;
        y[i] += dirY[i] * speed[i] * dt;
        if (x[i] < -10 || x[i] > screenW + 10 ||
            y[i] < -10 || y[i] > screenH + 10)
            isAlive[i] = false;
    }
}
