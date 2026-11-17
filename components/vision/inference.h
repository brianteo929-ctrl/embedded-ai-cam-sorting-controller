#pragma once
#include "decision.h"
#include "esp_err.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    MOCK_RECYCLABLE, MOCK_ORGANIC, MOCK_HAZARDOUS, MOCK_EMPTY, MOCK_LOW,
    MOCK_UNKNOWN, MOCK_MULTIPLE, MOCK_CONFLICT, MOCK_ERROR, MOCK_COUNT
} mock_case_t;
typedef struct { detection_t items[DETECTION_LIMIT]; size_t count; bool mocked; } inference_result_t;
void inference_dimensions(unsigned *width, unsigned *height);
esp_err_t inference_run(const uint8_t *rgb, unsigned width, unsigned height, mock_case_t scenario, inference_result_t *out);
const char *inference_backend(void);
#ifdef __cplusplus
}
#endif
