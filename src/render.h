#pragma once

#include "km_defines.h"
#include "main_platform.h"

internal void ClearBackbuffer(GameBackbuffer* backbuffer, Vec4 color);
internal void RenderGrayscaleBitmap(GameBackbuffer* backbuffer,
    int x, int y, const uint8* bitmap, int width, int height);
internal void RenderGrayscaleBitmapSection(
    GameBackbuffer* backbuffer, int x, int y,
    const uint8* bitmap, int bitmapWidth, int bitmapHeight,
    int sectionX, int sectionY, int sectionWidth, int sectionHeight);