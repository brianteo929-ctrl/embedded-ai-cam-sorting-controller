#include "decision.h"
#include "waste_labels.h"
#include "inference.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    detection_t p[] = {{"plastic_bottle", .8f}, {"background", .2f}};
#define PICK() decide_classification(p, 2, true, waste_labels, waste_label_count, .8f, .1f)
    for (size_t i = 0; i < waste_label_count; ++i) {
        assert(waste_label_known(waste_labels[i].label));
        p[0].label = waste_labels[i].label;
        p[1].label = strcmp(p[0].label, "background") ? "background" : "unknown";
        decision_t d = PICK();
        assert(d.bin == waste_labels[i].bin);
        assert(d.reason == (d.bin == BIN_NONE ? DECISION_UNKNOWN : DECISION_ACCEPT));
    }
    p[0].label = "plastic_bottle"; p[1].label = "background";
    assert(PICK().reason == DECISION_ACCEPT);
    p[0].confidence = nextafterf(.8f, 0); assert(PICK().reason == DECISION_LOW_CONFIDENCE);
    p[0].confidence = p[1].confidence = .5f; assert(PICK().reason == DECISION_AMBIGUOUS);
    p[0].confidence = .54f; p[1].confidence = .46f; assert(PICK().reason == DECISION_AMBIGUOUS);
    p[0].confidence = .9f; p[1].confidence = .1f;
    p[1].label = "unmapped"; assert(PICK().reason == DECISION_UNKNOWN);
    p[1].label = p[0].label; assert(PICK().reason == DECISION_INVALID);
    p[1].label = "battery"; assert(PICK().bin == BIN_RECYCLABLE);
    p[0].label = NULL; assert(PICK().reason == DECISION_INVALID);
    p[0].label = ""; assert(PICK().reason == DECISION_INVALID);
    p[0].label = "plastic_bottle";
    const float bad[] = {NAN, INFINITY, -INFINITY, -.1f, 1.1f, .2f};
    for (size_t i = 0; i < sizeof(bad)/sizeof(bad[0]); ++i) {
        p[0].confidence = bad[i]; assert(PICK().reason == DECISION_INVALID);
    }
    assert(decide_classification(NULL, 0, true, waste_labels, waste_label_count, .8f, .1f).reason == DECISION_EMPTY);
    assert(decide_classification(p, 2, false, waste_labels, waste_label_count, .8f, .1f).reason == DECISION_MODEL_ERROR);
    assert(decide_classification(p, 2, true, waste_labels, waste_label_count, .8f, NAN).reason == DECISION_INVALID);
    assert(!waste_label_known(NULL) && !waste_label_known("plastic") && !waste_label_known("Plastic_bottle"));
    p[0] = (detection_t){"plastic_bottle", .75f}; p[1] = (detection_t){"battery", .25f};
    assert(decide_classification(p, 2, true, waste_labels, waste_label_count, .75f, .5f).reason == DECISION_ACCEPT);
    assert(decide_classification(p, 2, true, waste_labels, waste_label_count, .75f, nextafterf(.5f, 1)).reason == DECISION_AMBIGUOUS);
    inference_result_t result = {.count = 2, .output = INFERENCE_CLASSIFICATION};
    result.items[0] = p[0]; result.items[1] = p[1];
    assert(inference_decide(&result, true, .75f, .5f).bin == BIN_RECYCLABLE);
    assert(inference_decide(&result, false, .75f, .5f).reason == DECISION_MODEL_ERROR);
    result.output = INFERENCE_DETECTIONS;
    assert(inference_decide(&result, true, .75f, .5f).reason == DECISION_CONFLICT);
    result.count = 1; result.mocked = true; result.items[0] = (detection_t){"organic", .95f};
    assert(inference_decide(&result, true, .8f, .1f).bin == BIN_ORGANIC);
    result.mocked = false;
    assert(inference_decide(&result, true, .8f, .1f).reason == DECISION_UNKNOWN);
    puts("PASS classification policy: canonical labels, reject classes, boundaries, ties, margins, malformed probabilities");
}
