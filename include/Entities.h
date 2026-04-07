// ============================================================
// Entities.h — SoA data layout for Player, Zombies, Bullets
// Structure of Arrays (SoA) maximizes cache & GPU throughput
// ============================================================
#pragma once

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>

// --------------- Constants ---------------
constexpr int   MAX_ZOMBIES     = 50000;   // Horde mode capacity
constexpr int   MAX_BULLETS     = 2000;
constexpr float ZOMBIE_RADIUS   = 10.0f;
constexpr float BULLET_RADIUS   = 5.0f;
constexpr float PLAYER_RADIUS   = 14.0f;
constexpr float PLAYER_SPEED    = 220.0f;  // pixels / sec
constexpr float BULLET_SPEED    = 600.0f;  // pixels / sec
constexpr float SHOOT_COOLDOWN  = 0.12f;   // seconds between shots

// ===============================================================
// Obstacle  (axis-aligned rectangle = building footprint)
// ===============================================================
struct Obstacle {
    float x, y;
    float width, height;
    int   variant;          // visual style 0/1/2

    // AABB collision: returns true if circle (cx,cy,radius) overlaps this rect
    bool intersects(float cx, float cy, float radius) const;
};

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
    void update(float dt, int screenW, int screenH,
                const std::vector<Obstacle>& obstacles);
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

    int count;      // current number of zombies (including dead slots)
    int capacity;   // allocated size

    void allocate(int cap);
    void deallocate();
    void spawnAt(int index, float spawnX, float spawnY, float spd, int hp);
    void spawnWave(int num, int screenW, int screenH,
                   const std::vector<Obstacle>& obstacles);

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
