cmake_minimum_required(VERSION 3.20)
project(ZombieSurvivalGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# Options — Enable parallel modules as the project progresses
# ============================================================
option(ENABLE_OPENMP "Enable OpenMP parallelization (Week 3)" ON)
option(ENABLE_MPI    "Enable MPI distributed processing (Week 3)" OFF)
option(ENABLE_CUDA   "Enable CUDA GPU acceleration (Week 4)"  OFF)

# ============================================================
# SDL2 — Found automatically when using vcpkg toolchain
# ============================================================
find_package(SDL2 CONFIG REQUIRED)

# ============================================================
# Source files
# ============================================================
set(SOURCES
    src/main.cpp
    src/Game.cpp
    src/Entities.cpp
    src/sequential/Update.cpp
)

if(ENABLE_OPENMP)
    find_package(OpenMP)
    if(OpenMP_CXX_FOUND)
        list(APPEND SOURCES src/openmp/OMP_Update.cpp)
    endif()
endif()

if(ENABLE_MPI)
    find_package(MPI REQUIRED)
    list(APPEND SOURCES src/mpi/MPI_Update.cpp)
endif()

if(ENABLE_CUDA)
    enable_language(CUDA)
    list(APPEND SOURCES src/gpu/CUDA_Collisions.cu)
endif()

# ============================================================
# Executable
# ============================================================
add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2 SDL2::SDL2main)

# ============================================================
# Conditional linking & compile definitions
# ============================================================
if(ENABLE_OPENMP AND OpenMP_CXX_FOUND)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_OPENMP)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)
    message(STATUS ">> OpenMP ENABLED")
endif()

if(ENABLE_MPI AND MPI_CXX_FOUND)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_MPI)
    target_link_libraries(${PROJECT_NAME} PRIVATE MPI::MPI_CXX)
    message(STATUS ">> MPI ENABLED")
endif()

if(ENABLE_CUDA)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_CUDA)
    message(STATUS ">> CUDA ENABLED")
endif()

# ============================================================
# Output directory — keep .exe next to assets/
# ============================================================
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
