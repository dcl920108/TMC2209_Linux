#pragma once
#include <cstdint>

struct TrapProfile {
    uint32_t accel_steps;
    uint32_t const_steps;
    uint32_t decel_steps;
};

TrapProfile computeProfile(uint32_t total_steps,
                           uint32_t start_speed,
                           uint32_t max_speed,
                           uint32_t accel);
