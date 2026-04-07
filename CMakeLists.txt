cmake_minimum_required(VERSION 3.20)
project(ZombieSurvivalGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# Options — Enable parallel modules
# ============================================================
option(ENABLE_OPENMP "Enable OpenMP parallelization" ON)
option(ENABLE_MPI    "Enable MPI distributed processing" OFF)
option(ENABLE_CUDA   "Enable CUDA GPU acceleration" OFF)

# ============================================================
# SDL2 — Found via vcpkg toolchain file
# ============================================================
find_package(SDL2 CONFIG REQUIRED)

# ============================================================
# Source files (always built)
# ============================================================
set(SOURCES
    src/main.cpp
    src/Game.cpp
    src/Entities.cpp
    src/sequential/Update.cpp
)

# ============================================================
# OpenMP
# ============================================================
if(ENABLE_OPENMP)
    find_package(OpenMP)
    if(OpenMP_CXX_FOUND)
        list(APPEND SOURCES src/openmp/OMP_Update.cpp)
        message(STATUS ">> OpenMP ENABLED (${OpenMP_CXX_VERSION})")
    else()
        message(WARNING ">> OpenMP requested but NOT FOUND — building without it")
    endif()
endif()

# ============================================================
# MPI
# ============================================================
if(ENABLE_MPI)
    find_package(MPI REQUIRED)
    list(APPEND SOURCES src/mpi/MPI_Update.cpp)
    message(STATUS ">> MPI ENABLED")
endif()

# ============================================================
# CUDA
# ============================================================
if(ENABLE_CUDA)
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    list(APPEND SOURCES src/gpu/CUDA_Collisions.cu)
    message(STATUS ">> CUDA ENABLED")
endif()

# ============================================================
# Executable
# ============================================================
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE include)

# Use SDL2main for Windows subsystem entry point
target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL2::SDL2main
    SDL2::SDL2
)

# ============================================================
# Conditional compile definitions & linking
# ============================================================
if(ENABLE_OPENMP AND OpenMP_CXX_FOUND)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_OPENMP)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)
endif()

if(ENABLE_MPI AND MPI_CXX_FOUND)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_MPI)
    target_link_libraries(${PROJECT_NAME} PRIVATE MPI::MPI_CXX)
endif()

if(ENABLE_CUDA)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_CUDA)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON
    )
endif()

# ============================================================
# Output directory — keep .exe next to assets/
# ============================================================
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

# ============================================================
# Post-build: copy assets folder into bin/ so the exe finds them
# ============================================================
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
    COMMENT "Copying assets to bin/"
)

# Also copy SDL2 DLL on Windows (vcpkg puts it beside the lib)
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL2::SDL2>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying SDL2.dll to bin/"
    )
endif()
