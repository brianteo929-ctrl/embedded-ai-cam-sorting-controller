#include "fsm.h"
#include "motor.h"
#include "esp_log.h"

static const char *TAG = "FSM";

typedef enum {
    IDLE,
    ACTUATE,
    RESET
} state_t;

static state_t state;

void fsm_init(void) {
    state = IDLE;
}

void fsm_update(void) {
    switch (state) {

        case IDLE:
            ESP_LOGI(TAG, "IDLE");
            state = ACTUATE;
            break;

        case ACTUATE:
            ESP_LOGI(TAG, "ACTUATE");
            motor_move_steps(200, 1);
            state = RESET;
            break;
        // 
        case RESET:
            ESP_LOGI(TAG, "RESET");
            state = IDLE;
            break;
    }
}