#include "inference.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include <cassert>
#include <cstdio>
#include <cstring>
const char *ei_classifier_inferencing_categories[] = {"plastic_bottle", "background", "unknown", "multiple_items"};
static bool fail, bad_label;
static int (*saved_callback)(size_t, size_t, float *);
EI_IMPULSE_ERROR run_classifier(ei::signal_t *signal, ei_impulse_result_t *result, bool debug)
{
    assert(!debug && signal->total_length == 4);
    float values[4];
    assert(signal->get_data(0, 4, values) == 0);
    assert(values[0] == 0xFF0000 && values[1] == 0x00FF00 && values[2] == 0x0000FF && values[3] == 0xFFFFFF);
    assert(signal->get_data(4, 1, values) != 0 && signal->get_data(5, 0, values) != 0);
    assert(signal->get_data(0, 1, nullptr) != 0);
    saved_callback = signal->get_data;
    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; ++i)
        result->classification[i] = {ei_classifier_inferencing_categories[i], i ? 0.0f : 1.0f};
    if (bad_label) result->classification[0].label = "unmapped";
    return fail ? EI_IMPULSE_ERROR_TEST : EI_IMPULSE_OK;
}
int main()
{
    const uint8_t rgb[] = {255,0,0, 0,255,0, 0,0,255, 255,255,255};
    inference_result_t result;
    unsigned w, h; inference_dimensions(&w, &h); assert(w == 2 && h == 2);
    assert(inference_validate() == ESP_OK);
    assert(!std::strcmp(inference_model_version(), "1168582/1"));
    assert(inference_run(rgb, w, h, MOCK_EMPTY, &result) == ESP_OK);
    assert(!result.mocked && result.output == INFERENCE_CLASSIFICATION && result.count == 4);
    assert(result.items[0].confidence == 1);
    float value; assert(saved_callback(0, 1, &value) != 0);
    fail = true; assert(inference_run(rgb, w, h, MOCK_EMPTY, &result) == ESP_FAIL && result.count == 0);
    assert(saved_callback(0, 1, &value) != 0);
    fail = false; bad_label = true;
    assert(inference_run(rgb, w, h, MOCK_EMPTY, &result) == ESP_FAIL && result.count == 0);
    assert(inference_run(rgb, 3, h, MOCK_EMPTY, &result) == ESP_ERR_INVALID_ARG);
    ei_classifier_inferencing_categories[1] = "battery";
    assert(inference_validate() == ESP_ERR_INVALID_ARG);
    ei_classifier_inferencing_categories[1] = "unmapped";
    assert(inference_validate() == ESP_ERR_INVALID_ARG);
    ei_classifier_inferencing_categories[1] = "plastic_bottle";
    assert(inference_validate() == ESP_ERR_INVALID_ARG);
    puts("PASS EI classification contract with fake SDK: RGB, lifetime, metadata labels, output, errors (not real inference)");
}
