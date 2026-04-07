// ============================================================
// Game.cpp — Core game loop, rendering, phase system, pause
// Pixel-font renderer embedded (no SDL_ttf dependency)
// ============================================================

#include "Game.h"
#include "Processors.h"
#include <cstdio>
#include <cmath>
#include <cstring>
#include <algorithm>

// Helper: create an SDL_Rect lvalue so we can take its address in C++
// (C99 compound literals &(SDL_Rect){...} are rvalues in C++ — not allowed)
static inline SDL_Rect mkr(int x, int y, int w, int h) {
    SDL_Rect r = {x, y, w, h}; return r;
}
#define R(x,y,w,h) (mkr((x),(y),(w),(h)))

// =============================== Pixel Font (5x7) ===============================
// Index = ASCII - 32.  5 columns, 7 rows.  Bit4=leftmost col.
static const uint8_t FONT5x7[62][7] = {
/*' '*/{ 0x00,0x00,0x00,0x00,0x00,0x00,0x00 },
/*'!'*/{ 0x04,0x04,0x04,0x04,0x00,0x04,0x00 },
/*'"'*/{ 0x0A,0x0A,0x00,0x00,0x00,0x00,0x00 },
/*'#'*/{ 0x0A,0x1F,0x0A,0x0A,0x1F,0x0A,0x00 },
/*'$'*/{ 0x04,0x0F,0x14,0x0E,0x05,0x1E,0x04 },
/*'%'*/{ 0x18,0x19,0x02,0x04,0x08,0x13,0x03 },
/*'&'*/{ 0x0C,0x12,0x14,0x08,0x15,0x12,0x0D },
/*'''*/{ 0x04,0x04,0x08,0x00,0x00,0x00,0x00 },
/*'('*/{ 0x02,0x04,0x08,0x08,0x08,0x04,0x02 },
/*')'*/{ 0x08,0x04,0x02,0x02,0x02,0x04,0x08 },
/*'*'*/{ 0x00,0x04,0x15,0x0E,0x15,0x04,0x00 },
/*'+'*/{ 0x00,0x04,0x04,0x1F,0x04,0x04,0x00 },
/*','*/{ 0x00,0x00,0x00,0x00,0x0C,0x04,0x08 },
/*'-'*/{ 0x00,0x00,0x00,0x1F,0x00,0x00,0x00 },
/*'.'*/{ 0x00,0x00,0x00,0x00,0x00,0x0C,0x0C },
/*'/'*/{ 0x01,0x01,0x02,0x04,0x08,0x10,0x10 },
/*'0'*/{ 0x0E,0x11,0x13,0x15,0x19,0x11,0x0E },
/*'1'*/{ 0x04,0x0C,0x04,0x04,0x04,0x04,0x0E },
/*'2'*/{ 0x0E,0x11,0x01,0x06,0x08,0x10,0x1F },
/*'3'*/{ 0x1F,0x01,0x02,0x06,0x01,0x11,0x0E },
/*'4'*/{ 0x02,0x06,0x0A,0x12,0x1F,0x02,0x02 },
/*'5'*/{ 0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E },
/*'6'*/{ 0x06,0x08,0x10,0x1E,0x11,0x11,0x0E },
/*'7'*/{ 0x1F,0x01,0x02,0x04,0x08,0x08,0x08 },
/*'8'*/{ 0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E },
/*'9'*/{ 0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C },
/*':'*/{ 0x00,0x0C,0x0C,0x00,0x0C,0x0C,0x00 },
/*';'*/{ 0x00,0x0C,0x0C,0x00,0x0C,0x04,0x08 },
/*'<'*/{ 0x02,0x04,0x08,0x10,0x08,0x04,0x02 },
/*'='*/{ 0x00,0x00,0x1F,0x00,0x1F,0x00,0x00 },
/*'>'*/{ 0x08,0x04,0x02,0x01,0x02,0x04,0x08 },
/*'?'*/{ 0x0E,0x11,0x01,0x06,0x04,0x00,0x04 },
/*'@'*/{ 0x0E,0x11,0x17,0x15,0x17,0x10,0x0E },
/*'A'*/{ 0x0E,0x11,0x11,0x1F,0x11,0x11,0x11 },
/*'B'*/{ 0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E },
/*'C'*/{ 0x0E,0x11,0x10,0x10,0x10,0x11,0x0E },
/*'D'*/{ 0x1C,0x12,0x11,0x11,0x11,0x12,0x1C },
/*'E'*/{ 0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F },
/*'F'*/{ 0x1F,0x10,0x10,0x1E,0x10,0x10,0x10 },
/*'G'*/{ 0x0E,0x11,0x10,0x17,0x11,0x11,0x0F },
/*'H'*/{ 0x11,0x11,0x11,0x1F,0x11,0x11,0x11 },
/*'I'*/{ 0x0E,0x04,0x04,0x04,0x04,0x04,0x0E },
/*'J'*/{ 0x07,0x02,0x02,0x02,0x02,0x12,0x0C },
/*'K'*/{ 0x11,0x12,0x14,0x18,0x14,0x12,0x11 },
/*'L'*/{ 0x10,0x10,0x10,0x10,0x10,0x10,0x1F },
/*'M'*/{ 0x11,0x1B,0x15,0x15,0x11,0x11,0x11 },
/*'N'*/{ 0x11,0x19,0x15,0x13,0x11,0x11,0x11 },
/*'O'*/{ 0x0E,0x11,0x11,0x11,0x11,0x11,0x0E },
/*'P'*/{ 0x1E,0x11,0x11,0x1E,0x10,0x10,0x10 },
/*'Q'*/{ 0x0E,0x11,0x11,0x11,0x15,0x12,0x0D },
/*'R'*/{ 0x1E,0x11,0x11,0x1E,0x14,0x12,0x11 },
/*'S'*/{ 0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E },
/*'T'*/{ 0x1F,0x04,0x04,0x04,0x04,0x04,0x04 },
/*'U'*/{ 0x11,0x11,0x11,0x11,0x11,0x11,0x0E },
/*'V'*/{ 0x11,0x11,0x11,0x11,0x0A,0x0A,0x04 },
/*'W'*/{ 0x11,0x11,0x15,0x15,0x15,0x0A,0x0A },
/*'X'*/{ 0x11,0x0A,0x0A,0x04,0x0A,0x0A,0x11 },
/*'Y'*/{ 0x11,0x11,0x0A,0x04,0x04,0x04,0x04 },
/*'Z'*/{ 0x1F,0x01,0x02,0x04,0x08,0x10,0x1F },
/*'['*/{ 0x0E,0x08,0x08,0x08,0x08,0x08,0x0E },
/*'\'*/{ 0x10,0x10,0x08,0x04,0x02,0x01,0x01 },
/*']'*/{ 0x0E,0x02,0x02,0x02,0x02,0x02,0x0E },
};

// =============================== Text Renderer ===============================

void Game::drawChar(char c, int x, int y, int scale, SDL_Color col)
{
    if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
    if (c < 32 || c > 93) c = '?';  // extended: now covers [ \ ] (ASCII 91-93)
    int idx = (int)c - 32;
    SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, col.a);
    for (int row = 0; row < 7; row++) {
        uint8_t bits = FONT5x7[idx][row];
        for (int col2 = 0; col2 < 5; col2++) {
            if (bits & (1 << (4 - col2))) {
                SDL_Rect px = { x + col2 * scale, y + row * scale, scale, scale };
                SDL_RenderFillRect(renderer, &px);
            }
        }
    }
}

void Game::drawText(const char* text, int x, int y, int scale, SDL_Color col)
{
    int cx = x;
    while (*text) {
        drawChar(*text, cx, y, scale, col);
        cx += 6 * scale;
        text++;
    }
}

int Game::textWidth(const char* text, int scale) const
{
    int len = 0;
    const char* p = text;
    while (*p++) len++;
    if (len == 0) return 0;
    return len * 6 * scale - scale;
}

void Game::drawTextCentered(const char* text, int y, int scale, SDL_Color col)
{
    int w = textWidth(text, scale);
    drawText(text, screenWidth / 2 - w / 2, y, scale, col);
}

// =============================== Ctor / Dtor ===============================

Game::Game()
    : window(nullptr), renderer(nullptr), isRunning(false),
      screenWidth(1280), screenHeight(720),
      mode(ProcessorMode::SEQUENTIAL), gameState(GameState::MENU),
      mouseX(640), mouseY(360),
      fps(0), frameCount(0), fpsTimer(0.0f), totalTime(0.0f),
      updateTimeMs(0.0f), collisionTimeMs(0.0f), perfFreq(0),
      currentPhase(1), totalKills(0), wave(1),
      phaseTransitionTimer(0.0f), phaseTransitionActive(false)
{
    phaseZombieCount[0] = 30;
    phaseZombieCount[1] = 70;
    phaseZombieCount[2] = 150;
    phaseSpeedMult[0]   = 1.0f;
    phaseSpeedMult[1]   = 1.35f;
    phaseSpeedMult[2]   = 1.80f;
}

Game::~Game() { cleanup(); }

// =============================== Init ===============================

bool Game::init(const char* title, int width, int height)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        printf("[ERROR] SDL_Init: %s\n", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow(title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) { printf("[ERROR] SDL_CreateWindow: %s\n", SDL_GetError()); return false; }

    renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) { printf("[ERROR] SDL_CreateRenderer: %s\n", SDL_GetError()); return false; }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    screenWidth  = width;
    screenHeight = height;
    perfFreq     = SDL_GetPerformanceFrequency();

    player.init(width / 2.0f, height / 2.0f);
    zombies.allocate(MAX_ZOMBIES);
    bullets.allocate(MAX_BULLETS);

    // Static obstacles
    obstacles.push_back({80.0f,  60.0f,  140.0f, 90.0f,  0});
    obstacles.push_back({450.0f, 50.0f,  120.0f, 80.0f,  1});
    obstacles.push_back({950.0f, 60.0f,  160.0f, 100.0f, 2});
    obstacles.push_back({80.0f,  550.0f, 140.0f, 100.0f, 1});
    obstacles.push_back({540.0f, 560.0f, 180.0f, 100.0f, 0});
    obstacles.push_back({1000.0f,540.0f, 140.0f, 110.0f, 2});
    obstacles.push_back({350.0f, 260.0f, 100.0f, 120.0f, 1});
    obstacles.push_back({820.0f, 270.0f, 120.0f, 100.0f, 0});

    // Spawn Phase 1
    currentPhase = 1;
    zombies.spawnWave(phaseZombieCount[0], width, height, obstacles);
    // Phase 1 uses base speed (multiplier 1.0 — no adjustment needed)

    initHealthPickups();
    timer.init();
    isRunning = true;
    gameState = GameState::MENU;
    mpiStateSynced = false;
    return true;
}

#ifdef USE_MPI
#include <mpi.h>
#include "Processors.h"

void Game::initWorker()
{
    screenWidth = 1280;
    screenHeight = 720;
    zombies.allocate(MAX_ZOMBIES);
    bullets.allocate(MAX_BULLETS);
    obstacles.push_back({80.0f,  60.0f,  140.0f, 90.0f,  0});
    obstacles.push_back({450.0f, 50.0f,  120.0f, 80.0f,  1});
    obstacles.push_back({950.0f, 60.0f,  160.0f, 100.0f, 2});
    obstacles.push_back({80.0f,  550.0f, 140.0f, 100.0f, 1});
    obstacles.push_back({540.0f, 560.0f, 180.0f, 100.0f, 0});
    obstacles.push_back({1000.0f,540.0f, 140.0f, 110.0f, 2});
    obstacles.push_back({350.0f, 260.0f, 100.0f, 120.0f, 1});
    obstacles.push_back({820.0f, 270.0f, 120.0f, 100.0f, 0});
}

void Game::runWorker()
{
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    while (true) {
        int cmd = 0;
        MPI_Bcast(&cmd, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if (cmd == 0) break;

        if (cmd == 1) { // Sync state
            MPI_Bcast(zombies.x, MAX_ZOMBIES, MPI_FLOAT, 0, MPI_COMM_WORLD);
            MPI_Bcast(zombies.y, MAX_ZOMBIES, MPI_FLOAT, 0, MPI_COMM_WORLD);
            MPI_Bcast(zombies.speed, MAX_ZOMBIES, MPI_FLOAT, 0, MPI_COMM_WORLD);
            MPI_Bcast(zombies.isAlive, MAX_ZOMBIES, MPI_C_BOOL, 0, MPI_COMM_WORLD);
            MPI_Bcast(zombies.health, MAX_ZOMBIES, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Bcast(&zombies.count, 1, MPI_INT, 0, MPI_COMM_WORLD);
        } else if (cmd == 2) { // Update
            float dt;
            MPI_Bcast(&dt, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);
            updateZombiesMPI(zombies, player, dt, obstacles);
            checkCollisionsMPI(bullets, zombies, player, obstacles);
        }
    }
}
#endif

// =============================== Main Loop ===============================

void Game::run()
{
    while (isRunning) {
        timer.tick();
        float dt = timer.deltaTime;
        if (dt > 0.05f) dt = 0.05f;
        handleInput();
        update(dt);
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

                // ESC: pause if playing, resume if paused, quit if in menu
                case SDL_SCANCODE_ESCAPE:
                    if      (gameState == GameState::PLAYING) gameState = GameState::PAUSED;
                    else if (gameState == GameState::PAUSED)  gameState = GameState::PLAYING;
                    else if (gameState == GameState::MENU)    isRunning = false;
                    break;

                // P: toggle pause
                case SDL_SCANCODE_P:
                    if      (gameState == GameState::PLAYING) gameState = GameState::PAUSED;
                    else if (gameState == GameState::PAUSED)  gameState = GameState::PLAYING;
                    break;

                case SDL_SCANCODE_R:
                    restart();
                    break;

                case SDL_SCANCODE_RETURN:
                case SDL_SCANCODE_SPACE:
                    if      (gameState == GameState::MENU)    gameState = GameState::PLAYING;
                    else if (gameState == GameState::PAUSED)  gameState = GameState::PLAYING;
                    else if (gameState == GameState::WIN || gameState == GameState::LOSE) restart();
                    break;

                // Horde bonus wave (doesn't reset phase)
                case SDL_SCANCODE_H:
                    if (gameState == GameState::PLAYING) {
                        zombies.spawnWave(500, screenWidth, screenHeight, obstacles);
                        mpiStateSynced = false;
                    }
                    break;

                // Processing mode selection
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
            if (e.button.button == SDL_BUTTON_LEFT) {
                if (gameState == GameState::MENU)
                    gameState = GameState::PLAYING;
                else if (gameState == GameState::PLAYING)
                    player.shooting = true;
            }
            break;

        case SDL_MOUSEBUTTONUP:
            if (e.button.button == SDL_BUTTON_LEFT)
                player.shooting = false;
            break;
        }
    }

    player.aimAngle = atan2f(
        static_cast<float>(mouseY) - player.y,
        static_cast<float>(mouseX) - player.x);
}

// =============================== Advance Phase ===============================

void Game::advancePhase()
{
    currentPhase++;
    if (currentPhase > PHASE_COUNT) {
        gameState = GameState::WIN;
        return;
    }

    zombies.deallocate();
    zombies.allocate(MAX_ZOMBIES);

    int cnt = phaseZombieCount[currentPhase - 1];
    zombies.spawnWave(cnt, screenWidth, screenHeight, obstacles);

    // Apply speed multiplier for this phase
    float mult = phaseSpeedMult[currentPhase - 1];
    for (int i = 0; i < zombies.count; i++) {
        if (zombies.isAlive[i])
            zombies.speed[i] *= mult;
    }

    mpiStateSynced = false;

    wave = currentPhase;
    phaseTransitionTimer  = 3.0f;
    phaseTransitionActive = true;
}

// =============================== Update ===============================

void Game::update(float dt)
{
    // Phase transition banner countdown (runs even when playing)
    if (phaseTransitionActive) {
        phaseTransitionTimer -= dt;
        if (phaseTransitionTimer <= 0.0f) {
            phaseTransitionTimer  = 0.0f;
            phaseTransitionActive = false;
        }
    }

    if (gameState != GameState::PLAYING) return;

    totalTime += dt;

    // Player
    player.update(dt, screenWidth, screenHeight, obstacles);

    // Shoot bullets
    if (player.shooting && player.shootTimer <= 0.0f) {
        float dx = cosf(player.aimAngle);
        float dy = sinf(player.aimAngle);
        bullets.spawn(player.x, player.y, dx, dy, BULLET_SPEED);
        player.shootTimer = player.shootCooldown;
    }

    // Bullets
    bullets.update(dt, screenWidth, screenHeight);

    if (mode != ProcessorMode::MPI_MODE) {
        mpiStateSynced = false;
    }

    // Zombie Update (parallel mode dispatch)
    Uint64 t0 = SDL_GetPerformanceCounter();
    switch (mode) {
        case ProcessorMode::SEQUENTIAL:
            updateZombiesSequential(zombies, player, dt, obstacles);
            break;
#ifdef USE_OPENMP
        case ProcessorMode::OPENMP:
            updateZombiesOpenMP(zombies, player, dt, obstacles);
            break;
#endif
#ifdef USE_MPI
        case ProcessorMode::MPI_MODE: {
            if (!mpiStateSynced) {
                int cmd = 1; MPI_Bcast(&cmd, 1, MPI_INT, 0, MPI_COMM_WORLD);
                MPI_Bcast(zombies.x, MAX_ZOMBIES, MPI_FLOAT, 0, MPI_COMM_WORLD);
                MPI_Bcast(zombies.y, MAX_ZOMBIES, MPI_FLOAT, 0, MPI_COMM_WORLD);
                MPI_Bcast(zombies.speed, MAX_ZOMBIES, MPI_FLOAT, 0, MPI_COMM_WORLD);
                MPI_Bcast(zombies.isAlive, MAX_ZOMBIES, MPI_C_BOOL, 0, MPI_COMM_WORLD);
                MPI_Bcast(zombies.health, MAX_ZOMBIES, MPI_INT, 0, MPI_COMM_WORLD);
                MPI_Bcast(&zombies.count, 1, MPI_INT, 0, MPI_COMM_WORLD);
                mpiStateSynced = true;
            }
            int cmd = 2; MPI_Bcast(&cmd, 1, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Bcast(&dt, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);
            updateZombiesMPI(zombies, player, dt, obstacles);
            break;
        }
#endif
        default:
            updateZombiesSequential(zombies, player, dt, obstacles);
            break;
    }
    Uint64 t1 = SDL_GetPerformanceCounter();
    updateTimeMs = (float)(t1 - t0) * 1000.0f / (float)perfFreq;

    // Collisions
    int prevAlive = zombies.aliveCount();
    Uint64 c0 = SDL_GetPerformanceCounter();
    switch (mode) {
#ifdef USE_OPENMP
        case ProcessorMode::OPENMP:
            checkCollisionsOpenMP(bullets, zombies, player, obstacles);
            break;
#endif
#ifdef USE_MPI
        case ProcessorMode::MPI_MODE:
            checkCollisionsMPI(bullets, zombies, player, obstacles);
            break;
#endif
#ifdef USE_CUDA
        case ProcessorMode::GPU:
            checkCollisionsGPU(bullets, zombies, player, obstacles);
            break;
#endif
        default:
            checkCollisionsSequential(bullets, zombies, player, obstacles);
            break;
    }
    Uint64 c1 = SDL_GetPerformanceCounter();
    collisionTimeMs = (float)(c1 - c0) * 1000.0f / (float)perfFreq;
    totalKills += prevAlive - zombies.aliveCount();

    // Check win/lose/phase advance
    int alive = zombies.aliveCount();
    if (alive == 0 && zombies.count > 0) {
        advancePhase(); // increments phase, or sets WIN if done
    }
    if (player.health <= 0)
        gameState = GameState::LOSE;

    // Health pickups
    updateHealthPickups(dt);

    // FPS + window title
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
        const char* phaseNames[] = {"Easy","Medium","Hard"};
        const char* pname = (currentPhase >= 1 && currentPhase <= 3)
                            ? phaseNames[currentPhase-1] : "?";

        char title[256];
        snprintf(title, sizeof(title),
            "Zombie Survival | %s | FPS:%d | Phase:%s | Zombies:%d | HP:%d | Kills:%d",
            modeStr, fps, pname, alive, player.health, totalKills);
        SDL_SetWindowTitle(window, title);
    }
}

// =============================== Drawing Primitives ===============================

void Game::drawFilledCircle(int cx, int cy, int radius, SDL_Color col)
{
    SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, col.a);
    for (int dy = -radius; dy <= radius; dy++) {
        int dx = (int)sqrtf((float)(radius*radius - dy*dy));
        SDL_RenderDrawLine(renderer, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

void Game::drawLine(int x1, int y1, int x2, int y2, SDL_Color col)
{
    SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, col.a);
    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
}

void Game::drawPlayerSprite(int cx, int cy, float angle, bool invincible)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 80);
    for (int dy = -8; dy <= 8; dy++) {
        int dx = (int)sqrtf((float)(64 - dy*dy));
        SDL_RenderDrawLine(renderer, cx-dx+3, cy+dy+4, cx+dx+3, cy+dy+4);
    }
    SDL_Color bodyCol = invincible ? SDL_Color{255,120,0,255} : SDL_Color{40,130,50,255};
    drawFilledCircle(cx, cy, 13, bodyCol);
    SDL_SetRenderDrawColor(renderer, 20, 80, 30, 255);
    for (int a = 0; a < 360; a += 10) {
        float ra = a * 3.14159f / 180.0f;
        SDL_RenderDrawPoint(renderer, cx+(int)(cosf(ra)*13.5f), cy+(int)(sinf(ra)*13.5f));
    }
    drawFilledCircle(cx, cy-2, 6, {210,160,110,255});
    SDL_Color helmCol = {50,70,40,255};
    for (int dy = -6; dy <= 0; dy++) {
        int dx = (int)sqrtf(std::max(0.0f,(float)(36-dy*dy)));
        SDL_SetRenderDrawColor(renderer, helmCol.r, helmCol.g, helmCol.b, 255);
        SDL_RenderDrawLine(renderer, cx-dx, cy-2+dy, cx+dx, cy-2+dy);
    }
    float gx = cosf(angle), gy = sinf(angle);
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    for (int t = 8; t <= 22; t++) {
        SDL_RenderDrawPoint(renderer, cx+(int)(gx*t),          cy+(int)(gy*t));
        SDL_RenderDrawPoint(renderer, cx+(int)(gx*t+gy*1.5f),  cy+(int)(gy*t-gx*1.5f));
        SDL_RenderDrawPoint(renderer, cx+(int)(gx*t-gy*1.5f),  cy+(int)(gy*t+gx*1.5f));
    }
    if (player.shootTimer > player.shootCooldown * 0.5f)
        drawFilledCircle(cx+(int)(gx*24), cy+(int)(gy*24), 4, {255,200,50,220});
}

void Game::drawZombieSprite(int cx, int cy, float angle)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 60);
    for (int dy = -7; dy <= 7; dy++) {
        int dx = (int)sqrtf((float)(49-dy*dy));
        SDL_RenderDrawLine(renderer, cx-dx+2, cy+dy+4, cx+dx+2, cy+dy+4);
    }
    drawFilledCircle(cx, cy, 10, {80,140,60,255});
    SDL_SetRenderDrawColor(renderer, 40, 80, 20, 255);
    for (int a = 0; a < 360; a += 15) {
        float ra = a*3.14159f/180.0f;
        SDL_RenderDrawPoint(renderer, cx+(int)(cosf(ra)*10.5f), cy+(int)(sinf(ra)*10.5f));
    }
    drawFilledCircle(cx, cy, 7, {140,180,100,255});
    float ex = cosf(angle), ey = sinf(angle);
    drawFilledCircle(cx+(int)(ex*4-ey*3), cy+(int)(ey*4+ex*3), 2, {255,30,30,255});
    drawFilledCircle(cx+(int)(ex*4+ey*3), cy+(int)(ey*4-ex*3), 2, {255,30,30,255});
    SDL_SetRenderDrawColor(renderer, 100, 160, 60, 255);
    float ax = cosf(angle), ay = sinf(angle);
    for (int t = 8; t <= 18; t++) {
        SDL_RenderDrawPoint(renderer, cx+(int)(ax*t-ay*4), cy+(int)(ay*t+ax*4));
        SDL_RenderDrawPoint(renderer, cx+(int)(ax*t-ay*5), cy+(int)(ay*t+ax*5));
        SDL_RenderDrawPoint(renderer, cx+(int)(ax*t+ay*4), cy+(int)(ay*t-ax*4));
        SDL_RenderDrawPoint(renderer, cx+(int)(ax*t+ay*5), cy+(int)(ay*t-ax*5));
    }
}

void Game::drawBuildingSprite(int x, int y, int w, int h, int variant)
{
    SDL_Color wallColors[3] = {{100,80,60,255},{70,90,110,255},{90,70,70,255}};
    SDL_Color roofColors[3] = {{80,60,40,255},{50,70,90,255},{70,50,50,255}};
    int v = variant % 3;
    SDL_SetRenderDrawColor(renderer,wallColors[v].r,wallColors[v].g,wallColors[v].b,255);
    SDL_RenderFillRect(renderer, &(SDL_Rect){x,y,w,h});
    int roofH = std::max(8,h/6);
    SDL_SetRenderDrawColor(renderer,roofColors[v].r,roofColors[v].g,roofColors[v].b,255);
    SDL_RenderFillRect(renderer, &(SDL_Rect){x,y,w,roofH});
    SDL_SetRenderDrawColor(renderer,0,0,0,40);
    for (int by=y+roofH+6;by<y+h;by+=10)
        SDL_RenderDrawLine(renderer,x+2,by,x+w-2,by);
    bool stagger=false;
    for (int by=y+roofH+6;by<y+h;by+=10){
        int startX=stagger?x+10:x+5;
        for(int bx=startX;bx<x+w;bx+=20)
            SDL_RenderDrawLine(renderer,bx,by,bx,by+10);
        stagger=!stagger;
    }
    int winW=14,winH=12;
    int wCols=std::max(1,(w-20)/24);
    int wRows=std::max(1,(h-roofH-20)/22);
    for(int wr=0;wr<wRows;wr++) for(int wc=0;wc<wCols;wc++){
        int wx=x+10+wc*24,wy=y+roofH+10+wr*22;
        if(wx+winW>x+w-5||wy+winH>y+h-5) continue;
        bool lit=((wr+wc+variant)%3!=0);
        SDL_Color wc2=lit?SDL_Color{100,160,200,180}:SDL_Color{30,50,70,200};
        SDL_SetRenderDrawColor(renderer,wc2.r,wc2.g,wc2.b,wc2.a);
        SDL_RenderFillRect(renderer,&(SDL_Rect){wx,wy,winW,winH});
        SDL_SetRenderDrawColor(renderer,40,60,80,180);
        SDL_RenderDrawRect(renderer,&(SDL_Rect){wx,wy,winW,winH});
    }
    SDL_SetRenderDrawColor(renderer,20,20,20,220);
    SDL_RenderDrawRect(renderer,&(SDL_Rect){x,y,w,h});
    int doorW=std::max(10,w/5),doorH=std::max(12,h/4);
    int doorX=x+w/2-doorW/2,doorY=y+h-doorH;
    SDL_SetRenderDrawColor(renderer,60,35,15,255);
    SDL_RenderFillRect(renderer,&(SDL_Rect){doorX,doorY,doorW,doorH});
    SDL_SetRenderDrawColor(renderer,200,160,50,255);
    SDL_RenderDrawPoint(renderer,doorX+doorW-4,doorY+doorH/2);
    SDL_RenderDrawPoint(renderer,doorX+doorW-3,doorY+doorH/2);
}

void Game::drawBulletSprite(int cx, int cy)
{
    drawFilledCircle(cx, cy, 5, {255,150,0,200});
    drawFilledCircle(cx, cy, 3, {255,240,80,255});
}

// =============================== Render (top-level) ===============================

void Game::render()
{
    if (gameState == GameState::MENU) { renderMenu(); return; }

    renderBackground();
    renderObstacles();
    renderZombies();
    renderBullets();
    renderHealthPickups();
    renderPlayer();
    renderHUD();
    renderPhaseTransition();

    if (gameState == GameState::PAUSED)
        renderPaused();
    else if (gameState == GameState::WIN || gameState == GameState::LOSE)
        renderOverlay();
    else
        SDL_RenderPresent(renderer);
}

// =============================== Background ===============================

void Game::renderBackground()
{
    SDL_SetRenderDrawColor(renderer, 35, 45, 30, 255);
    SDL_RenderClear(renderer);
    int tileSize = 64;
    for (int gy = 0; gy < screenHeight; gy += tileSize)
        for (int gx = 0; gx < screenWidth; gx += tileSize) {
            int sh = ((gx/tileSize + gy/tileSize)%2==0) ? 38 : 32;
            int gs = ((gx/tileSize + gy/tileSize)%2==0) ? 48 : 42;
            SDL_SetRenderDrawColor(renderer, sh, gs, sh-5, 255);
            SDL_RenderFillRect(renderer, &(SDL_Rect){gx, gy, tileSize, tileSize});
        }
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &(SDL_Rect){0, screenHeight/2-24, screenWidth, 48});
    SDL_RenderFillRect(renderer, &(SDL_Rect){screenWidth/2-24, 0, 48, screenHeight});
    SDL_SetRenderDrawColor(renderer, 220, 200, 60, 180);
    for (int rx = 0; rx < screenWidth;  rx += 40) SDL_RenderDrawLine(renderer,rx,screenHeight/2,rx+20,screenHeight/2);
    for (int ry = 0; ry < screenHeight; ry += 40) SDL_RenderDrawLine(renderer,screenWidth/2,ry,screenWidth/2,ry+20);
    SDL_SetRenderDrawColor(renderer, 80, 60, 40, 255);
    SDL_Rect borders[4] = {{0,0,screenWidth,6},{0,screenHeight-6,screenWidth,6},{0,0,6,screenHeight},{screenWidth-6,0,6,screenHeight}};
    for (auto& b : borders) SDL_RenderFillRect(renderer, &b);
}

void Game::renderObstacles()
{
    for (const auto& obs : obstacles)
        drawBuildingSprite((int)obs.x,(int)obs.y,(int)obs.width,(int)obs.height,obs.variant);
}

void Game::renderZombies()
{
    for (int i = 0; i < zombies.count; i++) {
        if (!zombies.isAlive[i]) continue;
        float dx = player.x - zombies.x[i];
        float dy = player.y - zombies.y[i];
        float angle = atan2f(dy, dx);
        float hf = (float)zombies.health[i] / 3.0f;
        if (hf > 1.0f) hf = 1.0f;
        int hbX=(int)zombies.x[i]-10, hbY=(int)zombies.y[i]-16;
        SDL_SetRenderDrawColor(renderer,60,0,0,200);
        SDL_RenderFillRect(renderer,&(SDL_Rect){hbX,hbY,20,3});
        SDL_SetRenderDrawColor(renderer,220,40,40,255);
        SDL_RenderFillRect(renderer,&(SDL_Rect){hbX,hbY,(int)(20*hf),3});
        drawZombieSprite((int)zombies.x[i],(int)zombies.y[i],angle);
    }
}

void Game::renderBullets()
{
    for (int i = 0; i < bullets.count; i++)
        if (bullets.isAlive[i])
            drawBulletSprite((int)bullets.x[i],(int)bullets.y[i]);
}

void Game::renderPlayer()
{
    SDL_SetRenderDrawColor(renderer, 255, 50, 50, 120);
    SDL_RenderDrawLine(renderer,(int)player.x,(int)player.y,
        (int)(player.x+cosf(player.aimAngle)*80.0f),
        (int)(player.y+sinf(player.aimAngle)*80.0f));
    drawPlayerSprite((int)player.x,(int)player.y,player.aimAngle,player.invincibleTimer>0.0f);
}

// =============================== HUD ===============================

void Game::renderHUD()
{
    // --- Health bar (top-left) ---
    int hbX=15,hbY=15,hbW=220,hbH=22;
    SDL_SetRenderDrawColor(renderer,20,20,20,200);
    SDL_RenderFillRect(renderer,&(SDL_Rect){hbX-2,hbY-2,hbW+4,hbH+4});
    int hpW=(int)(hbW*(player.health/100.0f)); if(hpW<0)hpW=0;
    SDL_Color hpCol = player.health>60 ? SDL_Color{30,200,50,255} : player.health>30 ? SDL_Color{220,180,0,255} : SDL_Color{220,30,30,255};
    SDL_SetRenderDrawColor(renderer,hpCol.r,hpCol.g,hpCol.b,255);
    SDL_RenderFillRect(renderer,&(SDL_Rect){hbX,hbY,hpW,hbH});
    SDL_SetRenderDrawColor(renderer,200,200,200,200);
    SDL_RenderDrawRect(renderer,&(SDL_Rect){hbX,hbY,hbW,hbH});
    // "HP" label
    drawText("HP", hbX+4, hbY+4, 2, {255,255,255,220});

    // --- Top-right panel ---
    int panelW=320, panelH=195, panelX=screenWidth-panelW-10, panelY=10;
    SDL_SetRenderDrawColor(renderer,0,0,0,190);
    SDL_RenderFillRect(renderer,&(SDL_Rect){panelX,panelY,panelW,panelH});
    SDL_SetRenderDrawColor(renderer,80,200,80,200);
    SDL_RenderDrawRect(renderer,&(SDL_Rect){panelX,panelY,panelW,panelH});

    // Mode color + label
    SDL_Color modeCol;
    const char* modeStr;
    switch(mode){
        case ProcessorMode::OPENMP:   modeStr="OPENMP  [2]"; modeCol={80,200,255,255}; break;
        case ProcessorMode::MPI_MODE: modeStr="MPI     [3]"; modeCol={255,180,50,255}; break;
        case ProcessorMode::GPU:      modeStr="CUDA GPU[4]"; modeCol={200,80,255,255}; break;
        default:                      modeStr="SEQ     [1]"; modeCol={80,255,80,255};  break;
    }
    SDL_SetRenderDrawColor(renderer,modeCol.r,modeCol.g,modeCol.b,255);
    SDL_RenderFillRect(renderer,&(SDL_Rect){panelX+4,panelY+4,panelW-8,20});
    drawText(modeStr, panelX+8, panelY+7, 2, {10,10,10,255});

    // Stat bars with labels
    auto drawStat=[&](int row, const char* label, float val, float maxVal, SDL_Color col){
        int ry=panelY+28+row*28;
        SDL_SetRenderDrawColor(renderer,30,30,30,200);
        SDL_RenderFillRect(renderer,&(SDL_Rect){panelX+4,ry,panelW-8,24});
        drawText(label, panelX+7, ry+5, 2, {180,180,180,255});
        int barX=panelX+52, barMaxW=panelW-60;
        int barW=(int)(barMaxW*(val/maxVal));
        if(barW>barMaxW)barW=barMaxW; if(barW<0)barW=0;
        SDL_SetRenderDrawColor(renderer,col.r,col.g,col.b,200);
        SDL_RenderFillRect(renderer,&(SDL_Rect){barX,ry+4,barW,16});
        SDL_SetRenderDrawColor(renderer,100,100,100,200);
        SDL_RenderDrawRect(renderer,&(SDL_Rect){barX,ry+4,barMaxW,16});
    };

    int alive=zombies.aliveCount();

    // Adaptive UPD scale: use at least 1ms, at most 10ms, so bars are always visible
    float updScale = updateTimeMs   < 1.0f ? 1.0f : (updateTimeMs   > 10.0f ? 10.0f : updateTimeMs   * 1.5f);
    float colScale = collisionTimeMs < 1.0f ? 1.0f : (collisionTimeMs > 10.0f ? 10.0f : collisionTimeMs * 1.5f);

    drawStat(0,"FPS",(float)fps,120.0f,{80,220,80,255});
    drawStat(1,"UPD",updateTimeMs,updScale,{80,180,255,255});
    drawStat(2,"COL",collisionTimeMs,colScale,{255,150,50,255});
    drawStat(3,"ZOM",(float)alive,(float)std::max(1,zombies.count),{220,50,50,255});
    drawStat(4,"KIL",(float)(totalKills%200),200.0f,{200,200,50,255});

    // ms numeric labels on UPD and COL bars
    char updBuf[24], colBuf[24], fpsBuf[16];
    snprintf(updBuf, sizeof(updBuf), "%.2fMS", updateTimeMs);
    snprintf(colBuf, sizeof(colBuf), "%.2fMS", collisionTimeMs);
    snprintf(fpsBuf, sizeof(fpsBuf), "%d", fps);
    int rowH = 28;
    drawText(updBuf, panelX + panelW - 52, panelY + 28 + 1*rowH + 6, 1, {180,220,255,255});
    drawText(colBuf, panelX + panelW - 52, panelY + 28 + 2*rowH + 6, 1, {255,200,130,255});
    drawText(fpsBuf, panelX + panelW - 28, panelY + 28 + 0*rowH + 6, 1, {150,255,150,255});

    // --- Phase indicator (top-center) ---
    const char* phaseNames[]={"EASY","MEDIUM","HARD"};
    SDL_Color phaseColors[]={{80,220,80,255},{255,180,50,255},{255,60,60,255}};
    int ph=currentPhase-1; if(ph<0)ph=0; if(ph>2)ph=2;
    char phaseBuf[64];
    snprintf(phaseBuf,sizeof(phaseBuf),"PHASE %d: %s",currentPhase,phaseNames[ph]);
    int pbW=textWidth(phaseBuf,2);
    SDL_SetRenderDrawColor(renderer,0,0,0,180);
    SDL_RenderFillRect(renderer,&(SDL_Rect){screenWidth/2-pbW/2-12,8,pbW+24,28});
    SDL_SetRenderDrawColor(renderer,phaseColors[ph].r,phaseColors[ph].g,phaseColors[ph].b,200);
    SDL_RenderDrawRect(renderer,&(SDL_Rect){screenWidth/2-pbW/2-12,8,pbW+24,28});
    drawText(phaseBuf,screenWidth/2-pbW/2,14,2,phaseColors[ph]);

    // --- HP number label ---
    char hpBuf[16]; snprintf(hpBuf,sizeof(hpBuf),"%d",player.health);
    drawText(hpBuf, hbX+hbW+6, hbY+4, 2, {220,220,220,255});

    // --- Kill counter (below health) ---
    char kBuf[32]; snprintf(kBuf,sizeof(kBuf),"KILLS: %d",totalKills);
    drawText(kBuf, hbX, hbY+30, 2, {200,200,50,255});

    // --- Timer (below kills) ---
    int mins=(int)(totalTime/60), secs=(int)(totalTime)%60;
    char tBuf[32]; snprintf(tBuf,sizeof(tBuf),"TIME: %02d:%02d",mins,secs);
    drawText(tBuf, hbX, hbY+52, 2, {150,200,255,255});

    // --- Bottom legend ---
    SDL_SetRenderDrawColor(renderer,0,0,0,160);
    SDL_RenderFillRect(renderer,&(SDL_Rect){10,screenHeight-52,430,46});
    SDL_SetRenderDrawColor(renderer,60,60,60,200);
    SDL_RenderDrawRect(renderer,&(SDL_Rect){10,screenHeight-52,430,46});
    const char* hints="[P]PAUSE  [R]RESTART  [H]HORDE  [1-4]MODE  [ESC]PAUSE";
    drawText(hints,16,screenHeight-44,1,{160,160,160,220});
    const char* modeKeys="[1]SEQ  [2]OMP  [3]MPI  [4]GPU";
    drawText(modeKeys,16,screenHeight-30,1,{120,200,120,220});

    SDL_RenderPresent(renderer);
}

// =============================== Phase Transition Banner ===============================

void Game::renderPhaseTransition()
{
    if (!phaseTransitionActive) return;

    float alpha=1.0f;
    if (phaseTransitionTimer < 0.5f)  alpha = phaseTransitionTimer/0.5f;
    if (phaseTransitionTimer > 2.5f)  alpha = (3.0f-phaseTransitionTimer)/0.5f;
    if (alpha < 0) alpha = 0;
    Uint8 a = (Uint8)(255*alpha);

    SDL_SetRenderDrawColor(renderer,0,0,0,(Uint8)(180*alpha));
    SDL_Rect banner={screenWidth/2-380,screenHeight/2-90,760,180};
    SDL_RenderFillRect(renderer,&banner);

    const char* phaseNames[]={"EASY","MEDIUM","HARD"};
    SDL_Color phaseColors[]={{80,255,80,255},{255,180,50,255},{255,50,50,255}};
    int ph=currentPhase-1; if(ph<0)ph=0; if(ph>2)ph=2;

    char line1[32]; snprintf(line1,sizeof(line1),"PHASE %d",currentPhase);
    SDL_Color c1=phaseColors[ph]; c1.a=a;
    drawTextCentered(line1, screenHeight/2-80, 5, c1);

    SDL_Color c2={255,255,255,a};
    drawTextCentered(phaseNames[ph], screenHeight/2-20, 4, c2);

    char line3[64]; snprintf(line3,sizeof(line3),"%d ZOMBIES INCOMING!",phaseZombieCount[ph]);
    SDL_Color c3={220,220,100,(Uint8)(a*0.85f)};
    drawTextCentered(line3, screenHeight/2+45, 2, c3);
}

// =============================== Pause Screen ===============================

void Game::renderPaused()
{
    // Dark overlay over game
    SDL_SetRenderDrawColor(renderer,0,0,0,160);
    SDL_Rect fs={0,0,screenWidth,screenHeight};
    SDL_RenderFillRect(renderer,&fs);

    int pw=540, ph=300, px=screenWidth/2-pw/2, py=screenHeight/2-ph/2;
    SDL_SetRenderDrawColor(renderer,12,18,12,245);
    SDL_RenderFillRect(renderer,&(SDL_Rect){px,py,pw,ph});
    SDL_SetRenderDrawColor(renderer,100,220,100,255);
    SDL_RenderDrawRect(renderer,&(SDL_Rect){px,py,pw,ph});
    SDL_SetRenderDrawColor(renderer,60,140,60,180);
    SDL_RenderDrawRect(renderer,&(SDL_Rect){px+3,py+3,pw-6,ph-6});

    // "PAUSED"
    drawTextCentered("PAUSED", py+22, 5, {255,220,50,255});

    // Stats
    char buf[128];
    const char* phaseNames[]={"EASY","MEDIUM","HARD"};
    int ph2=currentPhase-1; if(ph2<0)ph2=0; if(ph2>2)ph2=2;
    SDL_Color white={220,220,220,255};
    int ly=py+105, lineH=32;

    snprintf(buf,sizeof(buf),"PHASE: %d of %d (%s)",currentPhase,PHASE_COUNT,phaseNames[ph2]);
    drawTextCentered(buf,ly,2,white); ly+=lineH;

    snprintf(buf,sizeof(buf),"KILLS: %d",totalKills);
    drawTextCentered(buf,ly,2,white); ly+=lineH;

    int mins=(int)(totalTime/60), secs=(int)(totalTime)%60;
    snprintf(buf,sizeof(buf),"TIME: %02d:%02d",mins,secs);
    drawTextCentered(buf,ly,2,white); ly+=lineH+6;

    drawTextCentered("[P] RESUME",   ly,2,{80,255,80,255});
    int resumeW=textWidth("[P] RESUME",2);
    drawText("[R] RESTART", screenWidth/2+resumeW/2+20, ly, 2, {255,150,50,255});

    SDL_RenderPresent(renderer);
}

// =============================== WIN / LOSE Overlay ===============================

void Game::renderOverlay()
{
    SDL_Rect fs={0,0,screenWidth,screenHeight};

    if (gameState == GameState::LOSE) {
        SDL_SetRenderDrawColor(renderer,0,0,0,160); SDL_RenderFillRect(renderer,&fs);
        SDL_SetRenderDrawColor(renderer,180,0,0,70); SDL_RenderFillRect(renderer,&fs);

        int pw=580,ph=340,px=screenWidth/2-pw/2,py=screenHeight/2-ph/2;
        SDL_SetRenderDrawColor(renderer,20,5,5,245);
        SDL_RenderFillRect(renderer,&(SDL_Rect){px,py,pw,ph});
        SDL_SetRenderDrawColor(renderer,220,30,30,255);
        SDL_RenderDrawRect(renderer,&(SDL_Rect){px,py,pw,ph});
        SDL_SetRenderDrawColor(renderer,140,10,10,180);
        SDL_RenderDrawRect(renderer,&(SDL_Rect){px+3,py+3,pw-6,ph-6});

        drawTextCentered("GAME OVER", py+22, 5, {255,50,50,255});

        char buf[128];
        SDL_Color white={220,220,220,255};
        const char* phaseNames[]={"EASY","MEDIUM","HARD"};
        int ph2=currentPhase-1; if(ph2<0)ph2=0; if(ph2>2)ph2=2;
        int ly=py+120, lineH=34;

        snprintf(buf,sizeof(buf),"FELL ON PHASE: %d (%s)",currentPhase,phaseNames[ph2]);
        drawTextCentered(buf,ly,2,white); ly+=lineH;

        snprintf(buf,sizeof(buf),"TOTAL KILLS: %d",totalKills);
        drawTextCentered(buf,ly,2,white); ly+=lineH;

        int mins=(int)(totalTime/60),secs=(int)(totalTime)%60;
        snprintf(buf,sizeof(buf),"SURVIVED: %02d:%02d",mins,secs);
        drawTextCentered(buf,ly,2,white); ly+=lineH+8;

        drawTextCentered("PRESS R TO RESTART", ly, 2, {255,220,50,255});
    }

    if (gameState == GameState::WIN) {
        SDL_SetRenderDrawColor(renderer,0,0,0,140); SDL_RenderFillRect(renderer,&fs);
        SDL_SetRenderDrawColor(renderer,0,150,0,60); SDL_RenderFillRect(renderer,&fs);

        int pw=620,ph=400,px=screenWidth/2-pw/2,py=screenHeight/2-ph/2;
        SDL_SetRenderDrawColor(renderer,5,20,5,245);
        SDL_RenderFillRect(renderer,&(SDL_Rect){px,py,pw,ph});
        SDL_SetRenderDrawColor(renderer,50,220,50,255);
        SDL_RenderDrawRect(renderer,&(SDL_Rect){px,py,pw,ph});
        SDL_SetRenderDrawColor(renderer,200,180,50,200);
        SDL_RenderDrawRect(renderer,&(SDL_Rect){px+3,py+3,pw-6,ph-6});

        drawTextCentered("YOU WIN!", py+22, 5, {255,215,0,255});

        drawTextCentered("ALL 3 PHASES CLEARED!", py+108, 2, {80,255,80,200});

        char buf[128];
        SDL_Color white={220,220,220,255};
        int ly=py+148, lineH=38;

        snprintf(buf,sizeof(buf),"TOTAL KILLS: %d",totalKills);
        drawTextCentered(buf,ly,2,white); ly+=lineH;

        int mins=(int)(totalTime/60),secs=(int)(totalTime)%60;
        snprintf(buf,sizeof(buf),"COMPLETION TIME: %02d:%02d",mins,secs);
        drawTextCentered(buf,ly,2,white); ly+=lineH;

        const char* modeNames[]={"SEQUENTIAL","OPENMP","MPI","CUDA GPU"};
        int mIdx=(int)mode; if(mIdx<0||mIdx>3)mIdx=0;
        snprintf(buf,sizeof(buf),"MODE USED: %s",modeNames[mIdx]);
        drawTextCentered(buf,ly,2,white); ly+=lineH+8;

        drawTextCentered("PRESS R TO PLAY AGAIN", ly, 2, {255,220,50,255});
    }

    SDL_RenderPresent(renderer);
}

// =============================== Menu Screen ===============================

void Game::renderMenu()
{
    // Background grid
    SDL_SetRenderDrawColor(renderer,10,15,10,255);
    SDL_RenderClear(renderer);
    int tileSize=64;
    for (int gy=0;gy<screenHeight;gy+=tileSize)
        for (int gx=0;gx<screenWidth;gx+=tileSize){
            int sh=((gx/tileSize+gy/tileSize)%2==0)?22:18;
            SDL_SetRenderDrawColor(renderer,sh,sh+4,sh,255);
            SDL_RenderFillRect(renderer,&(SDL_Rect){gx,gy,tileSize,tileSize});
        }

    // Zombie sprites in background
    for (int z=0;z<8;z++) {
        int zx=80+z*160, zy=screenHeight-90+(z%3)*18;
        drawZombieSprite(zx,zy,3.14159f);
    }

    // --- Title box ---
    int tw2=680, th=92, tx=screenWidth/2-tw2/2, ty=65;
    SDL_SetRenderDrawColor(renderer,0,0,0,120);
    SDL_RenderFillRect(renderer,&(SDL_Rect){tx+6,ty+6,tw2,th});
    SDL_SetRenderDrawColor(renderer,155,12,12,235);
    SDL_RenderFillRect(renderer,&(SDL_Rect){tx,ty,tw2,th/2});
    SDL_SetRenderDrawColor(renderer,110,8,8,235);
    SDL_RenderFillRect(renderer,&(SDL_Rect){tx,ty+th/2,tw2,th-th/2});
    SDL_SetRenderDrawColor(renderer,255,80,80,255);
    SDL_RenderDrawRect(renderer,&(SDL_Rect){tx,ty,tw2,th});
    SDL_SetRenderDrawColor(renderer,200,40,40,160);
    SDL_RenderDrawRect(renderer,&(SDL_Rect){tx+3,ty+3,tw2-6,th-6});

    // Title text "ZOMBIE SURVIVAL"
    int tScale=4;
    int tTW=textWidth("ZOMBIE SURVIVAL",tScale);
    drawText("ZOMBIE SURVIVAL", screenWidth/2-tTW/2, ty+(th-7*tScale)/2, tScale, {255,255,255,255});

    // Subtitle
    int subY=ty+th+10;
    SDL_SetRenderDrawColor(renderer,18,55,18,210);
    int subTW=textWidth("PDC PARALLEL COMPUTING PROJECT",2);
    SDL_RenderFillRect(renderer,&(SDL_Rect){screenWidth/2-subTW/2-16,subY,subTW+32,28});
    SDL_SetRenderDrawColor(renderer,80,200,80,200);
    SDL_RenderDrawRect(renderer,&(SDL_Rect){screenWidth/2-subTW/2-16,subY,subTW+32,28});
    drawText("PDC PARALLEL COMPUTING PROJECT", screenWidth/2-subTW/2, subY+7, 2, {80,220,80,255});

    // --- Controls panel ---
    int ctrlY=subY+45, ctrlH=220;
    int ctrlX=screenWidth/2-340;
    SDL_SetRenderDrawColor(renderer,0,0,0,190);
    SDL_RenderFillRect(renderer,&(SDL_Rect){ctrlX,ctrlY,680,ctrlH});
    SDL_SetRenderDrawColor(renderer,100,100,100,200);
    SDL_RenderDrawRect(renderer,&(SDL_Rect){ctrlX,ctrlY,680,ctrlH});

    // Controls title
    drawTextCentered("CONTROLS", ctrlY+8, 2, {200,200,200,220});
    SDL_SetRenderDrawColor(renderer,60,60,60,200);
    SDL_RenderDrawLine(renderer,ctrlX+10,ctrlY+24,ctrlX+670,ctrlY+24);

    struct CEntry { const char* key; const char* desc; SDL_Color col; };
    CEntry entries[]={
        {"WASD",      "MOVE PLAYER",          {80,255,80,255}},
        {"MOUSE",     "AIM DIRECTION",         {80,200,255,255}},
        {"L-CLICK",   "SHOOT",                 {255,200,50,255}},
        {"H",         "HORDE MODE (500 ZOMBIES)", {255,80,80,255}},
        {"1/2/3/4",   "SWITCH COMPUTE MODE",   {200,100,255,255}},
        {"P / ESC",   "PAUSE / RESUME",        {150,200,150,255}},
        {"R",         "RESTART GAME",          {150,150,150,255}},
    };
    int numEntries=7, colW=340;
    for (int i=0;i<numEntries;i++){
        int col2=i%2, row=i/2;
        int ex=ctrlX+16+col2*colW, ey=ctrlY+32+row*40;
        SDL_SetRenderDrawColor(renderer,entries[i].col.r,entries[i].col.g,entries[i].col.b,50);
        SDL_RenderFillRect(renderer,&(SDL_Rect){ex,ey,colW-20,28});
        SDL_SetRenderDrawColor(renderer,entries[i].col.r,entries[i].col.g,entries[i].col.b,180);
        SDL_RenderDrawRect(renderer,&(SDL_Rect){ex,ey,colW-20,28});
        drawText(entries[i].key, ex+6, ey+6, 2, entries[i].col);
        int kw=textWidth(entries[i].key,2);
        drawText("-", ex+kw+8, ey+6, 2, {120,120,120,200});
        drawText(entries[i].desc, ex+kw+20, ey+6, 2, {200,200,200,220});
    }

    // --- Mode buttons ---
    int modeY=ctrlY+ctrlH+14;
    struct ModeBtn { const char* label; SDL_Color col; };
    ModeBtn modes[]={
        {"[1] SEQUENTIAL", {80,255,80,255}},
        {"[2] OPENMP",     {80,200,255,255}},
        {"[3] MPI",        {255,180,50,255}},
        {"[4] CUDA GPU",   {200,80,255,255}},
    };
    int btnW=156, btnH=30, btnGap=8;
    int totalW=4*btnW+3*btnGap;
    int startX=screenWidth/2-totalW/2;
    for (int i=0;i<4;i++){
        int bx=startX+i*(btnW+btnGap);
        SDL_Color bc=modes[i].col;
        SDL_SetRenderDrawColor(renderer,bc.r/5,bc.g/5,bc.b/5,220);
        SDL_RenderFillRect(renderer,&(SDL_Rect){bx,modeY,btnW,btnH});
        SDL_SetRenderDrawColor(renderer,bc.r,bc.g,bc.b,220);
        SDL_RenderDrawRect(renderer,&(SDL_Rect){bx,modeY,btnW,btnH});
        int lw=textWidth(modes[i].label,2);
        drawText(modes[i].label, bx+btnW/2-lw/2, modeY+8, 2, bc);
    }

    // --- START button ---
    int startBtnY=modeY+42;
    int sbW=320, sbH=52, sbX=screenWidth/2-sbW/2;
    SDL_SetRenderDrawColor(renderer,0,0,0,120);
    SDL_RenderFillRect(renderer,&(SDL_Rect){sbX+5,startBtnY+5,sbW,sbH});
    SDL_SetRenderDrawColor(renderer,0,160,0,230);
    SDL_RenderFillRect(renderer,&(SDL_Rect){sbX,startBtnY,sbW,sbH});
    SDL_SetRenderDrawColor(renderer,80,255,80,255);
    SDL_RenderDrawRect(renderer,&(SDL_Rect){sbX,startBtnY,sbW,sbH});
    SDL_SetRenderDrawColor(renderer,50,200,50,180);
    SDL_RenderDrawRect(renderer,&(SDL_Rect){sbX+3,startBtnY+3,sbW-6,sbH-6});
    int stw=textWidth("CLICK TO START",3);
    drawText("CLICK TO START", screenWidth/2-stw/2, startBtnY+(sbH-7*3)/2, 3, {255,255,255,255});

    // --- Phase preview ---
    int previewY=startBtnY+sbH+14;
    const char* phases[]={"PHASE 1: EASY (30)","PHASE 2: MEDIUM (70)","PHASE 3: HARD (150)"};
    SDL_Color pCols[]={{80,220,80,200},{255,180,50,200},{255,60,60,200}};
    int phW=190;
    int phStart=screenWidth/2-(3*phW+2*8)/2;
    for(int i=0;i<3;i++){
        int px2=phStart+i*(phW+8);
        SDL_SetRenderDrawColor(renderer,pCols[i].r,pCols[i].g,pCols[i].b,30);
        SDL_RenderFillRect(renderer,&(SDL_Rect){px2,previewY,phW,26});
        SDL_SetRenderDrawColor(renderer,pCols[i].r,pCols[i].g,pCols[i].b,180);
        SDL_RenderDrawRect(renderer,&(SDL_Rect){px2,previewY,phW,26});
        int pw2=textWidth(phases[i],1);
        drawText(phases[i], px2+phW/2-pw2/2, previewY+7, 1, pCols[i]);
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

    player.init(screenWidth/2.0f, screenHeight/2.0f);
    totalKills   = 0;
    totalTime    = 0.0f;
    currentPhase = 1;
    wave         = 1;
    fps          = 0;
    frameCount   = 0;
    fpsTimer     = 0.0f;
    updateTimeMs = 0.0f;
    collisionTimeMs = 0.0f;
    phaseTransitionActive = false;
    phaseTransitionTimer  = 0.0f;
    mpiStateSynced = false;

    zombies.spawnWave(phaseZombieCount[0], screenWidth, screenHeight, obstacles);
    initHealthPickups();
    gameState = GameState::PLAYING;
}

// =============================== Cleanup ===============================

void Game::cleanup()
{
#ifdef USE_MPI
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        int cmd = 0;
        MPI_Bcast(&cmd, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }
#endif
    zombies.deallocate();
    bullets.deallocate();
    if (renderer) { SDL_DestroyRenderer(renderer); renderer=nullptr; }
    if (window)   { SDL_DestroyWindow(window);     window=nullptr; }
    SDL_Quit();
}

// =============================== Health Pickups ===============================

void Game::spawnHealthPickupAt(int i)
{
    const int margin = 80;
    float cx = screenWidth  / 2.0f;
    float cy = screenHeight / 2.0f;

    for (int tries = 0; tries < 150; tries++) {
        float sx = margin + (float)(rand() % (screenWidth  - 2*margin));
        float sy = margin + (float)(rand() % (screenHeight - 2*margin));

        // Not too close to player spawn (center)
        if (fabsf(sx - cx) < 120.0f && fabsf(sy - cy) < 120.0f) continue;

        // Not inside any obstacle
        bool blocked = false;
        for (const auto& obs : obstacles) {
            if (sx > obs.x - 24 && sx < obs.x + obs.width  + 24 &&
                sy > obs.y - 24 && sy < obs.y + obs.height + 24) {
                blocked = true; break;
            }
        }
        if (blocked) continue;

        // Not too close to another alive pickup (min 150px apart)
        bool tooClose = false;
        for (int j = 0; j < MAX_HEALTH_PICKUPS; j++) {
            if (j == i || !healthPickups[j].alive) continue;
            float dx = healthPickups[j].x - sx;
            float dy = healthPickups[j].y - sy;
            if (dx*dx + dy*dy < 150.0f*150.0f) { tooClose = true; break; }
        }
        if (tooClose) continue;

        healthPickups[i] = { sx, sy, true, 0.0f };
        return;
    }
    // Fallback positions if random placement fails
    float fallbacks[3][2] = {{180.0f,180.0f},{(float)(screenWidth-180),(float)(screenHeight-180)},{180.0f,(float)(screenHeight-180)}};
    healthPickups[i] = { fallbacks[i%3][0], fallbacks[i%3][1], true, 0.0f };
}

void Game::initHealthPickups()
{
    for (int i = 0; i < MAX_HEALTH_PICKUPS; i++) {
        healthPickups[i] = { 0, 0, false, 0.0f };
    }
    // Spawn them staggered so they don't all appear at once
    for (int i = 0; i < MAX_HEALTH_PICKUPS; i++) {
        spawnHealthPickupAt(i);
    }
}

void Game::updateHealthPickups(float dt)
{
    const float PICKUP_RADIUS_SQ = 22.0f * 22.0f;

    for (int i = 0; i < MAX_HEALTH_PICKUPS; i++) {
        if (!healthPickups[i].alive) {
            healthPickups[i].respawnTimer -= dt;
            if (healthPickups[i].respawnTimer <= 0.0f)
                spawnHealthPickupAt(i);
            continue;
        }

        // Check if player walks over it
        float dx = player.x - healthPickups[i].x;
        float dy = player.y - healthPickups[i].y;
        if (dx*dx + dy*dy < PICKUP_RADIUS_SQ) {
            // Only heal if player is not at full health
            if (player.health < 100) {
                player.health += HEALTH_RESTORE;
                if (player.health > 100) player.health = 100;
            }
            // Always collect (even if full, it disappears)
            healthPickups[i].alive = false;
            healthPickups[i].respawnTimer = (float)PICKUP_RESPAWN_SEC;
        }
    }
}

void Game::renderHealthPickups()
{
    float t = SDL_GetTicks() / 1000.0f;
    float pulse = (sinf(t * 4.0f) + 1.0f) * 0.5f;  // 0..1 oscillating

    for (int i = 0; i < MAX_HEALTH_PICKUPS; i++) {
        if (!healthPickups[i].alive) {
            // Draw a faint "incoming" indicator showing when it respawns
            if (healthPickups[i].respawnTimer > 0.0f) continue;
            continue;
        }

        int cx = (int)healthPickups[i].x;
        int cy = (int)healthPickups[i].y;

        // --- Outer glow ring ---
        int glowR = 18 + (int)(pulse * 8.0f);
        Uint8 glowA = (Uint8)(50 + pulse * 80);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 40, 255, 100, glowA);
        for (int dy2 = -glowR; dy2 <= glowR; dy2++) {
            int dx2 = (int)sqrtf((float)(glowR*glowR - dy2*dy2));
            SDL_RenderDrawLine(renderer, cx-dx2, cy+dy2, cx+dx2, cy+dy2);
        }

        // --- Dark background circle ---
        drawFilledCircle(cx, cy, 14, {10, 35, 15, 230});

        // --- Bright border ring ---
        SDL_SetRenderDrawColor(renderer, 50, 220, 90, 255);
        for (int a = 0; a < 360; a += 8) {
            float ra = a * 3.14159f / 180.0f;
            SDL_RenderDrawPoint(renderer, cx+(int)(cosf(ra)*14.5f), cy+(int)(sinf(ra)*14.5f));
        }

        // --- Green cross (horizontal bar) ---
        SDL_SetRenderDrawColor(renderer, 60, 240, 90, 255);
        SDL_Rect hBar = {cx-10, cy-3, 20, 6};
        SDL_RenderFillRect(renderer, &hBar);

        // --- Green cross (vertical bar) ---
        SDL_Rect vBar = {cx-3, cy-10, 6, 20};
        SDL_RenderFillRect(renderer, &vBar);

        // --- Bright center pip ---
        drawFilledCircle(cx, cy, 3, {200, 255, 210, 255});

        // --- Floating "+25" label ---
        float labelBob = sinf(t * 2.5f) * 5.0f;
        drawText("+25 HP", cx - 18, cy - 26 + (int)labelBob, 1, {150, 255, 160, (Uint8)(180 + pulse*70)});
    }
}
