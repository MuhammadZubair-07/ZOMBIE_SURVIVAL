// ============================================================
// gpu/CUDA_Collisions.cu — GPU-accelerated collision detection
// WEEK 4 IMPLEMENTATION (stub — fill in during Week 4)
// ============================================================

#ifdef USE_CUDA

#include <cuda_runtime.h>
#include <cstdio>
#include <cmath>

// Forward declarations of host-side wrappers
// These will be called from Processors.h via checkCollisionsGPU()

// ===================== CUDA Kernel =====================
// Each thread checks ONE (bullet, zombie) pair.
// Grid:   bulletCount  blocks
// Block:  min(zombieCount, 256)  threads
//
// Algorithm:
//   tid = blockIdx.x * blockDim.x + threadIdx.x
//   bullet_index = blockIdx.x
//   zombie_index = threadIdx.x  (with grid-stride loop)
//   if distance(bullet, zombie) < threshold → mark both as dead.

__global__ void collisionKernel(
    const float* bx, const float* by, bool* bAlive,
    const float* zx, const float* zy, bool* zAlive, int* zHealth,
    int bulletCount, int zombieCount,
    float threshold)
{
    int bi = blockIdx.x;
    if (bi >= bulletCount || !bAlive[bi]) return;

    for (int zi = threadIdx.x; zi < zombieCount; zi += blockDim.x) {
        if (!zAlive[zi]) continue;

        float dx = bx[bi] - zx[zi];
        float dy = by[bi] - zy[zi];
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist < threshold) {
            bAlive[bi] = false;
            // Atomic decrement so multiple bullets don't corrupt health
            int prev = atomicSub(&zHealth[zi], 1);
            if (prev <= 1) zAlive[zi] = false;
            return; // This bullet is consumed
        }
    }
}

// ===================== Host Wrapper =====================
// Called from Game.cpp when mode == GPU

// Include the project header inside an extern "C++" block so nvcc
// can see the struct definitions.
extern "C++" {
    #include "../include/Entities.h"
    #include "../include/Processors.h"
}

void checkCollisionsGPU(BulletPool& b, ZombieSwarm& z, Player& p)
{
    if (b.count == 0 || z.count == 0) return;

    // --- Device memory ---
    float *d_bx, *d_by, *d_zx, *d_zy;
    bool  *d_bAlive, *d_zAlive;
    int   *d_zHealth;

    size_t bBytes  = b.count * sizeof(float);
    size_t zBytes  = z.count * sizeof(float);
    size_t zbBytes = b.count * sizeof(bool);
    size_t zzBytes = z.count * sizeof(bool);
    size_t zhBytes = z.count * sizeof(int);

    cudaMalloc(&d_bx, bBytes);
    cudaMalloc(&d_by, bBytes);
    cudaMalloc(&d_zx, zBytes);
    cudaMalloc(&d_zy, zBytes);
    cudaMalloc(&d_bAlive, zbBytes);
    cudaMalloc(&d_zAlive, zzBytes);
    cudaMalloc(&d_zHealth, zhBytes);

    // --- Copy Host → Device ---
    cudaMemcpy(d_bx,     b.x,       bBytes,  cudaMemcpyHostToDevice);
    cudaMemcpy(d_by,     b.y,       bBytes,  cudaMemcpyHostToDevice);
    cudaMemcpy(d_zx,     z.x,       zBytes,  cudaMemcpyHostToDevice);
    cudaMemcpy(d_zy,     z.y,       zBytes,  cudaMemcpyHostToDevice);
    cudaMemcpy(d_bAlive, b.isAlive, zbBytes, cudaMemcpyHostToDevice);
    cudaMemcpy(d_zAlive, z.isAlive, zzBytes, cudaMemcpyHostToDevice);
    cudaMemcpy(d_zHealth, z.health, zhBytes, cudaMemcpyHostToDevice);

    // --- Launch kernel ---
    int threads = 256;
    int blocks  = b.count;
    float threshold = ZOMBIE_RADIUS + BULLET_RADIUS;

    collisionKernel<<<blocks, threads>>>(
        d_bx, d_by, d_bAlive,
        d_zx, d_zy, d_zAlive, d_zHealth,
        b.count, z.count, threshold);

    cudaDeviceSynchronize();

    // --- Copy Device → Host ---
    cudaMemcpy(b.isAlive, d_bAlive, zbBytes, cudaMemcpyDeviceToHost);
    cudaMemcpy(z.isAlive, d_zAlive, zzBytes, cudaMemcpyDeviceToHost);
    cudaMemcpy(z.health,  d_zHealth, zhBytes, cudaMemcpyDeviceToHost);

    // --- Free device memory ---
    cudaFree(d_bx);
    cudaFree(d_by);
    cudaFree(d_zx);
    cudaFree(d_zy);
    cudaFree(d_bAlive);
    cudaFree(d_zAlive);
    cudaFree(d_zHealth);

    // --- Zombie vs Player (trivial — stays on CPU) ---
    for (int i = 0; i < z.count; i++) {
        if (!z.isAlive[i]) continue;
        float dx   = p.x - z.x[i];
        float dy   = p.y - z.y[i];
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < PLAYER_RADIUS + ZOMBIE_RADIUS) {
            if (p.invincibleTimer <= 0.0f) {
                p.health -= 10;
                p.invincibleTimer = 0.5f;
            }
        }
    }
}

#endif // USE_CUDA
