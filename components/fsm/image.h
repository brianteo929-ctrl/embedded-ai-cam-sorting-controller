#pragma once
#include <stdbool.h>
#include <stdint.h>
/* Separate RGB888 buffers; caller supplies width * height * 3 bytes each. */
bool image_center_crop(const uint8_t *src, unsigned sw, unsigned sh,
                       uint8_t *dst, unsigned dw, unsigned dh);
