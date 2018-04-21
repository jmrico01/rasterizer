#include "load_bmp.h"

#define BMP_HEADER_SIZE 54

// Code borrowed from
// http://www.opengl-tutorial.org/beginners-tutorials/tutorial-5-a-textured-cube/

bool LoadBMP(ThreadContext* thread,
    const char* fileName, Bitmap* dst,
    DEBUGPlatformReadFileFunc* DEBUGPlatformReadFile,
    DEBUGPlatformFreeFileMemoryFunc* DEBUGPlatformFreeFileMemory)
{
    DEBUGReadFileResult bmpFile = DEBUGPlatformReadFile(thread, fileName);
    if (!bmpFile.data) {
        DEBUG_PRINT("Failed to open BMP file at: %s\n", fileName);
        return false;
    }

    if (bmpFile.size < 54) {
        DEBUG_PRINT("Invalid BMP file: incomplete header (%s)\n", fileName);
        return false;
    }

    uint8* fileData = (uint8*)bmpFile.data;
    if (fileData[0] != 'B' || fileData[1] != 'M') {
        DEBUG_PRINT("Invalid BMP file: no BM bytes (%s)\n", fileName);
        return false;
    }

    int width = *((int*)&fileData[0x12]);
    int height = *((int*)&fileData[0x16]);
    int dataPos = *((int*)&fileData[0x0a]);
    if (dataPos <= 0) {
        dataPos = 54;
    }
    int16 bitsPerPixel = *((int16*)&fileData[0x1c]);
    if (bitsPerPixel != 24) {
        DEBUG_PRINT("Unsupported BMP color depth: %d (%s)\n",
            bitsPerPixel, fileName);
        return false;
    }
    dst->bytesPerPixel = (int)bitsPerPixel / 8;

    /*int imageSize = *((int*)&fileData[0x22]);
    if (imageSize <= 0) {
        imageSize = width * height * dst->bytesPerPixel;
    }*/
    // TODO: for some reason, imageSize in header is usually
    // slightly bigger than width * height * dst->bytesPerPixel
    int imageSize = width * height * dst->bytesPerPixel;
    DEBUG_ASSERT(imageSize <= IMAGE_MAX_SIZE);

    dst->width = width;
    dst->height = height;
    for (int i = 0; i < imageSize; i++) {
        dst->data[i] = fileData[dataPos + i];
    }

    DEBUGPlatformFreeFileMemory(thread, &bmpFile);

    return true;
}