#include "trapezoidal.h"
#include <algorithm>

TrapProfile computeProfile(uint32_t total_steps,
                           uint32_t start_speed,
                           uint32_t max_speed,
                           uint32_t accel)
{
    if (accel == 0 || max_speed <= start_speed) {
        return {0, total_steps, 0};
    }

    uint32_t steps_to_max = (max_speed - start_speed) / accel;

    if (2 * steps_to_max >= total_steps) {
        uint32_t half = total_steps / 2;
        return {half, 0, total_steps - half};
    }

    uint32_t const_steps = total_steps - 2 * steps_to_max;
    return {steps_to_max, const_steps, steps_to_max};
}
