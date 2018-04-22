#pragma once

#include "km_defines.h"
#include "main_platform.h"

void ClearBackbuffer(GameBackbuffer* backbuffer, Vec3 color, uint32 depth);

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

void RenderOverwriteColorBitmap(
    GameBackbuffer* backbuffer, Vec2Int pos,
    Bitmap* bitmap);

void RenderTrianglesWire(GameBackbuffer* backbuffer,
    const MeshScratch* scratch,
    Vec3 color);
void RenderTrianglesFlat(GameBackbuffer* backbuffer,
    const MeshScratch* scratch,
    Vec3 cameraPos, Vec3 lightPos, Material material);
void RenderTrianglesGouraud(GameBackbuffer* backbuffer,
    const MeshScratch* scratch,
    Vec3 cameraPos, Vec3 lightPos, Material material);
void RenderTrianglesPhong(GameBackbuffer* backbuffer,
    const MeshScratch* scratch,
    Vec3 cameraPos, Vec3 lightPos, Material material);
void RenderTrianglesPhongTextured(GameBackbuffer* backbuffer,
    const MeshScratch* scratch,
    Bitmap* diffuseMap, Bitmap* specularMap, Bitmap* normalMap,
    Vec3 cameraPos, Vec3 lightPos, Material material);

Vec3 CalculatePhongColor(Vec3 vertex, Vec3 normal,
    Vec3 cameraPos, Vec3 lightPos, Material material);