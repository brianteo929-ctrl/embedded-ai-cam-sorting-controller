#include "motor.h"
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/rmt_tx.h"
#include <string.h>

static motion_config_t cfg;
static bool initialized, armed, referenced, busy;
static int32_t position[2], target_position[2], delta[2];
static unsigned axis;
static uint32_t started, duration;
#if CONFIG_SORTER_MOTOR_HARDWARE
static rmt_channel_handle_t channel[2];
static rmt_encoder_handle_t encoder;
static bool channel_enabled[2];
static rmt_symbol_word_t pulses[MOTION_MAX_STEPS + 1];

static esp_err_t start_axis(void)
{
    while (axis < 2 && delta[axis] == 0) ++axis;
    if (axis == 2) return ESP_OK;
    bool invert = axis == 0 ? CONFIG_SORTER_INVERT_1 : CONFIG_SORTER_INVERT_2;
    esp_err_t err = gpio_set_level(cfg.dir_pin[axis], (delta[axis] > 0) ^ invert);
    if (err != ESP_OK) return err;
    uint32_t count = delta[axis] < 0 ? (uint32_t)-delta[axis] : (uint32_t)delta[axis];
    pulses[0] = (rmt_symbol_word_t){.level0 = 0, .duration0 = 1000, .level1 = 0, .duration1 = 1000};
    for (uint32_t i = 0; i < count; ++i)
        pulses[i + 1] = (rmt_symbol_word_t){.level0 = 1, .duration0 = 3, .level1 = 0,
            .duration1 = motion_period(&cfg, i, count) - 3};
    rmt_transmit_config_t tx = {.flags.eot_level = 0, .flags.queue_nonblocking = 1};
    return rmt_transmit(channel[axis], encoder, pulses, (count + 1) * sizeof(pulses[0]), &tx);
}
#endif

esp_err_t motor_init(const motion_config_t *config)
{
    if (initialized || !motion_config_valid(config)) return ESP_ERR_INVALID_ARG;
    cfg = *config;
#if CONFIG_SORTER_MOTOR_HARDWARE
    gpio_config_t pins = {.mode = GPIO_MODE_OUTPUT, .pin_bit_mask = 1ULL << cfg.enable_pin};
    esp_err_t err = gpio_set_level(cfg.enable_pin, 1);
    if (err == ESP_OK) err = gpio_config(&pins);
    if (err != ESP_OK) return err;
    for (unsigned i = 0; i < 2; ++i) {
        pins.pin_bit_mask = 1ULL << cfg.dir_pin[i];
        if ((err = gpio_set_level(cfg.dir_pin[i], 0)) != ESP_OK || (err = gpio_config(&pins)) != ESP_OK) goto failed;
        rmt_tx_channel_config_t tx = {.gpio_num = cfg.step_pin[i], .clk_src = RMT_CLK_SRC_DEFAULT,
            .resolution_hz = 1000000, .mem_block_symbols = 64, .trans_queue_depth = 1};
        if ((err = rmt_new_tx_channel(&tx, &channel[i])) != ESP_OK) goto failed;
    }
    rmt_copy_encoder_config_t copy;
    memset(&copy, 0, sizeof(copy));
    if ((err = rmt_new_copy_encoder(&copy, &encoder)) != ESP_OK) goto failed;
#endif
    initialized = true;
    return ESP_OK;
#if CONFIG_SORTER_MOTOR_HARDWARE
failed:
    gpio_set_level(cfg.enable_pin, 1);
    for (unsigned i = 0; i < 2; ++i) if (channel[i]) { rmt_del_channel(channel[i]); channel[i] = NULL; }
    return err;
#endif
}
esp_err_t motor_arm(void)
{
    if (!initialized || busy || armed) return ESP_ERR_INVALID_STATE;
#if CONFIG_SORTER_MOTOR_HARDWARE
    for (unsigned i = 0; i < 2; ++i) {
        esp_err_t err = rmt_enable(channel[i]);
        if (err != ESP_OK) { motor_stop(); return err; }
        channel_enabled[i] = true;
    }
    esp_err_t err = gpio_set_level(cfg.enable_pin, 0);
    if (err != ESP_OK) { motor_stop(); return err; }
#endif
    armed = true;
    return ESP_OK;
}
esp_err_t motor_reference(void)
{
    if (!armed || busy) return ESP_ERR_INVALID_STATE;
    memset(position, 0, sizeof(position));
    referenced = true;
    return ESP_OK;
}
esp_err_t motor_start(const int32_t target[2], uint32_t now)
{
    if (!armed || !referenced || busy) return ESP_ERR_INVALID_STATE;
    if (!motion_plan(&cfg, position, target, delta)) return ESP_ERR_INVALID_ARG;
    memcpy(target_position, target, sizeof(target_position));
    axis = 0; started = now; duration = motion_duration_ms(&cfg, delta); busy = true;
#if CONFIG_SORTER_MOTOR_HARDWARE
    esp_err_t err = start_axis();
    if (err != ESP_OK) { motor_stop(); return err; }
#endif
    return ESP_OK;
}
esp_err_t motor_poll(uint32_t now, bool *done)
{
    if (!done) return ESP_ERR_INVALID_ARG;
    *done = false;
    if (!busy) return ESP_OK;
#if CONFIG_SORTER_MOTOR_HARDWARE
    (void)now;
    if (axis < 2) {
        esp_err_t err = rmt_tx_wait_all_done(channel[axis], 0);
        if (err == ESP_ERR_TIMEOUT) return ESP_OK;
        if (err != ESP_OK) { motor_stop(); return err; }
        position[axis] = target_position[axis];
        ++axis;
        if ((err = start_axis()) != ESP_OK) { motor_stop(); return err; }
        if (axis < 2) return ESP_OK;
    }
#else
    if (now - started < duration) return ESP_OK;
#endif
    memcpy(position, target_position, sizeof(position));
    busy = false; *done = true;
    return ESP_OK;
}
esp_err_t motor_stop(void)
{
    esp_err_t result = ESP_OK;
#if CONFIG_SORTER_MOTOR_HARDWARE
    if (initialized) result = gpio_set_level(cfg.enable_pin, 1);
    for (unsigned i = 0; i < 2; ++i) if (channel_enabled[i]) {
        esp_err_t err = rmt_disable(channel[i]);
        if (err == ESP_OK) channel_enabled[i] = false;
        else result = err;
    }
    if (encoder) rmt_encoder_reset(encoder);
#endif
    busy = armed = referenced = false;
    return result;
}
void motor_position(int32_t out[2]) { memcpy(out, position, sizeof(position)); }
bool motor_busy(void) { return busy; }
