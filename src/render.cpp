#include "render.h"

// Declares 4 uint8 variables: nameR, nameG, nameB, nameA
#define COLOR_VEC4_TO_UINT8(v4, nameR, nameG, nameB, nameA) \
    uint8 (nameR) = (uint8)((v4).r * 255.0f); \
    uint8 (nameG) = (uint8)((v4).g * 255.0f); \
    uint8 (nameB) = (uint8)((v4).b * 255.0f); \
    uint8 (nameA) = (uint8)((v4).a * 255.0f);


internal void SetPixelColor(GameBackbuffer* backbuffer,
    Vec2Int coords, Vec4 color)
{
    DEBUG_ASSERT(0 <= coords.x && coords.x < backbuffer->width);
    DEBUG_ASSERT(0 <= coords.y && coords.y < backbuffer->height);
    COLOR_VEC4_TO_UINT8(color, r, g, b, a);

    uint8* backbufferData = (uint8*)backbuffer->data;
    uint32* pixel = (uint32*)(backbufferData +
        coords.y * backbuffer->width * backbuffer->bytesPerPixel
        + coords.x * backbuffer->bytesPerPixel);
    *pixel = (a << 24) | (r << 16) | (g << 8) | b;
}

void ClearBackbuffer(GameBackbuffer* backbuffer, Vec4 color)
{
    COLOR_VEC4_TO_UINT8(color, r, g, b, a);

    uint8* row = (uint8*)backbuffer->data;    
    for (int j = 0; j < backbuffer->height; j++) {
        uint32* pixel = (uint32*)row;
        for (int i = 0; i < backbuffer->width; i++) {
            *pixel++ = (a << 24) | (r << 16) | (g << 8) | b;
        }
        
        row += backbuffer->width * backbuffer->bytesPerPixel;
    }
}

void RenderOverwriteRect(
    GameBackbuffer* backbuffer,
    Vec2Int pos, Vec2Int size, Vec4 color)
{
    uint8* backbufferData = (uint8*)backbuffer->data;
    int backbufferBytesPerRow = backbuffer->width * backbuffer->bytesPerPixel;
    COLOR_VEC4_TO_UINT8(color, r, g, b, a);

    for (int j = 0; j < size.y; j++) {
        for (int i = 0; i < size.x; i++) {
            if (pos.x + i < 0 || pos.x + i >= backbuffer->width) {
                continue;
            }
            if (pos.y + j < 0 || pos.y + j >= backbuffer->height) {
                continue;
            }
            int backbufferOff = (pos.y + j) * backbufferBytesPerRow
                + (pos.x + i) * backbuffer->bytesPerPixel;
            uint32* pixel = (uint32*)(backbufferData + backbufferOff);
            *pixel = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }
}

void RenderOverwriteGrayscaleBitmap(
    GameBackbuffer* backbuffer, Vec2Int pos,
    const uint8* bitmap, int bitmapWidth, int bitmapHeight)
{
    RenderOverwriteGrayscaleBitmapSection(backbuffer, pos,
        bitmap, bitmapWidth, bitmapHeight,
        Vec2Int::zero, bitmapWidth, bitmapHeight);
}

void RenderOverwriteGrayscaleBitmapSection(
    GameBackbuffer* backbuffer, Vec2Int pos,
    const uint8* bitmap, int bitmapWidth, int bitmapHeight,
    Vec2Int origin, int sectionWidth, int sectionHeight)
{
    uint8* backbufferData = (uint8*)backbuffer->data;
    int backbufferBytesPerRow = backbuffer->width * backbuffer->bytesPerPixel;

    for (int j = 0; j < sectionHeight; j++) {
        for (int i = 0; i < sectionWidth; i++) {
            if (pos.x + i < 0 || pos.x + i >= backbuffer->width) {
                continue;
            }
            if (pos.y + j < 0 || pos.y + j >= backbuffer->height) {
                continue;
            }
            int backbufferOff = (pos.y + j) * backbufferBytesPerRow
                + (pos.x + i) * backbuffer->bytesPerPixel;
            int bitmapOff = (origin.y + j) * bitmapWidth + origin.x + i;
            uint8 gray = bitmap[bitmapOff];
            uint32* pixel = (uint32*)(backbufferData + backbufferOff);
            *pixel = (255 << 24) | (gray << 16) | (gray << 8) | gray;
        }
    }
}

void RenderAddClampGrayscaleBitmapSection(
    GameBackbuffer* backbuffer, Vec2Int pos,
    const uint8* bitmap, int bitmapWidth, int bitmapHeight,
    Vec2Int origin, int sectionWidth, int sectionHeight,
    Vec4 colorMult)
{
    uint8* backbufferData = (uint8*)backbuffer->data;
    int backbufferBytesPerRow = backbuffer->width * backbuffer->bytesPerPixel;

    for (int j = 0; j < sectionHeight; j++) {
        for (int i = 0; i < sectionWidth; i++) {
            if (pos.x + i < 0 || pos.x + i >= backbuffer->width) {
                continue;
            }
            if (pos.y + j < 0 || pos.y + j >= backbuffer->height) {
                continue;
            }
            int backbufferOff = (pos.y + j) * backbufferBytesPerRow
                + (pos.x + i) * backbuffer->bytesPerPixel;
            int bitmapOff = (origin.y + j) * bitmapWidth + origin.x + i;
            uint8 gray = bitmap[bitmapOff];
            uint32* pixel = (uint32*)(backbufferData + backbufferOff);
            uint8 r = (uint8)((*pixel >> 16) & 0xff);
            uint8 g = (uint8)((*pixel >> 8)  & 0xff);
            uint8 b = (uint8)((*pixel)       & 0xff);
            r = (uint8)MinInt((uint8)(gray * colorMult.r) + r, 255);
            g = (uint8)MinInt((uint8)(gray * colorMult.g) + g, 255);
            b = (uint8)MinInt((uint8)(gray * colorMult.b) + b, 255);
            *pixel = (255 << 24) | (r << 16) | (g << 8) | b;
        }
    }
}

void RenderTriangleWire(GameBackbuffer* backbuffer,
    Vec2Int verts[3], Vec4 color)
{
    Vec2Int edges[3];
    edges[0] = verts[1] - verts[0];
    edges[1] = verts[2] - verts[1];
    edges[2] = verts[0] - verts[2];

    for (int i = 0; i < 3; i++) {
        Vec2 edge = {
            (float32)edges[i].x,
            (float32)edges[i].y
        };
        float32 edgeLen = Mag(edge);
        edge /= edgeLen;
        for (float t = 0; t < edgeLen; t += 0.5f) {
            Vec2Int pix;
            pix.x = RoundFloat32((float32)verts[i].x + edge.x * t);
            pix.y = RoundFloat32((float32)verts[i].y + edge.y * t);
            if (0 <= pix.x && pix.x < backbuffer->width
            && 0 <= pix.y && pix.y < backbuffer->height) {
                SetPixelColor(backbuffer, pix, color);
            }
            else {
                break;
            }
        }
    }
}