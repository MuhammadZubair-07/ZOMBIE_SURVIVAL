// ============================================================
// main.cpp — Entry point for the Parallel Zombie Survival Game
// PDC Semester Project — C++ | SDL2 | OpenMP | MPI | CUDA
// ============================================================

#include "Game.h"
#include <cstdio>
#include <ctime>

#ifdef USE_MPI
#include <mpi.h>
#endif

int main(int argc, char* argv[])
{
#ifdef USE_MPI
    MPI_Init(&argc, &argv);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#else
    int rank = 0;
#endif

    // Seed random number generator
    srand(static_cast<unsigned int>(time(nullptr)));

    Game game;

    if (rank == 0) {
        if (!game.init("Zombie Survival | PDC Project | Keys: WASD=Move Mouse=Aim LClick=Shoot H=Horde 1-4=Mode R=Restart", 1280, 720)) {
            printf("[FATAL] Failed to initialise the game.\n");
            return -1;
        }
        game.run();
        game.cleanup();
    } else {
#ifdef USE_MPI
        game.initWorker();
        game.runWorker();
#endif
    }

#ifdef USE_MPI
    MPI_Finalize();
#endif

    return 0;
}
