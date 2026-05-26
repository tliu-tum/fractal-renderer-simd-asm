# Burning Ship Fractal — C & x86-64 SSE Assembly

Renders the [Burning Ship fractal](https://en.wikipedia.org/wiki/Burning_Ship_fractal) to a 24-bit BMP file.
The same algorithm is implemented twice: once in plain C as a reference, and once in hand-written x86-64 Assembly using SSE SIMD instructions to process 4 pixels per iteration.

Built as part of a university systems programming course (C, Assembly, performance analysis).

## Files

```
main.c        - CLI entry point, argument parsing, benchmark loop
impl_c.c      - Reference implementation in C
impl_asm.S    - Optimized implementation in x86-64 Assembly (SSE SIMD)
bmp.c/h       - BMP file writer (24-bit, no dependencies)
impl.h        - Shared function signatures
Makefile      - Build config; sanitizer flags included (commented out)
```

## Build

Requires GCC and Make on a Linux x86-64 machine.

```bash
make
```

To build with AddressSanitizer + UBSan (for development/debugging):

```bash
# Uncomment the sanitizer lines in Makefile, then:
make clean && make
```

## Usage

```
./main [options]

Options:
  -V <0|1>            Implementation: 0 = ASM SIMD (default), 1 = C reference
  -B[N]               Benchmark mode, repeat N times (e.g. -B200)
  -s <real>,<imag>    Start point in the complex plane  (default: -2.0,-2.0)
  -d <width>,<height> Image size in pixels              (default: 800,600)
  -r <float>          Step size per pixel               (default: 0.005)
  -n <int>            Max iterations per pixel          (default: 200)
  -o <filename>       Output BMP filename               (default: burning_ship.bmp)
```

**Example — render and open:**
```bash
./main -V 0 -s -2.0,-2.0 -d 800,600 -r 0.005 -n 200 -o out.bmp
```

**Example — benchmark ASM SIMD vs C reference at 512×512:**
```bash
./main -V 0 -B200 -s -2.0,-2.0 -d 512,512 -r 0.005 -n 200 -o /dev/null
./main -V 1 -B200 -s -2.0,-2.0 -d 512,512 -r 0.005 -n 200 -o /dev/null
```

## How it works

The fractal is computed per-pixel using the recurrence:

```
z_{k+1} = (|Re(z)| + |Im(z)|·i)² + c
```

A pixel is colored black if it does not escape within `n` iterations; otherwise it is colored using a smooth gradient based on how quickly it escapes (Bernstein polynomial coloring).

The ASM version uses SSE registers to compute 4 pixels in parallel per loop iteration. It uses `movmskps` to check all 4 lanes at once and exits early as soon as every lane has escaped, avoiding unnecessary iterations.

## Benchmark results

Tested on AMD EPYC 9554P (x86-64, Zen 4), averaged over 200 runs:

| Resolution  | C -O0 (ms) | C -O2 (ms) | ASM SISD (ms) | ASM SIMD (ms) | Speedup (vs C -O2) |
|-------------|------------|------------|---------------|---------------|--------------------|
| 128 × 128   | 0.346      | 0.085      | 0.104         | 0.072         | 1.18×              |
| 256 × 256   | 5.741      | 1.739      | 1.529         | 0.607         | 2.86×              |
| 512 × 512   | 243.947    | 62.799     | 67.780        | 13.171        | **4.77×**          |
| 1024 × 1024 | 271.636    | 69.344     | 75.938        | 17.634        | 3.93×              |
| 2048 × 2048 | 340.730    | 85.034     | 97.642        | 32.329        | 2.63×              |
| 4096 × 4096 | 595.155    | 149.311    | 172.514       | 94.466        | 1.58×              |

The speedup peaks at 512×512 and drops off at larger sizes because the working set exceeds L2 cache, shifting the bottleneck from compute to memory bandwidth.

## Technologies

- C11, x86-64 Assembly (Intel syntax)
- SSE / SIMD (via GCC inline `.S` file)
- GCC, Make
- AddressSanitizer + UBSan (used during development)
