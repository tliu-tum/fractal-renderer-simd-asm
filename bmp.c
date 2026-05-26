#include "bmp.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#pragma pack(push, 1)
typedef struct
{
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} BITMAPFILEHEADER;

typedef struct
{
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)

int write_bmp24(const char *filename, const unsigned char *img_bgr, size_t width, size_t height)
{
    FILE *f = fopen(filename, "wb");
    if (!f)
    {
        fprintf(stderr, "Error: cannot open output file: %s\n", filename);
        return 0;
    }

    size_t row_bytes = width * 3;
    size_t padding = (4 - (row_bytes % 4)) % 4;
    size_t stride = row_bytes + padding;
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    memset(&fileHeader, 0, sizeof(fileHeader));
    memset(&infoHeader, 0, sizeof(infoHeader));
    fileHeader.bfType = 0x4D42;
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    fileHeader.bfSize = (uint32_t)(fileHeader.bfOffBits + stride * height);
    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = (int32_t)width;
    infoHeader.biHeight = (int32_t)height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = 0;
    infoHeader.biSizeImage = (uint32_t)(stride * height);
    fwrite(&fileHeader, sizeof(fileHeader), 1, f);
    fwrite(&infoHeader, sizeof(infoHeader), 1, f);
    unsigned char pad[3] = {0, 0, 0};

    for (size_t y = 0; y < height; y++)
    {
        size_t src_y = height - 1 - y;
        const unsigned char *row = img_bgr + src_y * width * 3;

        fwrite(row, 1, row_bytes, f);
        fwrite(pad, 1, padding, f);
    }

    fclose(f);
    return 1;
}
