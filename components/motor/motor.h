#pragma once
#include "esp_err.h"
#include "motion.h"
esp_err_t motor_init(const motion_config_t *config);
esp_err_t motor_arm(void);
esp_err_t motor_reference(void);
esp_err_t motor_start(const int32_t target[2], uint32_t now);
esp_err_t motor_poll(uint32_t now, bool *done);
esp_err_t motor_stop(void);
void motor_position(int32_t out[2]);
bool motor_busy(void);
