#pragma once
#include <stdbool.h>
#include <stdint.h>
#define MOTION_MAX_STEPS 2048
typedef struct {
    int32_t limit[2];
    uint32_t start_period_us, cruise_period_us, ramp_steps;
    int step_pin[2], dir_pin[2], enable_pin;
} motion_config_t;
bool motion_config_valid(const motion_config_t *c);
bool motion_plan(const motion_config_t *c, const int32_t from[2], const int32_t to[2], int32_t delta[2]);
uint32_t motion_period(const motion_config_t *c, uint32_t index, uint32_t steps);
uint32_t motion_duration_ms(const motion_config_t *c, const int32_t delta[2]);
