@echo off
cd /d "%~dp0bin"
echo ============================================================
echo  ZOMBIE SURVIVAL GAME - TRUE MPI MODE (4 Processes)
echo ============================================================
echo.
echo  Controls:
echo   WASD       - Move player
echo   Mouse      - Aim
echo   Left Click - Shoot
echo   H          - Spawn Horde
echo   3          - Switch to MPI mode to see the speedup!
echo.
echo Starting game via MPI (mpiexec -n 4)...
echo.
mpiexec -n 4 ZombieSurvivalGame.exe
pause
