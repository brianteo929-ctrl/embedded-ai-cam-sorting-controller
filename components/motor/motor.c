#include "motor.h"
#include "esp_log.h"

static const char *TAG = "MOTOR";

void motor_init(void) {
    ESP_LOGI(TAG, "Motor initialized");
}

void motor_move_steps(int steps, int direction) {
    ESP_LOGI(TAG, "Move %d steps, direction %d", steps, direction);
}