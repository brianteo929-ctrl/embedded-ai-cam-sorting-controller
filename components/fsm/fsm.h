#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef enum {
    SORT_UNHOMED, SORT_IDLE, SORT_CAPTURE, SORT_INFERENCE, SORT_ACTUATE,
    SORT_VERIFY, SORT_RESET, SORT_FAULT
} sort_state_t;
typedef enum {
    EV_ARM, EV_HOME, EV_TRIGGER, EV_CAPTURED, EV_ACCEPT, EV_REJECT,
    EV_MOTION_DONE, EV_TICK, EV_ERROR, EV_STOP, EV_RECOVER
} sort_event_t;
typedef enum { FAULT_NONE, FAULT_STOP, FAULT_CAPTURE, FAULT_INFERENCE, FAULT_MOTION, FAULT_IO } sort_fault_t;
enum { ACT_NONE = 0, ACT_ARM = 1, ACT_REFERENCE = 2, ACT_CAPTURE = 4, ACT_SORT = 8, ACT_RETURN = 16, ACT_DISABLE = 32 };
typedef struct { uint32_t capture_ms, inference_ms, motion_ms, settle_ms; } fsm_config_t;
typedef struct {
    sort_state_t state;
    sort_fault_t fault;
    bool armed, referenced;
    uint32_t entered_ms;
    fsm_config_t config;
} sorter_fsm_t;
bool fsm_init(sorter_fsm_t *fsm, const fsm_config_t *config, uint32_t now);
unsigned fsm_event(sorter_fsm_t *fsm, sort_event_t event, uint32_t now);
const char *fsm_state_name(sort_state_t state);
const char *fsm_fault_name(sort_fault_t fault);
