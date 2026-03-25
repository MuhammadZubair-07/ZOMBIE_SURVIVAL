// ============================================================
// Game.cpp — Core game loop, input handling, rendering
// ============================================================

#include "Game.h"
#include "Processors.h"
#include <cstdio>
#include <cmath>

// =============================== Ctor / Dtor ===============================

Game::Game()
    : window(nullptr), renderer(nullptr), isRunning(false),
      screenWidth(800), screenHeight(600),
      mode(ProcessorMode::SEQUENTIAL), gameState(GameState::PLAYING),
      mouseX(400), mouseY(300),
      fps(0), frameCount(0), fpsTimer(0.0f)
{}

Game::~Game() { cleanup(); }

// =============================== Init ===============================

bool Game::init(const char* title, int width, int height)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("[ERROR] SDL_Init: %s\n", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_SHOWN);

    if (!window) {
        printf("[ERROR] SDL_CreateWindow: %s\n", SDL_GetError());
        return false;
    }

    renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!renderer) {
        printf("[ERROR] SDL_CreateRenderer: %s\n", SDL_GetError());
        return false;
    }

    // Enable alpha blending for overlays
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    screenWidth  = width;
    screenHeight = height;

    // ---- Entities ----
    player.init(width / 2.0f, height / 2.0f);
    zombies.allocate(MAX_ZOMBIES);
    bullets.allocate(MAX_BULLETS);
    zombies.spawnWave(200, width, height);   // initial wave

    timer.init();
    isRunning = true;
    gameState = GameState::PLAYING;
    return true;
}

// =============================== Main Loop ===============================

void Game::run()
{
    while (isRunning) {
        timer.tick();
        handleInput();
        update(timer.deltaTime);
        render();
    }
}

// =============================== Input ===============================

void Game::handleInput()
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {

        case SDL_QUIT:
            isRunning = false;
            break;

        case SDL_KEYDOWN:
            switch (e.key.keysym.scancode) {
                case SDL_SCANCODE_W: player.moveUp    = true; break;
                case SDL_SCANCODE_S: player.moveDown  = true; break;
                case SDL_SCANCODE_A: player.moveLeft  = true; break;
                case SDL_SCANCODE_D: player.moveRight = true; break;

                case SDL_SCANCODE_ESCAPE: isRunning = false; break;

                // Restart
                case SDL_SCANCODE_R: restart(); break;

                // Horde mode — spawn 10 000 more zombies
                case SDL_SCANCODE_H:
                    zombies.spawnWave(10000, screenWidth, screenHeight);
                    break;

                // Processing-mode selection
                case SDL_SCANCODE_1: mode = ProcessorMode::SEQUENTIAL; break;
                case SDL_SCANCODE_2: mode = ProcessorMode::OPENMP;     break;
                case SDL_SCANCODE_3: mode = ProcessorMode::MPI_MODE;   break;
                case SDL_SCANCODE_4: mode = ProcessorMode::GPU;        break;

                default: break;
            }
            break;

        case SDL_KEYUP:
            switch (e.key.keysym.scancode) {
                case SDL_SCANCODE_W: player.moveUp    = false; break;
                case SDL_SCANCODE_S: player.moveDown  = false; break;
                case SDL_SCANCODE_A: player.moveLeft  = false; break;
                case SDL_SCANCODE_D: player.moveRight = false; break;
                default: break;
            }
            break;

        case SDL_MOUSEMOTION:
            mouseX = e.motion.x;
            mouseY = e.motion.y;
            break;

        case SDL_MOUSEBUTTONDOWN:
            if (e.button.button == SDL_BUTTON_LEFT)
                player.shooting = true;
            break;

        case SDL_MOUSEBUTTONUP:
            if (e.button.button == SDL_BUTTON_LEFT)
                player.shooting = false;
            break;
        }
    }

    // Aim toward mouse
    player.aimAngle = atan2f(
        static_cast<float>(mouseY) - player.y,
        static_cast<float>(mouseX) - player.x);
}

// =============================== Update ===============================

void Game::update(float dt)
{
    if (gameState != GameState::PLAYING) return;

    // ---- Player ----
    player.update(dt, screenWidth, screenHeight);

    // ---- Shoot bullets ----
    if (player.shooting && player.shootTimer <= 0.0f) {
        float dx = cosf(player.aimAngle);
        float dy = sinf(player.aimAngle);
        bullets.spawn(player.x, player.y, dx, dy, BULLET_SPEED);
        player.shootTimer = player.shootCooldown;
    }

    // ---- Bullets ----
    bullets.update(dt, screenWidth, screenHeight);

    // ---- Zombies (dispatch to selected processor) ----
    switch (mode) {
        case ProcessorMode::SEQUENTIAL:
            updateZombiesSequential(zombies, player, dt);
            break;
#ifdef USE_OPENMP
        case ProcessorMode::OPENMP:
            updateZombiesOpenMP(zombies, player, dt);
            break;
#endif
        default:
            updateZombiesSequential(zombies, player, dt);
            break;
    }

    // ---- Collisions ----
    checkCollisionsSequential(bullets, zombies, player);

    // ---- Win / Lose ----
    int alive = zombies.aliveCount();
    if (alive == 0 && zombies.count > 0)
        gameState = GameState::WIN;
    if (player.health <= 0)
        gameState = GameState::LOSE;

    // ---- FPS counter (update window title once per second) ----
    fpsTimer += dt;
    frameCount++;
    if (fpsTimer >= 1.0f) {
        fps = frameCount;
        frameCount = 0;
        fpsTimer  -= 1.0f;

        const char* modeStr = "Sequential";
        if (mode == ProcessorMode::OPENMP)   modeStr = "OpenMP";
        if (mode == ProcessorMode::MPI_MODE) modeStr = "MPI";
        if (mode == ProcessorMode::GPU)      modeStr = "GPU";

        char title[256];
        snprintf(title, sizeof(title),
            "Zombie Survival | %s | FPS: %d | Zombies: %d/%d | HP: %d",
            modeStr, fps, alive, zombies.count, player.health);
        SDL_SetWindowTitle(window, title);
    }
}

// =============================== Render ===============================

void Game::render()
{
    // ---- Background ----
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    // ---- Zombies (red) ----
    SDL_SetRenderDrawColor(renderer, 200, 30, 30, 255);
    for (int i = 0; i < zombies.count; i++) {
        if (!zombies.isAlive[i]) continue;
        SDL_Rect r = {
            static_cast<int>(zombies.x[i] - ZOMBIE_RADIUS),
            static_cast<int>(zombies.y[i] - ZOMBIE_RADIUS),
            static_cast<int>(ZOMBIE_RADIUS * 2),
            static_cast<int>(ZOMBIE_RADIUS * 2)
        };
        SDL_RenderFillRect(renderer, &r);
    }

    // ---- Bullets (yellow) ----
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    for (int i = 0; i < bullets.count; i++) {
        if (!bullets.isAlive[i]) continue;
        SDL_Rect r = {
            static_cast<int>(bullets.x[i] - BULLET_RADIUS),
            static_cast<int>(bullets.y[i] - BULLET_RADIUS),
            static_cast<int>(BULLET_RADIUS * 2),
            static_cast<int>(BULLET_RADIUS * 2)
        };
        SDL_RenderFillRect(renderer, &r);
    }

    // ---- Player (green, flashes brighter when invincible) ----
    if (player.invincibleTimer > 0)
        SDL_SetRenderDrawColor(renderer, 0, 255, 100, 220);
    else
        SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255);

    SDL_Rect pr = {
        static_cast<int>(player.x - PLAYER_RADIUS),
        static_cast<int>(player.y - PLAYER_RADIUS),
        static_cast<int>(PLAYER_RADIUS * 2),
        static_cast<int>(PLAYER_RADIUS * 2)
    };
    SDL_RenderFillRect(renderer, &pr);

    // ---- Aim line (white) ----
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
    int lx = static_cast<int>(player.x + cosf(player.aimAngle) * 50.0f);
    int ly = static_cast<int>(player.y + sinf(player.aimAngle) * 50.0f);
    SDL_RenderDrawLine(renderer,
        static_cast<int>(player.x), static_cast<int>(player.y), lx, ly);

    // ---- Health bar (top-left) ----
    SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
    SDL_Rect hpBg = { 10, 10, 200, 20 };
    SDL_RenderFillRect(renderer, &hpBg);

    int hpW = static_cast<int>(200.0f * (player.health / 100.0f));
    if (hpW < 0) hpW = 0;
    if (player.health > 50)
        SDL_SetRenderDrawColor(renderer, 0, 220, 0, 255);
    else if (player.health > 25)
        SDL_SetRenderDrawColor(renderer, 255, 200, 0, 255);
    else
        SDL_SetRenderDrawColor(renderer, 220, 0, 0, 255);

    SDL_Rect hpFill = { 10, 10, hpW, 20 };
    SDL_RenderFillRect(renderer, &hpFill);

    // ---- Win / Lose overlay ----
    if (gameState == GameState::WIN) {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 70);
        SDL_Rect fs = { 0, 0, screenWidth, screenHeight };
        SDL_RenderFillRect(renderer, &fs);
    } else if (gameState == GameState::LOSE) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 70);
        SDL_Rect fs = { 0, 0, screenWidth, screenHeight };
        SDL_RenderFillRect(renderer, &fs);
    }

    SDL_RenderPresent(renderer);
}

// =============================== Restart ===============================

void Game::restart()
{
    zombies.deallocate();
    bullets.deallocate();

    zombies.allocate(MAX_ZOMBIES);
    bullets.allocate(MAX_BULLETS);

    player.init(screenWidth / 2.0f, screenHeight / 2.0f);
    zombies.spawnWave(200, screenWidth, screenHeight);

    gameState = GameState::PLAYING;
    fps = 0;
    frameCount = 0;
    fpsTimer = 0.0f;
}

// =============================== Cleanup ===============================

void Game::cleanup()
{
    zombies.deallocate();
    bullets.deallocate();

    if (renderer) { SDL_DestroyRenderer(renderer); renderer = nullptr; }
    if (window)   { SDL_DestroyWindow(window);     window   = nullptr; }
    SDL_Quit();
}
