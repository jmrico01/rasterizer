#pragma once

#include "km_defines.h"
#include "main_platform.h"

void ClearBackbuffer(GameBackbuffer* backbuffer, Vec4 color);

void RenderOverwriteRect(
    GameBackbuffer* backbuffer,
    Vec2Int pos, Vec2Int size, Vec4 color);

void RenderOverwriteGrayscaleBitmap(
    GameBackbuffer* backbuffer, Vec2Int pos,
    const uint8* bitmap, int bitmapWidth, int bitmapHeight);
void RenderOverwriteGrayscaleBitmapSection(
    GameBackbuffer* backbuffer, Vec2Int pos,
    const uint8* bitmap, int bitmapWidth, int bitmapHeight,
    Vec2Int origin, int sectionWidth, int sectionHeight);
void RenderAddClampGrayscaleBitmapSection(
    GameBackbuffer* backbuffer, Vec2Int pos,
    const uint8* bitmap, int bitmapWidth, int bitmapHeight,
    Vec2Int origin, int sectionWidth, int sectionHeight);

void RenderTriangleWire(GameBackbuffer* backbuffer,
    Vec2Int verts[3], Vec4 color);