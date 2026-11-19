#include "inference.h"
#include "waste_labels.h"
#include "sdkconfig.h"
decision_t inference_decide(const inference_result_t *result, bool model_ok, float threshold, float margin)
{
    static const label_map_t mock_labels[] = {
        {CONFIG_SORTER_LABEL_RECYCLABLE, BIN_RECYCLABLE},
        {CONFIG_SORTER_LABEL_ORGANIC, BIN_ORGANIC},
        {CONFIG_SORTER_LABEL_HAZARDOUS, BIN_HAZARDOUS},
    };
    if (!result) return (decision_t){BIN_NONE, DECISION_INVALID, 0};
    const label_map_t *map = result->mocked ? mock_labels : waste_labels;
    const size_t count = result->mocked ? sizeof(mock_labels)/sizeof(mock_labels[0]) : waste_label_count;
    if (result->output == INFERENCE_CLASSIFICATION)
        return decide_classification(result->items, result->count, model_ok, map, count, threshold, margin);
    if (result->output != INFERENCE_DETECTIONS) return (decision_t){BIN_NONE, DECISION_INVALID, 0};
    return decide(result->items, result->count, model_ok, map, count, threshold);
}
