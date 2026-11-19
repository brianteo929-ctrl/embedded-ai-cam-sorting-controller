#pragma once
#include <stdbool.h>
#include <stdint.h>
static inline bool vision_event_current(uint32_t event_cycle, uint32_t current_cycle,
                                       uint32_t outstanding_cycle, bool terminal, bool *busy)
{
    if (event_cycle != outstanding_cycle || !*busy) return false;
    if (terminal) *busy = false;
    return event_cycle == current_cycle;
}
