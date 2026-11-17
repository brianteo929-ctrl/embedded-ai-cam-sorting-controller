#pragma once
#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"
enum { JPEG_IMAGE_FORMAT_RGB888, JPEG_IMAGE_SCALE_0 };
typedef struct {
    uint8_t *indata, *outbuf;
    size_t indata_size, outbuf_size;
    int out_format, out_scale;
    struct { void *working_buffer; size_t working_buffer_size; } advanced;
} esp_jpeg_image_cfg_t;
typedef struct { unsigned width, height; size_t output_len; } esp_jpeg_image_output_t;
esp_err_t esp_jpeg_decode(esp_jpeg_image_cfg_t *, esp_jpeg_image_output_t *);
