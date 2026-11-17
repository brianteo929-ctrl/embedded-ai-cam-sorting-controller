#include "motion.h"
#include <stddef.h>
bool motion_config_valid(const motion_config_t *c)
{
    if (!c || c->enable_pin != 15 || c->step_pin[0] == c->step_pin[1] || c->dir_pin[0] == c->dir_pin[1]) return false;
    for (unsigned i = 0; i < 2; ++i) {
        if ((c->step_pin[i] != 2 && c->step_pin[i] != 14) || (c->dir_pin[i] != 12 && c->dir_pin[i] != 13) ||
            c->limit[i] <= 0 || c->limit[i] > MOTION_MAX_STEPS / 2) return false;
    }
    return c->cruise_period_us >= 500 && c->start_period_us >= c->cruise_period_us &&
        c->start_period_us <= 20000 && c->ramp_steps <= MOTION_MAX_STEPS;
}
bool motion_plan(const motion_config_t *c, const int32_t from[2], const int32_t to[2], int32_t delta[2])
{
    if (!motion_config_valid(c) || !from || !to || !delta) return false;
    for (unsigned i = 0; i < 2; ++i)
        if (from[i] < -c->limit[i] || from[i] > c->limit[i] || to[i] < -c->limit[i] || to[i] > c->limit[i]) return false;
    for (unsigned i = 0; i < 2; ++i) delta[i] = to[i] - from[i];
    return true;
}
uint32_t motion_period(const motion_config_t *c, uint32_t index, uint32_t steps)
{
    if (!c || !steps || index >= steps) return 0;
    uint32_t edge = index < steps - 1 - index ? index : steps - 1 - index;
    if (!c->ramp_steps || edge >= c->ramp_steps) return c->cruise_period_us;
    return c->start_period_us - (c->start_period_us - c->cruise_period_us) * edge / c->ramp_steps;
}
uint32_t motion_duration_ms(const motion_config_t *c, const int32_t delta[2])
{
    if (!motion_config_valid(c) || !delta) return 0;
    uint32_t us = 0;
    for (unsigned axis = 0; axis < 2; ++axis) {
        if (delta[axis] < -MOTION_MAX_STEPS || delta[axis] > MOTION_MAX_STEPS) return 0;
        uint32_t count = delta[axis] < 0 ? (uint32_t)-delta[axis] : (uint32_t)delta[axis];
        if (count) us += 2000;
        for (uint32_t i = 0; i < count; ++i) us += motion_period(c, i, count);
    }
    return (us + 999) / 1000;
}
