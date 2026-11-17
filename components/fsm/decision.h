#pragma once
#include <stdbool.h>
#include <stddef.h>
#define DETECTION_LIMIT 16
typedef enum { BIN_NONE, BIN_RECYCLABLE, BIN_ORGANIC, BIN_HAZARDOUS } waste_bin_t;
typedef struct { const char *label; float confidence; } detection_t;
typedef struct { const char *label; waste_bin_t bin; } label_map_t;
typedef enum {
    DECISION_ACCEPT, DECISION_EMPTY, DECISION_LOW_CONFIDENCE, DECISION_UNKNOWN,
    DECISION_MULTIPLE, DECISION_CONFLICT, DECISION_INVALID, DECISION_MODEL_ERROR
} decision_reason_t;
typedef struct { waste_bin_t bin; decision_reason_t reason; float confidence; } decision_t;
decision_t decide(const detection_t *items, size_t count, bool model_ok,
                  const label_map_t *map, size_t map_count, float threshold);
const char *bin_name(waste_bin_t bin);
const char *decision_name(decision_reason_t reason);
