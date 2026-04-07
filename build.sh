#!/bin/bash
export PATH=/c/msys64/ucrt64/bin:$PATH

ROOT="/e/PDC ZOMBIE GAME"
SRC="$ROOT/src"
INC="$ROOT/include"
BIN="$ROOT/bin"
OBJ="$ROOT/bin/obj"
SDL="/c/msys64/ucrt64/include/SDL2"
SDLLIB="/c/msys64/ucrt64/lib"

mkdir -p "$OBJ"

FLAGS="-std=c++17 -O2 -DUSE_OPENMP -fopenmp -fpermissive -DSDL_MAIN_HANDLED -DUSE_MPI"
INCS="-I\"$INC\" -I\"$SDL\""

echo "[1/6] Compiling main.cpp..."
g++ $FLAGS -I"$INC" -I"$SDL" -c "$SRC/main.cpp" -o "$OBJ/main.o"
[ $? -ne 0 ] && echo "FAILED: main.cpp" && exit 1
echo "   OK"

echo "[2/6] Compiling Entities.cpp..."
g++ $FLAGS -I"$INC" -I"$SDL" -c "$SRC/Entities.cpp" -o "$OBJ/Entities.o"
[ $? -ne 0 ] && echo "FAILED: Entities.cpp" && exit 1
echo "   OK"

echo "[3/6] Compiling Game.cpp..."
g++ $FLAGS -I"$INC" -I"$SDL" -c "$SRC/Game.cpp" -o "$OBJ/Game.o"
[ $? -ne 0 ] && echo "FAILED: Game.cpp" && exit 1
echo "   OK"

echo "[4/6] Compiling sequential/Update.cpp..."
g++ $FLAGS -I"$INC" -I"$SDL" -c "$SRC/sequential/Update.cpp" -o "$OBJ/Update.o"
[ $? -ne 0 ] && echo "FAILED: Update.cpp" && exit 1
echo "   OK"

echo "[5/6] Compiling openmp/OMP_Update.cpp..."
g++ $FLAGS -I"$INC" -I"$SDL" -c "$SRC/openmp/OMP_Update.cpp" -o "$OBJ/OMP_Update.o"
[ $? -ne 0 ] && echo "FAILED: OMP_Update.cpp" && exit 1
echo "   OK"

echo "[6/7] Compiling mpi/MPI_Update.cpp..."
g++ $FLAGS -I"$INC" -I"$SDL" -I"/c/Program Files (x86)/Microsoft SDKs/MPI/Include" -c "$SRC/mpi/MPI_Update.cpp" -o "$OBJ/MPI_Update.o"
[ $? -ne 0 ] && echo "FAILED: MPI_Update.cpp" && exit 1
echo "   OK"

echo "[7/7] Linking..."
g++ $FLAGS \
    "$OBJ/main.o" "$OBJ/Entities.o" "$OBJ/Game.o" \
    "$OBJ/Update.o" "$OBJ/OMP_Update.o" "$OBJ/MPI_Update.o" \
    -L"$SDLLIB" -lSDL2 -lgomp -lmingwthrd \
    "/c/Program Files (x86)/Microsoft SDKs/MPI/Lib/x64/msmpi.lib" \
    -Wl,--subsystem,console \
    -o "$BIN/ZombieSurvivalGame.exe"
[ $? -ne 0 ] && echo "FAILED: Linking" && exit 1
echo "   OK"

cp /c/msys64/ucrt64/bin/SDL2.dll "$BIN/" 2>/dev/null || true

echo ""
echo "===== BUILD SUCCESS ====="
