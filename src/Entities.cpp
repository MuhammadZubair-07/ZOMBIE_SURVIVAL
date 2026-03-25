// ============================================================
// Entities.cpp — Allocation, deallocation, spawn, update
// for Player, ZombieSwarm (SoA), and BulletPool (SoA)
// ============================================================

#include "Entities.h"
#include <cstdlib>
#include <cstring>
#include <cmath>

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

void Player::update(float dt, int screenW, int screenH)
{
    // Compute movement direction
    float dx = 0.0f, dy = 0.0f;
    if (moveUp)    dy -= 1.0f;
    if (moveDown)  dy += 1.0f;
    if (moveLeft)  dx -= 1.0f;
    if (moveRight) dx += 1.0f;

    // Normalize diagonal movement so it isn't faster
    float len = sqrtf(dx * dx + dy * dy);
    if (len > 0.0f) {
        dx /= len;
        dy /= len;
    }

    x += dx * speed * dt;
    y += dy * speed * dt;

    // Clamp to screen bounds
    if (x < PLAYER_RADIUS)                          x = PLAYER_RADIUS;
    if (y < PLAYER_RADIUS)                          y = PLAYER_RADIUS;
    if (x > static_cast<float>(screenW) - PLAYER_RADIUS) x = static_cast<float>(screenW) - PLAYER_RADIUS;
    if (y > static_cast<float>(screenH) - PLAYER_RADIUS) y = static_cast<float>(screenH) - PLAYER_RADIUS;

    // Cooldowns
    if (shootTimer > 0.0f) shootTimer -= dt;
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

void ZombieSwarm::spawnWave(int num, int screenW, int screenH)
{
    float cx = screenW / 2.0f;
    float cy = screenH / 2.0f;

    for (int i = 0; i < num && count < capacity; i++) {
        float sx, sy;
        // Keep spawning until the zombie is at least 150 px from center
        do {
            sx = 20.0f + static_cast<float>(rand() % (screenW - 40));
            sy = 20.0f + static_cast<float>(rand() % (screenH - 40));
        } while (sqrtf((sx - cx) * (sx - cx) + (sy - cy) * (sy - cy)) < 150.0f);

        float spd = 30.0f + static_cast<float>(rand() % 30);   // 30-60 px/s
        spawnAt(count, sx, sy, spd, 1);
        count++;
    }
}

int ZombieSwarm::aliveCount() const
{
    int alive = 0;
    for (int i = 0; i < count; i++) {
        if (isAlive[i]) alive++;
    }
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
            x[i]       = startX;
            y[i]       = startY;
            dirX[i]    = dX;
            dirY[i]    = dY;
            speed[i]   = spd;
            isAlive[i] = true;
            return;
        }
    }
    // Otherwise append
    if (count < capacity) {
        x[count]       = startX;
        y[count]       = startY;
        dirX[count]    = dX;
        dirY[count]    = dY;
        speed[count]   = spd;
        isAlive[count] = true;
        count++;
    }
}

void BulletPool::update(float dt, int screenW, int screenH)
{
    for (int i = 0; i < count; i++) {
        if (!isAlive[i]) continue;

        x[i] += dirX[i] * speed[i] * dt;
        y[i] += dirY[i] * speed[i] * dt;

        // Kill bullet if it leaves the screen
        if (x[i] < 0 || x[i] > screenW || y[i] < 0 || y[i] > screenH)
            isAlive[i] = false;
    }
}
