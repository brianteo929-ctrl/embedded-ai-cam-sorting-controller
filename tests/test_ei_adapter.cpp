#include "inference.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include <cassert>
#include <cstdio>
static ei_impulse_result_bounding_box_t boxes[17];
static size_t count;
static bool fail;
EI_IMPULSE_ERROR run_classifier(ei::signal_t *signal, ei_impulse_result_t *result, bool debug)
{
    assert(!debug && signal->total_length == 4);
    float values[4];
    assert(signal->get_data(0, 4, values) == 0);
    assert(values[0] == 0xFF0000 && values[1] == 0x00FF00 && values[2] == 0x0000FF && values[3] == 0xFFFFFF);
    assert(signal->get_data(4, 1, values) != 0 && signal->get_data(5, 0, values) != 0);
    result->bounding_boxes = boxes; result->bounding_boxes_count = count;
    return fail ? EI_IMPULSE_ERROR_TEST : EI_IMPULSE_OK;
}
int main()
{
    const uint8_t rgb[] = {255,0,0, 0,255,0, 0,0,255, 255,255,255};
    inference_result_t result;
    unsigned w, h; inference_dimensions(&w, &h); assert(w == 2 && h == 2);
    assert(inference_run(rgb, w, h, MOCK_EMPTY, &result) == ESP_OK && !result.mocked && result.count == 0);
    boxes[0] = {"recyclable", .9f}; boxes[1] = {"unused", 0}; count = 2;
    assert(inference_run(rgb, w, h, MOCK_EMPTY, &result) == ESP_OK && result.count == 1);
    assert(result.items[0].confidence == .9f);
    for (auto &box : boxes) box = {"recyclable", .9f};
    count = 17;
    assert(inference_run(rgb, w, h, MOCK_EMPTY, &result) == ESP_ERR_INVALID_SIZE);
    fail = true;
    assert(inference_run(rgb, w, h, MOCK_EMPTY, &result) == ESP_FAIL);
    assert(inference_run(rgb, 3, h, MOCK_EMPTY, &result) == ESP_ERR_INVALID_ARG);
    std::puts("PASS EI adapter contract with fake SDK: packed RGB, bounds, boxes, padding, overflow, errors (not model validation)");
}
