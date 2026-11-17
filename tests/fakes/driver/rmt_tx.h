#pragma once
#include "esp_err.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#define RMT_CLK_SRC_DEFAULT 0
typedef struct fake_channel *rmt_channel_handle_t;
typedef void *rmt_encoder_handle_t;
typedef struct { unsigned level0, duration0, level1, duration1; } rmt_symbol_word_t;
typedef struct { int gpio_num, clk_src; uint32_t resolution_hz; size_t mem_block_symbols, trans_queue_depth; } rmt_tx_channel_config_t;
typedef struct { struct { unsigned eot_level, queue_nonblocking; } flags; } rmt_transmit_config_t;
typedef struct { int unused; } rmt_copy_encoder_config_t;
esp_err_t rmt_new_tx_channel(const rmt_tx_channel_config_t *, rmt_channel_handle_t *);
esp_err_t rmt_new_copy_encoder(const rmt_copy_encoder_config_t *, rmt_encoder_handle_t *);
esp_err_t rmt_transmit(rmt_channel_handle_t, rmt_encoder_handle_t, const void *, size_t, const rmt_transmit_config_t *);
esp_err_t rmt_tx_wait_all_done(rmt_channel_handle_t, int);
esp_err_t rmt_enable(rmt_channel_handle_t);
esp_err_t rmt_disable(rmt_channel_handle_t);
esp_err_t rmt_del_channel(rmt_channel_handle_t);
esp_err_t rmt_encoder_reset(rmt_encoder_handle_t);
