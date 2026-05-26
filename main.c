#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <time.h>
#include <complex.h>
#include "bmp.h"
#include "impl.h"
typedef struct
{
    int impl;            // This is -V option to choose between c or ASM
    int bench_enabled;   // This is -B option to enable Benchmark or not
    unsigned bench_reps; // This is -B option to choose how many repetitions in Benchmark
    float start_real;    // This is the Real part of Z
    float start_imag;    // This is the Imaginary part of Z
    size_t width;        // the width
    size_t height;       // the height
    unsigned n;          // This is the number of max iterations in the loop
    float res;           // Resolution
    const char *out;     // Output filename
} Params;
static void print_help(const char *prog)
{
    printf("Help Message\n");
    printf("Usage: %s [options]\n\n", prog);
    printf("Options:\n");
    printf("  -V <Num>                Implementation to use (0=ASM/Main, 1=C Reference) [default: 0]\n");
    printf("  -B[Num]                 Benchmark: measure runtime; optional reps (e.g. -B or -B5)\n");
    printf("  -s <real>,<imag>        Start point in complex plane (e.g. -2.0,-2.0)\n");
    printf("  -d <width>,<height>     Image dimensions in pixels (e.g. 800,600)\n");
    printf("  -n <Num>                Max iterations per pixel\n");
    printf("  -r <Float>              Step size per pixel\n");
    printf("  -o <Filename>           Output BMP filename\n");
    printf("  -h, --help              Show this help and exit\n");
}
// This is for parsing "float,float" format strings, restricting the user input
static int parse_two_floats(const char *s, float *a, float *b)
{
    char *end = NULL;
    errno = 0;
    float x = strtof(s, &end);
    if (errno || end == s || *end != ',')
        return 0;
    const char *s2 = end + 1;
    errno = 0;
    float y = strtof(s2, &end);
    if (errno || end == s2 || *end != '\0')
        return 0;
    *a = x;
    *b = y;
    return 1;
}
// Same as above, but for size_t, also restricting user input
static int parse_two_sizes(const char *s, size_t *a, size_t *b)
{
    char *end = NULL;
    errno = 0;
    unsigned long x = strtoul(s, &end, 10);
    if (errno || end == s || *end != ',')
        return 0;
    const char *s2 = end + 1;
    errno = 0;
    unsigned long y = strtoul(s2, &end, 10);
    if (errno || end == s2 || *end != '\0')
        return 0;
    *a = (size_t)x;
    *b = (size_t)y;
    return 1;
}
// Still same as above, but for unsigned int, restricting user input
static unsigned parse_unsigned_or_fail(const char *optname, const char *s, int *ok)
{
    char *end = NULL;
    errno = 0;
    unsigned long v = strtoul(s, &end, 10);
    if (errno || end == s || *end != '\0')
    {
        fprintf(stderr, "Error: It is an invalid value for %s: '%s'\n", optname, s);
        *ok = 0;
        return 0;
    }
    *ok = 1;
    return (unsigned)v;
}
// Failure parsing float value, restricting user input
static float parse_float_or_fail(const char *optname, const char *s, int *ok)
{
    char *end = NULL;
    errno = 0;
    float v = strtof(s, &end);
    if (errno || end == s || *end != '\0')
    {
        fprintf(stderr, "Error: It is an invalid value for %s: '%s'\n", optname, s);
        *ok = 0;
        return 0.0f;
    }
    *ok = 1;
    return v;
}
// Getting current time in Nanoseconds for the Benchmark
static long long now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + (long long)ts.tv_nsec;
}

int main(int argc, char **argv)
{
    Params p = {
        .impl = 0, // As default use V0 to first implement the ASM version, but we will use C version now.
        .bench_enabled = 0,
        .bench_reps = 1,
        .start_real = -2.0f,
        .start_imag = -2.0f,
        .width = 800,
        .height = 600,
        .n = 200,
        .res = 0.005f,
        .out = "burning_ship.bmp"};
    static struct option long_opts[] = {
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}};
    const char *optstr = "hV:B::s:d:n:r:o:";
    int c;
    while ((c = getopt_long(argc, argv, optstr, long_opts, NULL)) != -1)
    {
        int ok = 1;
        switch (c)
        {
        case 'h':
            print_help(argv[0]);
            return EXIT_SUCCESS;
        case 'V':
            p.impl = (int)parse_unsigned_or_fail("-V", optarg, &ok);
            if (!ok)
                return EXIT_FAILURE;
            break;
        case 'B':
            p.bench_enabled = 1;
            if (optarg)
            {
                // Restricting user input for -B Num, avoiding -B -something like this
                p.bench_reps = parse_unsigned_or_fail("-B", optarg, &ok);
                if (!ok)
                    return EXIT_FAILURE;
                if (p.bench_reps == 0)
                {
                    fprintf(stderr, "Error: -B The repetitions must be >= 1\n");
                    return EXIT_FAILURE;
                }
            }
            else
            {
                // if only B
                p.bench_reps = 1;
            }
            break;

        case 's':
            if (!parse_two_floats(optarg, &p.start_real, &p.start_imag))
            {
                fprintf(stderr, "Error: It is an invalid -s format, expected <real>,<imag>\n");
                return EXIT_FAILURE;
            }
            break;

        case 'd':
            if (!parse_two_sizes(optarg, &p.width, &p.height) || p.width == 0 || p.height == 0)
            {
                fprintf(stderr, "Error: It is an invalid -d format, expected <width>,<height> with both > 0\n");
                return EXIT_FAILURE;
            }
            break;

        case 'n':
            p.n = parse_unsigned_or_fail("-n", optarg, &ok);
            if (!ok)
                return EXIT_FAILURE;
            break;

        case 'r':
            p.res = parse_float_or_fail("-r", optarg, &ok);
            if (!ok)
                return EXIT_FAILURE;
            break;

        case 'o':
            p.out = optarg;
            break;

        default:
            fprintf(stderr, "Error: unknown/invalid option. Please Use --help.\n");
            return EXIT_FAILURE;
        }
    }

    if (optind != argc)
    {
        fprintf(stderr, "Error: An unexpected argument: '%s'. Use --help.\n", argv[optind]);
        return EXIT_FAILURE;
    }

    size_t img_size = p.width * p.height * 3;
    unsigned char *img = (unsigned char *)malloc(img_size);
    if (!img)
    {
        fprintf(stderr, "Error: malloc failed, please check again\n");
        return EXIT_FAILURE;
    }
    void (*impl_fn)(float complex, size_t, size_t, float, unsigned, unsigned char *) = NULL;
    if (p.impl == 0)
    {
        impl_fn = burning_ship;
    }
    else if (p.impl == 1)
    {
        impl_fn = burning_ship_V1;
    }
    else
    {
        fprintf(stderr, "Error: The implementation -V %d not available.Choose between 0 and 1\n", p.impl);
        free(img);
        return EXIT_FAILURE;
    }
    float complex start_pos = p.start_real + p.start_imag * I;
    if (!p.bench_enabled)
    {
        impl_fn(start_pos, p.width, p.height, p.res, p.n, img);
    }
    else
    {
        //  Benchmark MOde
        long long t0 = now_ns();
        for (unsigned i = 0; i < p.bench_reps; i++)
        {
            impl_fn(start_pos, p.width, p.height, p.res, p.n, img);
        }
        long long t1 = now_ns();
        double ms = (double)(t1 - t0) / 1e6;
        printf("Benchmark(V=%d, reps=%u): %.3f ms total, %.3f ms per run\n",
               p.impl, p.bench_reps, ms, ms / (double)p.bench_reps);
    }
    if (!write_bmp24(p.out, img, p.width, p.height))
    {
        free(img);
        return EXIT_FAILURE;
    }
    free(img);
    return EXIT_SUCCESS;
}