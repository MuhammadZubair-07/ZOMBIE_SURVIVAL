// ============================================================
// Game.h — Core Game class: window, loop, input, render
// ============================================================
#pragma once

#include <SDL.h>
#include "Entities.h"
#include "Utils.h"
#include <vector>
#include <string>
#include <cstdint>

// Processing mode — switch at runtime with keys 1-4
enum class ProcessorMode {
    SEQUENTIAL,
    OPENMP,
    MPI_MODE,
    GPU
};

// Game state
enum class GameState {
    MENU,
    PLAYING,
    PAUSED,
    WIN,
    LOSE
};

// Health pickup dropped on the map
struct HealthPickup {
    float x, y;
    bool  alive;
    float respawnTimer;   // seconds until it respawns after being collected
};

class Game {
public:
    Game();
    ~Game();

    bool init(const char* title, int width, int height);
    void run();
    void cleanup();

#ifdef USE_MPI
    void initWorker();
    void runWorker();
#endif

private:
    bool mpiStateSynced;
    // ---------- Core loop phases ----------
    void handleInput();
    void update(float dt);
    void render();
    void restart();
    void advancePhase();

    // ---------- Rendering helpers ----------
    void renderBackground();
    void renderObstacles();
    void renderZombies();
    void renderBullets();
    void renderPlayer();
    void renderHealthPickups();
    void renderHUD();
    void renderMenu();
    void renderOverlay();
    void renderPaused();
    void renderPhaseTransition();

    // ---------- Health pickup system ----------
    void initHealthPickups();
    void updateHealthPickups(float dt);
    void spawnHealthPickupAt(int i);

    // ---------- Drawing primitives (SDL-drawn sprites) ----------
    void drawPlayerSprite(int cx, int cy, float angle, bool invincible);
    void drawZombieSprite(int cx, int cy, float angle);
    void drawBuildingSprite(int x, int y, int w, int h, int variant);
    void drawBulletSprite(int cx, int cy);
    void drawFilledCircle(int cx, int cy, int radius, SDL_Color col);
    void drawLine(int x1, int y1, int x2, int y2, SDL_Color col);

    // ---------- Pixel-font text renderer (no SDL_ttf needed) ----------
    void drawChar(char c, int x, int y, int scale, SDL_Color col);
    void drawText(const char* text, int x, int y, int scale, SDL_Color col);
    void drawTextCentered(const char* text, int y, int scale, SDL_Color col);
    int  textWidth(const char* text, int scale) const;

    // ---------- SDL ----------
    SDL_Window*   window;
    SDL_Renderer* renderer;
    bool isRunning;

    // ---------- Entities ----------
    Player       player;
    ZombieSwarm  zombies;
    BulletPool   bullets;
    std::vector<Obstacle> obstacles;

    // ---------- Settings ----------
    int screenWidth, screenHeight;
    ProcessorMode mode;
    GameState     gameState;

    // ---------- Input ----------
    int mouseX, mouseY;

    // ---------- Timing / HUD ----------
    Timer timer;
    int   fps;
    int   frameCount;
    float fpsTimer;
    float totalTime;        // total play time in seconds (paused time excluded)

    // ---------- Benchmarking overlay ----------
    float updateTimeMs;
    float collisionTimeMs;
    Uint64 perfFreq;

    // ---------- Phase system (3 difficulty phases) ----------
    static const int PHASE_COUNT = 3;
    int   currentPhase;                       // 1=Easy, 2=Medium, 3=Hard
    int   totalKills;
    int   phaseZombieCount[PHASE_COUNT];      // {30, 70, 150}
    float phaseSpeedMult[PHASE_COUNT];        // {1.0f, 1.35f, 1.8f}

    // Phase transition banner
    float phaseTransitionTimer;
    bool  phaseTransitionActive;

    // ---------- Health pickups ----------
    static const int MAX_HEALTH_PICKUPS = 3;
    static const int HEALTH_RESTORE     = 25;   // HP restored per pickup
    static const int PICKUP_RESPAWN_SEC = 8;    // seconds before respawn
    HealthPickup healthPickups[MAX_HEALTH_PICKUPS];

    // Legacy compatibility
    int wave;
};
