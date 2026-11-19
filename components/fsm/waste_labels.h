#pragma once
#include "decision.h"
#ifdef __cplusplus
extern "C" {
#endif
extern const label_map_t waste_labels[];
extern const size_t waste_label_count;
bool waste_label_known(const char *label);
#ifdef __cplusplus
}
#endif
