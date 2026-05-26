#pragma once
#include <stddef.h>

int write_bmp24(const char *filename,
                const unsigned char *img_bgr,
                size_t width,
                size_t height);
