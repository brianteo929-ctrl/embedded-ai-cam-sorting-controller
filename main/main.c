#include "fsm.h"
#include "motor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Tag used for ESP logging (helps identify logs in serial output)
static const char *TAG = "MAIN";

void app_main(void)
{
    // This function is the entry point for ESP-IDF applications
    // It runs after the RTOS scheduler has started

    ESP_LOGI(TAG, "System starting...");

    // Initialize motor driver GPIOs and any required peripherals
    motor_init();

    // Set initial state (e.g., IDLE)
    fsm_init();

    ESP_LOGI(TAG, "Entering main control loop");

    // Continuously updates system behavior via FSM
    // Runs indefinitely as part of the embedded system
    while (1) {

        // Update finite state machine
        // Handles logic such as IDLE, ACTUATE, RESET transitions
        fsm_update();

        // Small delay to:
        // Allow other FreeRTOS tasks to run
        // Control loop execution rate (~10 Hz here)
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}