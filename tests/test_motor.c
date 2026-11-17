#include "motor.h"
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/rmt_tx.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

struct fake_channel { bool enabled, transmitting, done; unsigned transmissions; size_t count; rmt_symbol_word_t pulses[MOTION_MAX_STEPS + 1]; } channels[2];
static unsigned created;
static unsigned levels[40];
static bool fail_next_tx;
esp_err_t gpio_set_level(int pin, unsigned value) { assert(pin >= 0 && pin < 40); levels[pin] = value; return ESP_OK; }
esp_err_t gpio_config(const gpio_config_t *c) { assert(c->mode == GPIO_MODE_OUTPUT); return ESP_OK; }
esp_err_t rmt_new_tx_channel(const rmt_tx_channel_config_t *c, rmt_channel_handle_t *out)
{
    assert(created < 2 && c->resolution_hz == 1000000 && c->trans_queue_depth == 1);
    *out = &channels[created++]; return ESP_OK;
}
esp_err_t rmt_new_copy_encoder(const rmt_copy_encoder_config_t *c, rmt_encoder_handle_t *e) { (void)c; *e = channels; return ESP_OK; }
esp_err_t rmt_transmit(rmt_channel_handle_t c, rmt_encoder_handle_t e, const void *p, size_t n, const rmt_transmit_config_t *cfg)
{
    (void)e; assert(c->enabled && !c->transmitting && cfg->flags.queue_nonblocking && !cfg->flags.eot_level);
    if (fail_next_tx) { fail_next_tx = false; return ESP_FAIL; }
    assert(n <= sizeof(c->pulses)); memcpy(c->pulses, p, n); c->count = n / sizeof(c->pulses[0]);
    c->transmitting = true; c->done = false; ++c->transmissions; return ESP_OK;
}
esp_err_t rmt_tx_wait_all_done(rmt_channel_handle_t c, int timeout)
{
    assert(timeout == 0);
    if (!c->done) return ESP_ERR_TIMEOUT;
    c->transmitting = false; return ESP_OK;
}
esp_err_t rmt_enable(rmt_channel_handle_t c) { assert(!c->enabled); c->enabled = true; return ESP_OK; }
esp_err_t rmt_disable(rmt_channel_handle_t c) { c->enabled = c->transmitting = false; return ESP_OK; }
esp_err_t rmt_del_channel(rmt_channel_handle_t c) { (void)c; return ESP_OK; }
esp_err_t rmt_encoder_reset(rmt_encoder_handle_t e) { (void)e; return ESP_OK; }
int main(void)
{
    motion_config_t c = {{200, 200}, 10000, 4000, 20, {14, 2}, {13, 12}, 15};
    int32_t goal[] = {80, -40}, position[2]; bool done;
    assert(motor_init(&c) == ESP_OK);
    assert(motor_start(goal, 0) == ESP_ERR_INVALID_STATE);
    assert(motor_arm() == ESP_OK);
    assert(motor_start(goal, 0) == ESP_ERR_INVALID_STATE);
    assert(motor_reference() == ESP_OK && motor_start(goal, 100) == ESP_OK);
    assert(motor_busy() && motor_reference() == ESP_ERR_INVALID_STATE);
    assert(motor_start(goal, 101) == ESP_ERR_INVALID_STATE);
#if CONFIG_SORTER_MOTOR_HARDWARE
    assert(created == 2 && levels[15] == 0 && levels[13] == 1);
    assert(channels[0].count == 81 && channels[1].transmissions == 0);
    assert(channels[0].pulses[0].level0 == 0 && channels[0].pulses[0].duration0 + channels[0].pulses[0].duration1 == 2000);
    for (unsigned i = 1; i < 81; ++i) {
        rmt_symbol_word_t p = channels[0].pulses[i];
        assert(p.level0 == 1 && p.level1 == 0 && p.duration0 == 3);
        assert(p.duration0 + p.duration1 == motion_period(&c, i - 1, 80));
    }
    assert(motor_poll(101, &done) == ESP_OK && !done);
    channels[0].done = true;
    assert(motor_poll(102, &done) == ESP_OK && !done && channels[1].count == 41);
    assert(levels[12] == 1); /* Negative direction with inversion. */
    channels[1].done = true;
    assert(motor_poll(103, &done) == ESP_OK && done);
#else
    assert(created == 0);
    assert(motor_poll(101, &done) == ESP_OK && !done);
    assert(motor_poll(100 + motion_duration_ms(&c, goal), &done) == ESP_OK && done);
#endif
    motor_position(position); assert(position[0] == 80 && position[1] == -40);
    assert(motor_start(goal, 2000) == ESP_OK);
    assert(motor_poll(2000, &done) == ESP_OK && done); /* Zero-length move. */
    goal[0] = goal[1] = 0;
    assert(motor_start(goal, 2001) == ESP_OK);
    assert(motor_stop() == ESP_OK && !motor_busy());
    assert(motor_poll(2002, &done) == ESP_OK && !done);
    assert(motor_arm() == ESP_OK && motor_start(goal, 2003) == ESP_ERR_INVALID_STATE);
    assert(motor_reference() == ESP_OK);
#if CONFIG_SORTER_MOTOR_HARDWARE
    fail_next_tx = true; goal[0] = 5;
    assert(motor_start(goal, 2004) == ESP_FAIL);
    assert(levels[15] == 1 && !motor_busy());
#endif
    puts("PASS motor backend: arming, reference, busy, pulses, sequential axes, completion, cancel, failure");
    return 0;
}
