#include "impl.h"
#include <math.h>
#include <complex.h> // This is required to use float complex in the function signatures that the pdf requires

static float f_abs(float x) { return x < 0.0f ? -x : x; }

static unsigned escape_time_burning_ship(float cr, float ci, unsigned n)
{
    float RealPartz = 0.0f;
    float ImagPartz = 0.0f;
    for (unsigned k = 0; k < n; k++)
    {
        float aa = f_abs(RealPartz);
        float bb = f_abs(ImagPartz);
        float new_a = aa * aa - bb * bb + cr;
        float new_b = 2.0f * aa * bb + ci;
        RealPartz = new_a;
        ImagPartz = new_b;
        if (RealPartz * RealPartz + ImagPartz * ImagPartz > 4.0f)
        {
            return k;
        }
    }
    return n;
}

static void color_map(unsigned k, unsigned n,
                      unsigned char *r, unsigned char *g, unsigned char *b)
{
    if (k >= n)
    {
        *r = *g = *b = 0;
        return;
    }
    // Using the float in klammer so that the div is done in float
    float t = 1 - (float)k / (float)n;
    float rr = 9.4f * (1.0f - t) * t * t * t;
    float gg = 15.5f * (1.0f - t) * (1.0f - t) * t * t;
    float bb = 9.5f * (1.0f - t) * (1.0f - t) * (1.0f - t) * t;
    int Redc = (int)(255.0f * rr);
    int Greenc = (int)(255.0f * gg);
    int Bluec = (int)(255.0f * bb);
    // Color matching
    if (Redc > 255)
        Redc = 255;
    if (Redc < 0)
        Redc = 0;
    if (Greenc > 255)
        Greenc = 255;
    if (Greenc < 0)
        Greenc = 0;
    if (Bluec > 255)
        Bluec = 255;
    if (Bluec < 0)
        Bluec = 0;
    *r = (unsigned char)Redc;
    *g = (unsigned char)Greenc;
    *b = (unsigned char)Bluec;
}
// This V1 is only using C to finish the task
void burning_ship_V1(
    float complex start,
    size_t width, size_t height,
    float res,
    unsigned n,
    unsigned char *img)
{
    // Getting the realpart and the imagpart of the Complex number
    float start_real = crealf(start);
    float start_imag = cimagf(start);

    for (size_t y = 0; y < height; y++)
    {
        for (size_t x = 0; x < width; x++)
        {
            float Cr = start_real + (float)x * res;
            float Ci = start_imag + (float)y * res;
            unsigned k = escape_time_burning_ship(Cr, Ci, n);
            unsigned char r, g, b;
            color_map(k, n, &r, &g, &b);

            size_t indexp = (y * width + x) * 3;
            // The written order of BMP is firstBlue then Green then Red
            img[indexp + 0] = b;
            img[indexp + 1] = g;
            img[indexp + 2] = r;
        }
    }
}