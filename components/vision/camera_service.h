#pragma once
#include "esp_camera.h"
#include "esp_err.h"
esp_err_t camera_service_init(void);
bool camera_service_ready(void);
camera_fb_t *camera_acquire(unsigned timeout_ms);
void camera_release(camera_fb_t *frame);
esp_err_t camera_decode_rgb888(const camera_fb_t *frame, uint8_t *rgb, size_t capacity);
esp_err_t camera_jpeg_copy(uint8_t **data, size_t *length);
