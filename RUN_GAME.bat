@echo off
cd /d "%~dp0bin"
echo ============================================================
echo  ZOMBIE SURVIVAL GAME - PDC Semester Project
echo ============================================================
echo.
echo  Controls:
echo   WASD       - Move player
echo   Mouse      - Aim
echo   Left Click - Shoot
echo   H          - Spawn Horde (5000 zombies!)
echo   1          - Sequential mode
echo   2          - OpenMP mode
echo   3          - MPI mode (requires mpiexec)
echo   4          - GPU/CUDA mode (requires CUDA GPU)
echo   R          - Restart
echo   ESC        - Quit
echo.
echo  Starting game...
echo.
ZombieSurvivalGame.exe
pause
