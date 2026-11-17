#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"
enum { LEDC_TIMER_0, LEDC_CHANNEL_0, PIXFORMAT_JPEG, FRAMESIZE_QVGA, CAMERA_FB_IN_PSRAM, CAMERA_GRAB_WHEN_EMPTY };
typedef struct { uint8_t *buf; size_t len; unsigned width, height; int format; } camera_fb_t;
typedef struct {
    int pin_pwdn, pin_reset, pin_xclk, pin_sccb_sda, pin_sccb_scl;
    int pin_d0, pin_d1, pin_d2, pin_d3, pin_d4, pin_d5, pin_d6, pin_d7;
    int pin_vsync, pin_href, pin_pclk, xclk_freq_hz, ledc_timer, ledc_channel;
    int pixel_format, frame_size, jpeg_quality, fb_count, fb_location, grab_mode;
} camera_config_t;
esp_err_t esp_camera_init(const camera_config_t *);
camera_fb_t *esp_camera_fb_get(void);
void esp_camera_fb_return(camera_fb_t *);
