#pragma once
#include "model-parameters/model_metadata.h"
#include "waste_labels.h"
#include <cstring>

#if !defined(EI_CLASSIFIER_PROJECT_ID) || EI_CLASSIFIER_PROJECT_ID != 1168582
#error "Use the existing Edge Impulse project 1168582"
#endif
#if !defined(EI_CLASSIFIER_PROJECT_DEPLOY_VERSION)
#error "Export must declare its deployment version"
#endif
#if !defined(EI_CLASSIFIER_RESIZE_MODE) || !defined(EI_CLASSIFIER_RESIZE_FIT_SHORTEST)
#error "Export must declare resize mode"
#elif EI_CLASSIFIER_RESIZE_MODE != EI_CLASSIFIER_RESIZE_FIT_SHORTEST
#error "Firmware preprocessing requires Fit shortest axis"
#endif
#if EI_CLASSIFIER_INPUT_WIDTH < 1 || EI_CLASSIFIER_INPUT_WIDTH > 256 || EI_CLASSIFIER_INPUT_HEIGHT < 1 || EI_CLASSIFIER_INPUT_HEIGHT > 256
#error "Unsupported model image dimensions"
#endif
#if EI_CLASSIFIER_LABEL_COUNT < 2 || EI_CLASSIFIER_LABEL_COUNT > DETECTION_LIMIT
#error "Unsupported model label count"
#endif
#if EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE != EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT
#error "Expected packed RGB camera signal"
#endif
#if EI_CLASSIFIER_NN_INPUT_FRAME_SIZE != EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE && EI_CLASSIFIER_NN_INPUT_FRAME_SIZE != 3 * EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE
#error "Expected grayscale or RGB Image processing block"
#endif

#define SORTER_STRINGIFY_INNER(x) #x
#define SORTER_STRINGIFY(x) SORTER_STRINGIFY_INNER(x)
extern "C" const char *inference_model_version(void)
{
    return "1168582/" SORTER_STRINGIFY(EI_CLASSIFIER_PROJECT_DEPLOY_VERSION);
}
static bool ei_labels_valid(bool require_reject_labels)
{
    bool background = false, unknown = false, multiple = false;
    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; ++i) {
        const char *label = ei_classifier_inferencing_categories[i];
        if (!waste_label_known(label)) return false;
        for (size_t j = 0; j < i; ++j)
            if (!std::strcmp(label, ei_classifier_inferencing_categories[j])) return false;
        background |= !std::strcmp(label, "background");
        unknown |= !std::strcmp(label, "unknown");
        multiple |= !std::strcmp(label, "multiple_items");
    }
    return !require_reject_labels || (background && unknown && multiple);
}
