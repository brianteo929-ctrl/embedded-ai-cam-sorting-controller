#pragma once
#include <stddef.h>
#include "model-parameters/model_metadata.h"
namespace ei { struct signal_t { size_t total_length; int (*get_data)(size_t, size_t, float *); }; }
struct ei_impulse_result_bounding_box_t { const char *label; float value; };
struct ei_impulse_result_classification_t { const char *label; float value; };
struct ei_impulse_result_t {
    size_t bounding_boxes_count;
    ei_impulse_result_bounding_box_t *bounding_boxes;
    ei_impulse_result_classification_t classification[EI_CLASSIFIER_LABEL_COUNT];
};
enum EI_IMPULSE_ERROR { EI_IMPULSE_OK, EI_IMPULSE_ERROR_TEST };
EI_IMPULSE_ERROR run_classifier(ei::signal_t *, ei_impulse_result_t *, bool);
