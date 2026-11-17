#include "inference.h"
#include "sdkconfig.h"
#if CONFIG_SORTER_INFERENCE_MOCK
void inference_dimensions(unsigned *width, unsigned *height) { *width = 96; *height = 96; }
const char *inference_backend(void) { return "mock"; }
esp_err_t inference_run(const uint8_t *rgb, unsigned width, unsigned height, mock_case_t scenario, inference_result_t *out)
{
    if (!out || !rgb || width != 96 || height != 96 || (unsigned)scenario >= MOCK_COUNT) return ESP_ERR_INVALID_ARG;
    *out = (inference_result_t){.mocked = true};
    if (scenario == MOCK_ERROR) return ESP_FAIL;
    if (scenario == MOCK_EMPTY) return ESP_OK;
    out->count = 1;
    out->items[0] = (detection_t){CONFIG_SORTER_LABEL_RECYCLABLE, .95f};
    if (scenario == MOCK_ORGANIC) out->items[0].label = CONFIG_SORTER_LABEL_ORGANIC;
    if (scenario == MOCK_HAZARDOUS) out->items[0].label = CONFIG_SORTER_LABEL_HAZARDOUS;
    if (scenario == MOCK_LOW) out->items[0].confidence = .01f;
    if (scenario == MOCK_UNKNOWN) out->items[0].label = "__unmapped__";
    if (scenario == MOCK_MULTIPLE || scenario == MOCK_CONFLICT) {
        out->count = 2; out->items[1] = out->items[0];
        if (scenario == MOCK_CONFLICT) out->items[1].label = CONFIG_SORTER_LABEL_ORGANIC;
    }
    return ESP_OK;
}
#endif
