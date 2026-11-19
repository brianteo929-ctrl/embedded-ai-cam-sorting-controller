#include "inference.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "model-parameters/model_metadata.h"
#include "ei_image_contract.h"

#if !EI_CLASSIFIER_OBJECT_DETECTION || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_CAMERA
#error "Export a camera object-detection impulse; see docs/MODEL_INTEGRATION.md"
#endif
#if defined(EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER) && defined(EI_CLASSIFIER_LAST_LAYER_FOMO)
#if EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER != EI_CLASSIFIER_LAST_LAYER_FOMO
#error "This adapter requires FOMO output"
#endif
#endif

static const uint8_t *pixels;
extern "C" esp_err_t inference_validate(void) { return ei_labels_valid(false) ? ESP_OK : ESP_ERR_INVALID_ARG; }
static int get_data(size_t offset, size_t length, float *out)
{
    const size_t count = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    if (!pixels || !out || offset > count || length > count - offset) return -1;
    for (size_t i = 0; i < length; ++i) {
        const uint8_t *p = pixels + 3 * (offset + i);
        out[i] = static_cast<float>((uint32_t(p[0]) << 16) | (uint32_t(p[1]) << 8) | p[2]);
    }
    return 0;
}
extern "C" void inference_dimensions(unsigned *width, unsigned *height)
{
    *width = EI_CLASSIFIER_INPUT_WIDTH; *height = EI_CLASSIFIER_INPUT_HEIGHT;
}
extern "C" const char *inference_backend(void) { return "edge_impulse_fomo"; }
extern "C" esp_err_t inference_run(const uint8_t *rgb, unsigned width, unsigned height,
                                    mock_case_t scenario, inference_result_t *out)
{
    (void)scenario;
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = {};
    if (!rgb || width != EI_CLASSIFIER_INPUT_WIDTH || height != EI_CLASSIFIER_INPUT_HEIGHT ||
        inference_validate() != ESP_OK) return ESP_ERR_INVALID_ARG;
    pixels = rgb;
    ei::signal_t signal;
    signal.total_length = width * height;
    signal.get_data = get_data;
    ei_impulse_result_t result = {};
    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false);
    pixels = nullptr;
    if (err != EI_IMPULSE_OK) return ESP_FAIL;
    if (result.bounding_boxes_count && !result.bounding_boxes) return ESP_FAIL;
    for (size_t i = 0; i < result.bounding_boxes_count; ++i) {
        const auto &box = result.bounding_boxes[i];
        if (box.value == 0) continue;
        if (out->count == DETECTION_LIMIT) return ESP_ERR_INVALID_SIZE;
        out->items[out->count++] = {box.label, box.value};
    }
    return ESP_OK;
}
