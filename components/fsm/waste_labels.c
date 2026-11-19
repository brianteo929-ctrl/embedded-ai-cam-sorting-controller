#include "waste_labels.h"
#include <string.h>
#define WASTE_LABEL(label, bin) {label, bin},
const label_map_t waste_labels[] = {
#include "waste_labels.def"
};
#undef WASTE_LABEL
const size_t waste_label_count = sizeof(waste_labels) / sizeof(waste_labels[0]);
bool waste_label_known(const char *label)
{
    if (!label || !*label) return false;
    for (size_t i = 0; i < waste_label_count; ++i)
        if (!strcmp(label, waste_labels[i].label)) return true;
    return false;
}
