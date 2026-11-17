#include "image.h"
#include <stddef.h>
bool image_center_crop(const uint8_t *src, unsigned sw, unsigned sh,
                       uint8_t *dst, unsigned dw, unsigned dh)
{
    if (!src || !dst || src == dst || !sw || !sh || !dw || !dh ||
        sw > 640 || sh > 480 || dw > 256 || dh > 256) return false;
    unsigned cw = sw, ch = sh;
    if (sw * dh > sh * dw) cw = sh * dw / dh;
    else ch = sw * dh / dw;
    if (!cw || !ch) return false;
    unsigned ox = (sw - cw) / 2, oy = (sh - ch) / 2;
    for (unsigned y = 0; y < dh; ++y) {
        int fy = (int)((2 * y + 1) * ch * 256 / (2 * dh)) - 128;
        if (fy < 0) fy = 0;
        unsigned y0 = (unsigned)fy / 256, wy = (unsigned)fy % 256;
        unsigned y1 = y0 + 1 < ch ? y0 + 1 : y0;
        for (unsigned x = 0; x < dw; ++x) {
            int fx = (int)((2 * x + 1) * cw * 256 / (2 * dw)) - 128;
            if (fx < 0) fx = 0;
            unsigned x0 = (unsigned)fx / 256, wx = (unsigned)fx % 256;
            unsigned x1 = x0 + 1 < cw ? x0 + 1 : x0;
            for (unsigned c = 0; c < 3; ++c) {
                uint32_t a = src[((y0 + oy) * sw + x0 + ox) * 3 + c];
                uint32_t b = src[((y0 + oy) * sw + x1 + ox) * 3 + c];
                uint32_t d = src[((y1 + oy) * sw + x0 + ox) * 3 + c];
                uint32_t e = src[((y1 + oy) * sw + x1 + ox) * 3 + c];
                dst[(y * dw + x) * 3 + c] = (uint8_t)(((a * (256 - wx) + b * wx) * (256 - wy) +
                    (d * (256 - wx) + e * wx) * wy + 32768) / 65536);
            }
        }
    }
    return true;
}
