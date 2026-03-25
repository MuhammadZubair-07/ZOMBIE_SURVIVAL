// ============================================================
// Game.h — Core Game class: window, loop, input, render
// ============================================================
#pragma once

#include <SDL.h>
#include "Entities.h"
#include "Utils.h"

// Processing mode — switch at runtime with keys 1-4
enum class ProcessorMode {
    SEQUENTIAL,
    OPENMP,
    MPI_MODE,
    GPU
};

// Game state
enum class GameState {
    PLAYING,
    WIN,
    LOSE
};

class Game {
public:
    Game();
    ~Game();

    bool init(const char* title, int width, int height);
    void run();
    void cleanup();

private:
    // ---------- Core loop phases ----------
    void handleInput();
    void update(float dt);
    void render();
    void restart();

    // ---------- SDL ----------
    SDL_Window*   window;
    SDL_Renderer* renderer;
    bool isRunning;

    // ---------- Entities ----------
    Player       player;
    ZombieSwarm  zombies;
    BulletPool   bullets;

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
};
