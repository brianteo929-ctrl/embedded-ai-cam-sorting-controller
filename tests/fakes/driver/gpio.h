#pragma once
#include "esp_err.h"
#include <stdint.h>
#define GPIO_MODE_OUTPUT 1
typedef struct { int mode; uint64_t pin_bit_mask; } gpio_config_t;
esp_err_t gpio_set_level(int pin, unsigned value);
esp_err_t gpio_config(const gpio_config_t *cfg);
