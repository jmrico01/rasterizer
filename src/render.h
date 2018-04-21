#pragma once

#include "km_defines.h"
#include "main_platform.h"

void ClearBackbuffer(GameBackbuffer* backbuffer, Vec3 color);

void RenderOverwriteRect(
    GameBackbuffer* backbuffer,
    Vec2Int pos, Vec2Int size, Vec3 color);

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
    Vec2Int origin, int sectionWidth, int sectionHeight,
    Vec3 colorMult);

void RenderTriangleWire(GameBackbuffer* backbuffer,
    Vec3Int verts[3], Vec3 color);
void RenderTriangleFlat(GameBackbuffer* backbuffer,
    Vec3Int verts[3], Vec3 color);

Vec3 CalculatePhongColor(Vec3 vertex, Vec3 normal,
    Vec3 cameraPos, Vec3 lightPos, Material material);