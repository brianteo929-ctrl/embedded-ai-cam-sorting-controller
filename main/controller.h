#pragma once
#include "decision.h"
#include "fsm.h"
#include "inference.h"
#include "esp_err.h"
typedef enum { CMD_ARM, CMD_HOME, CMD_TRIGGER, CMD_STOP, CMD_RECOVER } command_kind_t;
typedef struct { command_kind_t kind; mock_case_t scenario; } control_command_t;
typedef struct {
    sorter_fsm_t fsm;
    decision_t decision;
    int32_t position[2];
    uint32_t cycle, command_count, capture_ms, inference_ms, control_stack, vision_stack;
    bool vision_busy, healthy;
    const char *command_result;
} control_status_t;
esp_err_t controller_init(void);
esp_err_t controller_submit(control_command_t command);
void controller_status(control_status_t *status);
