#pragma once
#include <stddef.h>
#include <complex.h>

// According to the Pdf, the signature must used in the following format,this is the asm version
void burning_ship(
    float complex start,
    size_t width, size_t height,
    float res,
    unsigned n,
    unsigned char *img);

// Same as above, but this is the C version.
void burning_ship_V1(
    float complex start,
    size_t width, size_t height,
    float res,
    unsigned n,
    unsigned char *img);