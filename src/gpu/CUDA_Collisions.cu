// ============================================================
// gpu/CUDA_Collisions.cu — GPU-accelerated collision detection
// CUDA kernel: one block per bullet, threads iterate zombies
// ============================================================

#ifdef USE_CUDA

#include <cuda_runtime.h>
#include <cstdio>
#include <cmath>

extern "C++" {
    #include "../include/Entities.h"
    #include "../include/Processors.h"
    #include <vector>
}

// ===================== CUDA Kernel =====================
// Grid:  bulletCount blocks
// Block: up to 256 threads — each checks (zombieCount / blockDim.x) zombies
__global__ void collisionKernel(
    const float* __restrict__ bx, const float* __restrict__ by, bool* bAlive,
    const float* __restrict__ zx, const float* __restrict__ zy,
    bool* zAlive, int* zHealth,
    int bulletCount, int zombieCount,
    float threshold2)   // pass squared threshold so no sqrt needed
{
    int bi = blockIdx.x;
    if (bi >= bulletCount || !bAlive[bi]) return;

    for (int zi = threadIdx.x; zi < zombieCount; zi += blockDim.x) {
        if (!zAlive[zi]) continue;

        float dx = bx[bi] - zx[zi];
        float dy = by[bi] - zy[zi];

        if (dx * dx + dy * dy < threshold2) {
            bAlive[bi] = false;
            int prev = atomicSub(&zHealth[zi], 1);
            if (prev <= 1) zAlive[zi] = false;
            return;
        }
    }
}

// ===================== Host Wrapper =====================
void checkCollisionsGPU(BulletPool& b, ZombieSwarm& z, Player& p,
                         const std::vector<Obstacle>& obstacles)
{
    if (b.count == 0 || z.count == 0) return;

    // CPU: bullet vs obstacle (cheap, only ~8 buildings)
    for (int bi = 0; bi < b.count; bi++) {
        if (!b.isAlive[bi]) continue;
        for (const auto& obs : obstacles) {
            if (obs.intersects(b.x[bi], b.y[bi], BULLET_RADIUS)) {
                b.isAlive[bi] = false;
                break;
            }
        }
    }

    // Allocate device memory
    float *d_bx, *d_by, *d_zx, *d_zy;
    bool  *d_bAlive, *d_zAlive;
    int   *d_zHealth;

    size_t bSz  = b.count * sizeof(float);
    size_t zSz  = z.count * sizeof(float);
    size_t bzSz = b.count * sizeof(bool);
    size_t zzSz = z.count * sizeof(bool);
    size_t zhSz = z.count * sizeof(int);

    cudaMalloc(&d_bx,     bSz);
    cudaMalloc(&d_by,     bSz);
    cudaMalloc(&d_zx,     zSz);
    cudaMalloc(&d_zy,     zSz);
    cudaMalloc(&d_bAlive, bzSz);
    cudaMalloc(&d_zAlive, zzSz);
    cudaMalloc(&d_zHealth, zhSz);

    cudaMemcpy(d_bx,      b.x,       bSz,  cudaMemcpyHostToDevice);
    cudaMemcpy(d_by,      b.y,       bSz,  cudaMemcpyHostToDevice);
    cudaMemcpy(d_zx,      z.x,       zSz,  cudaMemcpyHostToDevice);
    cudaMemcpy(d_zy,      z.y,       zSz,  cudaMemcpyHostToDevice);
    cudaMemcpy(d_bAlive,  b.isAlive, bzSz, cudaMemcpyHostToDevice);
    cudaMemcpy(d_zAlive,  z.isAlive, zzSz, cudaMemcpyHostToDevice);
    cudaMemcpy(d_zHealth, z.health,  zhSz, cudaMemcpyHostToDevice);

    int threads = 256;
    float t = ZOMBIE_RADIUS + BULLET_RADIUS;
    float threshold2 = t * t;

    collisionKernel<<<b.count, threads>>>(
        d_bx, d_by, d_bAlive,
        d_zx, d_zy, d_zAlive, d_zHealth,
        b.count, z.count, threshold2);

    cudaDeviceSynchronize();

    cudaMemcpy(b.isAlive, d_bAlive,  bzSz, cudaMemcpyDeviceToHost);
    cudaMemcpy(z.isAlive, d_zAlive,  zzSz, cudaMemcpyDeviceToHost);
    cudaMemcpy(z.health,  d_zHealth, zhSz, cudaMemcpyDeviceToHost);

    cudaFree(d_bx); cudaFree(d_by);
    cudaFree(d_zx); cudaFree(d_zy);
    cudaFree(d_bAlive); cudaFree(d_zAlive); cudaFree(d_zHealth);

    // CPU: zombie vs player
    const float touchP2 = (PLAYER_RADIUS + ZOMBIE_RADIUS) * (PLAYER_RADIUS + ZOMBIE_RADIUS);
    for (int i = 0; i < z.count; i++) {
        if (!z.isAlive[i]) continue;
        float dx = p.x - z.x[i];
        float dy = p.y - z.y[i];
        if (dx * dx + dy * dy < touchP2) {
            if (p.invincibleTimer <= 0.0f) {
                p.health -= 10;
                p.invincibleTimer = 0.5f;
            }
        }
    }
}

#endif // USE_CUDA
