#include "render.h"

internal void ClearBackbuffer(GameBackbuffer* backbuffer, Vec4 color)
{
    uint8 r = (uint8)(color.r * 255.0f);
    uint8 g = (uint8)(color.g * 255.0f);
    uint8 b = (uint8)(color.b * 255.0f);
    uint8 a = (uint8)(color.a * 255.0f);

    uint8* row = (uint8*)backbuffer->data;    
    for (int j = 0; j < backbuffer->height; j++) {
        uint32* pixel = (uint32*)row;
        for (int i = 0; i < backbuffer->width; i++) {
            *pixel++ = (a << 24) | (r << 16) | (g << 8) | b;
        }
        
        row += backbuffer->width * backbuffer->bytesPerPixel;
    }
}

internal void RenderGrayscaleBitmap(GameBackbuffer* backbuffer,
    int x, int y, const uint8* bitmap, int bitmapWidth, int bitmapHeight)
{
    RenderGrayscaleBitmapSection(backbuffer, x, y,
        bitmap, bitmapWidth, bitmapHeight, 0, 0, bitmapWidth, bitmapHeight);
}

internal void RenderGrayscaleBitmapSection(
    GameBackbuffer* backbuffer, int x, int y,
    const uint8* bitmap, int bitmapWidth, int bitmapHeight,
    int sectionX, int sectionY, int sectionWidth, int sectionHeight)
{
    uint8* backbufferData = (uint8*)backbuffer->data;
    int backbufferBytesPerRow = backbuffer->width * backbuffer->bytesPerPixel;

    for (int j = 0; j < sectionHeight; j++) {
        for (int i = 0; i < sectionWidth; i++) {
            if (x + i < 0 || x + i >= backbuffer->width) {
                continue;
            }
            if (y + j < 0 || y + j >= backbuffer->height) {
                continue;
            }
            int backbufferOff = (y + j) * backbufferBytesPerRow
                + (x + i) * backbuffer->bytesPerPixel;
            int bitmapOff = (sectionY + j) * bitmapWidth + sectionX + i;
            uint8 gray = bitmap[bitmapOff];
            uint32* pixel = (uint32*)(backbufferData + backbufferOff);
            *pixel = (255 << 24) | (gray << 16) | (gray << 8) | gray;
        }
    }
}