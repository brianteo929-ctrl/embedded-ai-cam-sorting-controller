#include "decision.h"
#include <math.h>
#include <string.h>

decision_t decide(const detection_t *items, size_t count, bool model_ok,
                  const label_map_t *map, size_t map_count, float threshold)
{
    decision_t out = {BIN_NONE, DECISION_INVALID, 0};
    if (!model_ok) { out.reason = DECISION_MODEL_ERROR; return out; }
    if (!isfinite(threshold) || threshold <= 0 || threshold > 1 || !map || !map_count ||
        count > DETECTION_LIMIT || (count && !items)) return out;
    for (size_t i = 0; i < map_count; ++i) {
        if (!map[i].label || !*map[i].label || map[i].bin < BIN_NONE || map[i].bin > BIN_HAZARDOUS) return out;
        for (size_t j = 0; j < i; ++j)
            if (!strcmp(map[i].label, map[j].label)) return out;
    }
    if (!count) { out.reason = DECISION_EMPTY; return out; }
    bool low = false, unknown = false, conflict = false;
    waste_bin_t selected = BIN_NONE;
    for (size_t i = 0; i < count; ++i) {
        if (!items[i].label || !isfinite(items[i].confidence) || items[i].confidence < 0 || items[i].confidence > 1) return out;
        low |= items[i].confidence < threshold;
        waste_bin_t bin = BIN_NONE;
        for (size_t j = 0; j < map_count; ++j)
            if (!strcmp(items[i].label, map[j].label)) bin = map[j].bin;
        unknown |= bin == BIN_NONE;
        if (selected != BIN_NONE && bin != BIN_NONE && bin != selected) conflict = true;
        if (bin != BIN_NONE) selected = bin;
    }
    if (unknown) out.reason = DECISION_UNKNOWN;
    else if (conflict) out.reason = DECISION_CONFLICT;
    else if (count != 1) out.reason = DECISION_MULTIPLE;
    else if (low) out.reason = DECISION_LOW_CONFIDENCE;
    else { out.bin = selected; out.reason = DECISION_ACCEPT; out.confidence = items[0].confidence; }
    return out;
}
decision_t decide_classification(const detection_t *scores, size_t count, bool model_ok,
                                const label_map_t *map, size_t map_count,
                                float threshold, float minimum_margin)
{
    decision_t out = decide(NULL, 0, model_ok, map, map_count, threshold);
    if (out.reason != DECISION_EMPTY) return out;
    out.reason = DECISION_INVALID;
    if (!isfinite(minimum_margin) || minimum_margin < 0 || minimum_margin > 1 ||
        count > DETECTION_LIMIT || (count && !scores)) return out;
    if (!count) { out.reason = DECISION_EMPTY; return out; }
    if (count < 2) return out;
    float sum = 0, best = -1, runner_up = -1;
    size_t winner = 0;
    for (size_t i = 0; i < count; ++i) {
        if (!scores[i].label || !*scores[i].label || !isfinite(scores[i].confidence) ||
            scores[i].confidence < 0 || scores[i].confidence > 1) return out;
        for (size_t j = 0; j < i; ++j)
            if (!strcmp(scores[i].label, scores[j].label)) return out;
        bool known = false;
        for (size_t j = 0; j < map_count; ++j)
            known |= !strcmp(scores[i].label, map[j].label);
        if (!known) { out.reason = DECISION_UNKNOWN; return out; }
        const float p = scores[i].confidence;
        sum += p;
        if (p > best) { runner_up = best; best = p; winner = i; }
        else if (p > runner_up) runner_up = p;
    }
    /* Allow rounding of quantized softmax scores, not logits or multilabel output. */
    if (fabsf(sum - 1.0f) > .02f) return out;
    if (best == runner_up || best - runner_up < minimum_margin) {
        out.reason = DECISION_AMBIGUOUS; return out;
    }
    return decide(&scores[winner], 1, true, map, map_count, threshold);
}
const char *bin_name(waste_bin_t bin)
{
    static const char *names[] = {"none", "recyclable", "organic", "hazardous"};
    return (unsigned)bin < 4 ? names[bin] : "invalid";
}
const char *decision_name(decision_reason_t reason)
{
    static const char *names[] = {"accept", "empty", "low_confidence", "unknown_label", "multiple_objects", "conflicting_categories", "invalid_result", "model_error", "ambiguous_scores"};
    return (unsigned)reason < 9 ? names[reason] : "invalid";
}
