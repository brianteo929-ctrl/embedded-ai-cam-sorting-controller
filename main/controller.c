#include "controller.h"
#include "camera_service.h"
#include "image.h"
#include "motor.h"
#include "vision_guard.h"
#include "sdkconfig.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <string.h>

typedef struct { uint32_t cycle; mock_case_t scenario; } vision_request_t;
typedef struct {
    uint32_t cycle, capture_ms, inference_ms;
    bool captured;
    esp_err_t error;
    decision_t decision;
} vision_event_t;
static QueueHandle_t commands, vision_requests, vision_events;
static SemaphoreHandle_t status_lock;
static TaskHandle_t control_handle, vision_handle;
static control_status_t status, published;
static bool motor_ready;
static uint32_t outstanding_vision_cycle;
static const char *TAG = "SORTER";
static uint32_t now_ms(void) { return (uint32_t)(esp_timer_get_time() / 1000); }
static const motion_config_t motion = {
    .limit = {CONFIG_SORTER_POSITION_LIMIT, CONFIG_SORTER_POSITION_LIMIT},
    .start_period_us = CONFIG_SORTER_START_PERIOD_US, .cruise_period_us = CONFIG_SORTER_CRUISE_PERIOD_US,
    .ramp_steps = CONFIG_SORTER_RAMP_STEPS,
    .step_pin = {CONFIG_SORTER_STEP_1, CONFIG_SORTER_STEP_2},
    .dir_pin = {CONFIG_SORTER_DIR_1, CONFIG_SORTER_DIR_2}, .enable_pin = CONFIG_SORTER_ENABLE,
};
static const int32_t destinations[4][2] = {
    {0, 0}, {CONFIG_SORTER_RECYCLABLE_1, CONFIG_SORTER_RECYCLABLE_2},
    {CONFIG_SORTER_ORGANIC_1, CONFIG_SORTER_ORGANIC_2}, {CONFIG_SORTER_HAZARDOUS_1, CONFIG_SORTER_HAZARDOUS_2},
};
static void vision_task(void *arg)
{
    (void)arg;
    unsigned width, height;
    inference_dimensions(&width, &height);
    bool valid_size = width && height && width <= 256 && height <= 256;
    uint8_t *input = valid_size ? heap_caps_malloc(width * height * 3, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) : NULL;
#if CONFIG_SORTER_CAMERA_MOCK
    if (!input && valid_size) input = heap_caps_malloc(width * height * 3, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (input) memset(input, 127, width * height * 3);
#else
    uint8_t *rgb = heap_caps_malloc(320 * 240 * 3, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#endif
    vision_request_t request;
    for (;;) {
        xQueueReceive(vision_requests, &request, portMAX_DELAY);
        uint32_t start = now_ms();
        vision_event_t event = {.cycle = request.cycle, .error = ESP_OK,
            .decision = {.reason = DECISION_MODEL_ERROR}};
        if (!input) event.error = ESP_ERR_NO_MEM;
#if !CONFIG_SORTER_CAMERA_MOCK
        camera_fb_t *frame = event.error == ESP_OK && rgb ? camera_acquire(500) : NULL;
        if (!frame) event.error = rgb ? ESP_ERR_TIMEOUT : ESP_ERR_NO_MEM;
        else {
            if (frame->format != PIXFORMAT_JPEG || frame->width != 320 || frame->height != 240 ||
                camera_decode_rgb888(frame, rgb, 320 * 240 * 3) != ESP_OK) event.error = ESP_FAIL;
            else if (!image_center_crop(rgb, frame->width, frame->height, input, width, height)) event.error = ESP_ERR_INVALID_SIZE;
            camera_release(frame);
        }
#endif
        event.capture_ms = now_ms() - start;
        if (event.error == ESP_OK) {
            event.captured = true;
            xQueueSend(vision_events, &event, portMAX_DELAY);
            start = now_ms();
            inference_result_t result = {0};
            event.error = inference_run(input, width, height, request.scenario, &result);
            event.inference_ms = now_ms() - start;
            event.decision = inference_decide(&result, event.error == ESP_OK,
                CONFIG_SORTER_CONFIDENCE_PERCENT / 100.0f, CONFIG_SORTER_CLASSIFICATION_MARGIN_PERCENT / 100.0f);
        }
        event.captured = false;
        xQueueSend(vision_events, &event, portMAX_DELAY);
    }
}

static void dispatch(sort_event_t event, mock_case_t scenario)
{
    sort_state_t previous = status.fsm.state;
    unsigned action = fsm_event(&status.fsm, event, now_ms());
    if (previous != status.fsm.state)
        ESP_LOGI(TAG, "cycle=%lu state=%s fault=%s", (unsigned long)status.cycle,
            fsm_state_name(status.fsm.state), fsm_fault_name(status.fsm.fault));
    esp_err_t err = ESP_OK;
    if (action & ACT_DISABLE) {
        ++status.cycle;
        err = motor_stop();
        if (err != ESP_OK) status.healthy = false;
    }
    if (action & ACT_ARM) err = motor_arm();
    if (action & ACT_REFERENCE) err = motor_reference();
    if (action & ACT_CAPTURE) {
        vision_request_t request = {.cycle = ++status.cycle, .scenario = scenario};
        status.decision = (decision_t){.reason = DECISION_EMPTY};
        status.capture_ms = status.inference_ms = 0;
        if (xQueueSend(vision_requests, &request, 0) == pdTRUE) {
            outstanding_vision_cycle = request.cycle;
            status.vision_busy = true;
        }
        else err = ESP_ERR_INVALID_STATE;
    }
    if (action & (ACT_SORT | ACT_RETURN)) {
        unsigned bin = action & ACT_RETURN ? BIN_NONE : status.decision.bin;
        if (bin > BIN_HAZARDOUS) err = ESP_ERR_INVALID_ARG;
        else err = motor_start(destinations[bin], now_ms());
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "action=%u error=%s", action, esp_err_to_name(err));
        if (!(action & ACT_DISABLE)) dispatch(EV_ERROR, MOCK_EMPTY);
    }
}

static bool can_arm(void)
{
    if (!status.healthy || !motor_ready || !camera_service_ready()) return false;
#if CONFIG_SORTER_MOTOR_HARDWARE
#if !CONFIG_SORTER_MOTION_CALIBRATED
    return false;
#endif
#if CONFIG_SORTER_INFERENCE_MOCK && !CONFIG_SORTER_ALLOW_MOCK_MOTION
    return false;
#endif
#if !CONFIG_SORTER_INFERENCE_MOCK && !CONFIG_SORTER_MODEL_VALIDATED
    return false;
#endif
#endif
    return true;
}
static void process_command(control_command_t cmd)
{
    ++status.command_count;
    status.command_result = "rejected_state";
    sort_event_t event;
    switch (cmd.kind) {
    case CMD_ARM:
        if (status.fsm.state != SORT_UNHOMED || status.fsm.armed) return;
        if (!can_arm()) { status.command_result = "blocked_configuration_or_hardware"; return; }
        event = EV_ARM; break;
    case CMD_HOME:
        if (status.fsm.state != SORT_UNHOMED || !status.fsm.armed || status.vision_busy) return;
        event = EV_HOME; break;
    case CMD_TRIGGER:
        if (status.fsm.state != SORT_IDLE || status.vision_busy) return;
        event = EV_TRIGGER; break;
    case CMD_RECOVER:
        if (status.fsm.state != SORT_FAULT || status.vision_busy || motor_busy() || !status.healthy) return;
        event = EV_RECOVER; break;
    default: return;
    }
    dispatch(event, cmd.scenario);
    status.command_result = status.fsm.state == SORT_FAULT ? "failed" : "accepted";
}
static void control_task(void *arg)
{
    (void)arg;
    for (;;) {
        uint32_t notification = 0;
        if (xTaskNotifyWait(0, UINT32_MAX, &notification, 0) == pdTRUE && notification) {
            xQueueReset(commands);
            dispatch(EV_STOP, MOCK_EMPTY);
            ++status.command_count; status.command_result = "stopped";
        }
        dispatch(EV_TICK, MOCK_EMPTY);
        vision_event_t event;
        while (xQueueReceive(vision_events, &event, 0) == pdTRUE) {
            if (!vision_event_current(event.cycle, status.cycle, outstanding_vision_cycle,
                                      !event.captured, &status.vision_busy)) continue;
            status.capture_ms = event.capture_ms;
            if (event.captured) dispatch(EV_CAPTURED, MOCK_EMPTY);
            else {
                status.inference_ms = event.inference_ms; status.decision = event.decision;
                ESP_LOGI(TAG, "backend=%s reason=%s bin=%s capture_ms=%lu inference_ms=%lu", inference_backend(),
                    decision_name(event.decision.reason), bin_name(event.decision.bin),
                    (unsigned long)event.capture_ms, (unsigned long)event.inference_ms);
                dispatch(event.error != ESP_OK ? EV_ERROR : event.decision.reason == DECISION_ACCEPT ? EV_ACCEPT : EV_REJECT, MOCK_EMPTY);
            }
        }
        bool done = false;
        esp_err_t err = motor_poll(now_ms(), &done);
        if (err != ESP_OK) dispatch(EV_ERROR, MOCK_EMPTY);
        else if (done) dispatch(EV_MOTION_DONE, MOCK_EMPTY);
        control_command_t command;
        if (xQueueReceive(commands, &command, 0) == pdTRUE) process_command(command);
        motor_position(status.position);
        status.control_stack = uxTaskGetStackHighWaterMark(NULL);
        status.vision_stack = uxTaskGetStackHighWaterMark(vision_handle);
        xSemaphoreTake(status_lock, portMAX_DELAY); published = status; xSemaphoreGive(status_lock);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
esp_err_t controller_init(void)
{
    fsm_config_t config = {CONFIG_SORTER_CAPTURE_TIMEOUT_MS, CONFIG_SORTER_INFERENCE_TIMEOUT_MS,
        CONFIG_SORTER_MOTION_TIMEOUT_MS, CONFIG_SORTER_SETTLE_MS};
    if (!fsm_init(&status.fsm, &config, now_ms()) || !motion_config_valid(&motion)) return ESP_ERR_INVALID_ARG;
    for (unsigned i = 1; i < 4; ++i) {
        int32_t delta[2];
        if (!motion_plan(&motion, destinations[0], destinations[i], delta) ||
            motion_duration_ms(&motion, delta) + 200 >= config.motion_ms) return ESP_ERR_INVALID_ARG;
    }
    inference_result_t empty = {0};
#if CONFIG_SORTER_INFERENCE_MOCK
    empty.mocked = true;
#endif
    if (inference_validate() != ESP_OK || inference_decide(&empty, true,
        CONFIG_SORTER_CONFIDENCE_PERCENT / 100.0f, CONFIG_SORTER_CLASSIFICATION_MARGIN_PERCENT / 100.0f).reason != DECISION_EMPTY)
        return ESP_ERR_INVALID_ARG;
    ESP_LOGI(TAG, "backend=%s model=%s", inference_backend(), inference_model_version());
    status.command_result = "none";
    motor_ready = motor_init(&motion) == ESP_OK;
    status.healthy = motor_ready;
    commands = xQueueCreate(4, sizeof(control_command_t));
    vision_requests = xQueueCreate(1, sizeof(vision_request_t));
    vision_events = xQueueCreate(4, sizeof(vision_event_t));
    status_lock = xSemaphoreCreateMutex();
    if (!commands || !vision_requests || !vision_events || !status_lock) return ESP_ERR_NO_MEM;
    published = status;
    if (xTaskCreate(vision_task, "vision", 12288, NULL, 3, &vision_handle) != pdPASS) return ESP_ERR_NO_MEM;
    if (xTaskCreate(control_task, "control", 6144, NULL, 5, &control_handle) != pdPASS) return ESP_ERR_NO_MEM;
    return ESP_OK;
}
esp_err_t controller_submit(control_command_t command)
{
    if (!control_handle || (unsigned)command.kind > CMD_RECOVER || (unsigned)command.scenario >= MOCK_COUNT) return ESP_ERR_INVALID_ARG;
    if (command.kind == CMD_STOP) return xTaskNotify(control_handle, 1, eSetBits) == pdPASS ? ESP_OK : ESP_FAIL;
    return xQueueSend(commands, &command, 0) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}
void controller_status(control_status_t *out)
{
    xSemaphoreTake(status_lock, portMAX_DELAY); *out = published; xSemaphoreGive(status_lock);
}
