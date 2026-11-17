#include "camera_service.h"
#include "esp_heap_caps.h"
#include "freertos/semphr.h"
#include "jpeg_decoder.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static bool locked, allocation_failure;
static bool bad_dimensions, decode_error;
static unsigned acquisitions, returns, fail_get;
static uint8_t pixels[] = {1, 2, 3, 4};
static camera_fb_t frame = {.buf = pixels, .len = sizeof(pixels), .format = PIXFORMAT_JPEG};
bool esp_psram_is_initialized(void) { return true; }
SemaphoreHandle_t xSemaphoreCreateMutex(void) { return &locked; }
int xSemaphoreTake(SemaphoreHandle_t s, unsigned timeout) { (void)s; (void)timeout; if (locked) return 0; locked = true; return 1; }
int xSemaphoreGive(SemaphoreHandle_t s) { (void)s; assert(locked); locked = false; return 1; }
esp_err_t esp_camera_init(const camera_config_t *c)
{
    assert(c->fb_count == 1 && c->pin_xclk == 0 && c->pin_pwdn == 32 && c->pin_d0 == 5);
    return ESP_OK;
}
camera_fb_t *esp_camera_fb_get(void) { assert(locked); ++acquisitions; return acquisitions == fail_get ? NULL : &frame; }
void esp_camera_fb_return(camera_fb_t *f) { assert(locked && f == &frame); ++returns; }
void *heap_caps_malloc(size_t n, unsigned caps) { (void)caps; return allocation_failure ? NULL : malloc(n); }
esp_err_t esp_jpeg_decode(esp_jpeg_image_cfg_t *c, esp_jpeg_image_output_t *out)
{
    assert(c->outbuf_size == 12 && c->advanced.working_buffer_size >= 3100);
    if (decode_error) return ESP_FAIL;
    *out = (esp_jpeg_image_output_t){.width = bad_dimensions ? 1 : 2, .height = 2, .output_len = 12};
    return ESP_OK;
}
int main(void)
{
    assert(!camera_service_ready() && camera_service_init() == ESP_OK);
    camera_fb_t *f = camera_acquire(100);
    assert(f == &frame && acquisitions == 2 && returns == 1 && locked);
    assert(camera_acquire(0) == NULL && acquisitions == 2);
    camera_release(f); assert(returns == 2 && !locked);
    fail_get = acquisitions + 1;
    assert(camera_acquire(100) == NULL && !locked && returns == 2);
    fail_get = acquisitions + 2;
    assert(camera_acquire(100) == NULL && !locked && returns == 3);
    fail_get = 0;
    uint8_t *copy; size_t length;
    assert(camera_jpeg_copy(&copy, &length) == ESP_OK && length == 4 && !locked);
    assert(copy != frame.buf && !memcmp(copy, frame.buf, length)); free(copy);
    allocation_failure = true;
    assert(camera_jpeg_copy(&copy, &length) == ESP_ERR_NO_MEM && !copy && !locked);
    allocation_failure = false; frame.len = 128 * 1024 + 1;
    assert(camera_jpeg_copy(&copy, &length) == ESP_ERR_INVALID_SIZE && !copy && !locked);
    assert(camera_jpeg_copy(NULL, &length) == ESP_ERR_INVALID_ARG);
    frame.len = 4; frame.width = frame.height = 2; uint8_t rgb[12];
    assert(camera_decode_rgb888(&frame, rgb, sizeof(rgb)) == ESP_OK);
    assert(camera_decode_rgb888(&frame, rgb, 11) == ESP_ERR_INVALID_ARG);
    bad_dimensions = true;
    assert(camera_decode_rgb888(&frame, rgb, 12) == ESP_ERR_INVALID_SIZE);
    decode_error = true;
    assert(camera_decode_rgb888(&frame, rgb, 12) == ESP_FAIL);
    puts("PASS camera ownership: stale discard, exclusion, get failures, independent copy, bounds, allocation failure");
    return 0;
}
