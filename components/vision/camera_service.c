#include "camera_service.h"
#include "sdkconfig.h"
#include "esp_heap_caps.h"
#include "esp_psram.h"
#include "jpeg_decoder.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

static SemaphoreHandle_t lock;
static bool ready;
esp_err_t camera_service_init(void)
{
#if CONFIG_SORTER_CAMERA_MOCK
    ready = true;
    return ESP_OK;
#else
    if (!esp_psram_is_initialized()) return ESP_ERR_NO_MEM;
    lock = xSemaphoreCreateMutex();
    if (!lock) return ESP_ERR_NO_MEM;
    camera_config_t config = {
        .pin_pwdn = 32, .pin_reset = -1, .pin_xclk = 0,
        .pin_sccb_sda = 26, .pin_sccb_scl = 27,
        .pin_d0 = 5, .pin_d1 = 18, .pin_d2 = 19, .pin_d3 = 21,
        .pin_d4 = 36, .pin_d5 = 39, .pin_d6 = 34, .pin_d7 = 35,
        .pin_vsync = 25, .pin_href = 23, .pin_pclk = 22,
        .xclk_freq_hz = 20000000, .ledc_timer = LEDC_TIMER_0, .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_JPEG, .frame_size = FRAMESIZE_QVGA, .jpeg_quality = 12,
        .fb_count = 1, .fb_location = CAMERA_FB_IN_PSRAM, .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    };
    esp_err_t err = esp_camera_init(&config);
    ready = err == ESP_OK;
    return err;
#endif
}
bool camera_service_ready(void) { return ready; }
camera_fb_t *camera_acquire(unsigned timeout_ms)
{
    if (!ready || !lock || xSemaphoreTake(lock, pdMS_TO_TICKS(timeout_ms)) != pdTRUE) return NULL;
    camera_fb_t *frame = esp_camera_fb_get();
    /* A one-buffer camera can have a queued image from before the trigger. */
    if (frame) {
        esp_camera_fb_return(frame);
        frame = esp_camera_fb_get();
    }
    if (!frame) xSemaphoreGive(lock);
    return frame;
}
void camera_release(camera_fb_t *frame)
{
    if (frame) { esp_camera_fb_return(frame); xSemaphoreGive(lock); }
}
esp_err_t camera_decode_rgb888(const camera_fb_t *frame, uint8_t *rgb, size_t capacity)
{
    static uint8_t work[3100]; /* Single vision caller; no per-frame scratch allocation. */
    if (!frame || !rgb || frame->format != PIXFORMAT_JPEG || !frame->buf || !frame->len ||
        frame->len > 128 * 1024 || !frame->width || !frame->height || frame->width > 320 || frame->height > 240 ||
        capacity < frame->width * frame->height * 3 || capacity > 320 * 240 * 3) return ESP_ERR_INVALID_ARG;
    esp_jpeg_image_cfg_t config = {.indata = frame->buf, .indata_size = frame->len, .outbuf = rgb,
        .outbuf_size = capacity, .out_format = JPEG_IMAGE_FORMAT_RGB888, .out_scale = JPEG_IMAGE_SCALE_0,
        .advanced.working_buffer = work, .advanced.working_buffer_size = sizeof(work)};
    esp_jpeg_image_output_t info = {0};
    esp_err_t err = esp_jpeg_decode(&config, &info);
    if (err != ESP_OK) return err;
    return info.width == frame->width && info.height == frame->height &&
        info.output_len == frame->width * frame->height * 3 ? ESP_OK : ESP_ERR_INVALID_SIZE;
}
esp_err_t camera_jpeg_copy(uint8_t **data, size_t *length)
{
    if (!data || !length) return ESP_ERR_INVALID_ARG;
    *data = NULL; *length = 0;
    camera_fb_t *frame = camera_acquire(100);
    if (!frame) return ESP_ERR_TIMEOUT;
    esp_err_t err = ESP_ERR_INVALID_SIZE;
    if (frame->format == PIXFORMAT_JPEG && frame->len > 0 && frame->len <= 128 * 1024) {
        *data = heap_caps_malloc(frame->len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (*data) { memcpy(*data, frame->buf, frame->len); *length = frame->len; err = ESP_OK; }
        else err = ESP_ERR_NO_MEM;
    }
    camera_release(frame);
    return err;
}
