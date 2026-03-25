// ============================================================
// Utils.h — Timer helper using SDL high-performance counter
// ============================================================
#pragma once

#include <SDL.h>

struct Timer {
    Uint64 lastTime;
    Uint64 currentTime;
    float  deltaTime;   // seconds since last frame

    void init() {
        lastTime    = SDL_GetPerformanceCounter();
        currentTime = lastTime;
        deltaTime   = 0.0f;
    }

    void tick() {
        lastTime    = currentTime;
        currentTime = SDL_GetPerformanceCounter();
        deltaTime   = static_cast<float>(currentTime - lastTime)
                    / static_cast<float>(SDL_GetPerformanceFrequency());
        // Cap at 50 ms to prevent physics explosions on lag spikes
        if (deltaTime > 0.05f) deltaTime = 0.05f;
    }
};

// --------------- Math helpers ---------------
inline float vec2Length(float x, float y) {
    return sqrtf(x * x + y * y);
}

inline void vec2Normalize(float& x, float& y) {
    float len = vec2Length(x, y);
    if (len > 0.0001f) { x /= len; y /= len; }
}
