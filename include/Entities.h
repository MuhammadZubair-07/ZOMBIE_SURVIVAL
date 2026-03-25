// ============================================================
// Entities.h — SoA data layout for Player, Zombies, Bullets
// Structure of Arrays (SoA) maximizes cache & GPU throughput
// ============================================================
#pragma once

#include <cstdlib>
#include <cstring>
#include <cmath>

// --------------- Constants ---------------
constexpr int   MAX_ZOMBIES     = 50000;   // Horde mode capacity
constexpr int   MAX_BULLETS     = 1000;
constexpr float ZOMBIE_RADIUS   = 10.0f;
constexpr float BULLET_RADIUS   = 4.0f;
constexpr float PLAYER_RADIUS   = 15.0f;
constexpr float PLAYER_SPEED    = 200.0f;  // pixels / sec
constexpr float BULLET_SPEED    = 500.0f;  // pixels / sec
constexpr float SHOOT_COOLDOWN  = 0.15f;   // seconds between shots

// ===============================================================
// Player  (single entity — regular struct is fine)
// ===============================================================
struct Player {
    float x, y;
    float speed;
    int   health;
    float aimAngle;          // radians, toward mouse cursor

    // Movement flags (set by input)
    bool moveUp, moveDown, moveLeft, moveRight;
    bool shooting;

    float shootCooldown;
    float shootTimer;
    float invincibleTimer;   // brief invincibility after taking a hit

    void init(float startX, float startY);
    void update(float dt, int screenW, int screenH);
};

// ===============================================================
// ZombieSwarm  (SoA — thousands of zombies in contiguous arrays)
// ===============================================================
struct ZombieSwarm {
    float* x;
    float* y;
    float* speed;
    int*   health;
    bool*  isAlive;

    int count;      // current number of zombies (including dead)
    int capacity;   // allocated size

    void allocate(int cap);
    void deallocate();
    void spawnAt(int index, float spawnX, float spawnY, float spd, int hp);
    void spawnWave(int num, int screenW, int screenH);

    int  aliveCount() const;
};

// ===============================================================
// BulletPool  (SoA — fast iteration for collision checks)
// ===============================================================
struct BulletPool {
    float* x;
    float* y;
    float* dirX;
    float* dirY;
    float* speed;
    bool*  isAlive;

    int count;
    int capacity;

    void allocate(int cap);
    void deallocate();
    void spawn(float startX, float startY, float dX, float dY, float spd);
    void update(float dt, int screenW, int screenH);
};
