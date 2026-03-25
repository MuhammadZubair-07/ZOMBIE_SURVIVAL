// ============================================================
// main.cpp — Entry point for the Parallel Zombie Survival Game
// PDC Semester Project
// ============================================================

#include "Game.h"
#include <cstdio>
#include <ctime>

int main(int argc, char* argv[])
{
    // Seed random number generator
    srand(static_cast<unsigned int>(time(nullptr)));

    Game game;

    if (!game.init("Zombie Survival — PDC Project", 800, 600)) {
        printf("[FATAL] Failed to initialise the game.\n");
        return -1;
    }

    game.run();
    game.cleanup();

    return 0;
}
