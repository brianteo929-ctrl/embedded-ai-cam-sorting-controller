#include "inference.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    uint8_t image[96 * 96 * 3] = {0};
    const label_map_t labels[] = {{"recyclable", BIN_RECYCLABLE}, {"organic", BIN_ORGANIC}, {"hazardous", BIN_HAZARDOUS}};
    const decision_reason_t expected[] = {DECISION_ACCEPT, DECISION_ACCEPT, DECISION_ACCEPT, DECISION_EMPTY,
        DECISION_LOW_CONFIDENCE, DECISION_UNKNOWN, DECISION_MULTIPLE, DECISION_CONFLICT, DECISION_MODEL_ERROR};
    unsigned w, h; inference_dimensions(&w, &h); assert(w == 96 && h == 96);
    for (unsigned i = 0; i < MOCK_COUNT; ++i) {
        inference_result_t result;
        esp_err_t err = inference_run(image, w, h, (mock_case_t)i, &result);
        decision_t d = decide(result.items, result.count, err == ESP_OK, labels, 3, .8f);
        assert(result.mocked && d.reason == expected[i]);
        if (i < 3) assert(d.bin == (waste_bin_t)(i + 1));
    }
    inference_result_t result;
    assert(inference_run(NULL, w, h, MOCK_EMPTY, &result) == ESP_ERR_INVALID_ARG);
    assert(inference_run(image, 0, h, MOCK_EMPTY, &result) == ESP_ERR_INVALID_ARG);
    puts("PASS mock inference: all nine deterministic scenarios feed actual decision policy");
    return 0;
}
