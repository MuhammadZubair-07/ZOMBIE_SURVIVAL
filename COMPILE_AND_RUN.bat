@echo off
echo ============================================================
echo  ZOMBIE SURVIVAL - Build and Run
echo ============================================================
echo.
echo Building with MSYS2/ucrt64 g++...
echo.

C:\msys64\usr\bin\bash.exe --login /e/PDC ZOMBIE GAME/build.sh

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo BUILD FAILED - see errors above
    pause
    exit /b 1
)

echo.
echo Starting game...
cd /d "e:\PDC ZOMBIE GAME\bin"
ZombieSurvivalGame.exe
