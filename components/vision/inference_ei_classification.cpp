#include "inference.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "ei_image_contract.h"

#if EI_CLASSIFIER_OBJECT_DETECTION || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_CAMERA
#error "Classification adapter requires a camera classification export"
#endif
#if EI_CLASSIFIER_HAS_ANOMALY || EI_CLASSIFIER_FREEFORM_OUTPUT
#error "This adapter requires class probabilities without additional output heads"
#endif

static const uint8_t *pixels;
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
extern "C" esp_err_t inference_validate(void) { return ei_labels_valid(true) ? ESP_OK : ESP_ERR_INVALID_ARG; }
extern "C" void inference_dimensions(unsigned *width, unsigned *height)
{
    *width = EI_CLASSIFIER_INPUT_WIDTH; *height = EI_CLASSIFIER_INPUT_HEIGHT;
}
extern "C" const char *inference_backend(void) { return "edge_impulse_classification"; }
extern "C" esp_err_t inference_run(const uint8_t *rgb, unsigned width, unsigned height,
                                  mock_case_t scenario, inference_result_t *out)
{
    (void)scenario;
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = {};
    out->output = INFERENCE_CLASSIFICATION;
    if (!rgb || width != EI_CLASSIFIER_INPUT_WIDTH || height != EI_CLASSIFIER_INPUT_HEIGHT ||
        inference_validate() != ESP_OK) return ESP_ERR_INVALID_ARG;
    pixels = rgb;
    ei::signal_t signal;
    signal.total_length = width * height;
    signal.get_data = get_data;
    ei_impulse_result_t result = {};
    const EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false);
    pixels = nullptr;
    if (err != EI_IMPULSE_OK) return ESP_FAIL;
    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; ++i) {
        if (!result.classification[i].label ||
            std::strcmp(result.classification[i].label, ei_classifier_inferencing_categories[i])) return ESP_FAIL;
        out->items[i] = {result.classification[i].label, result.classification[i].value};
    }
    out->count = EI_CLASSIFIER_LABEL_COUNT;
    return ESP_OK;
}
