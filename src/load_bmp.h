#pragma once

#include "main_platform.h"

#define IMAGE_MAX_SIZE 2048 * 2048 * 3

struct Bitmap
{
    uint8 data[IMAGE_MAX_SIZE];
    int width;
    int height;
    int bytesPerPixel;
};

bool LoadBMP(ThreadContext* thread,
    const char* fileName, Bitmap* dst,
    DEBUGPlatformReadFileFunc* DEBUGPlatformReadFile,
    DEBUGPlatformFreeFileMemoryFunc* DEBUGPlatformFreeFileMemory);