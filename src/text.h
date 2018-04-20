#pragma once

#define MAX_GLYPHS 128
#define ATLAS_DIM_MIN 128
#define ATLAS_DIM_MAX 2048

#include <ft2build.h>
#include FT_FREETYPE_H

#include "main.h"
#include "main_platform.h"
#include "km_math.h"

struct GlyphInfo
{
    Vec2Int atlasCoords;
    uint32 width;
    uint32 height;
    
    int offsetX;
    int offsetY;
    int advanceX;
    int advanceY;
};
struct FontFace
{
    uint32 height;
    GlyphInfo glyphInfo[MAX_GLYPHS];

    uint8 atlasData[ATLAS_DIM_MAX * ATLAS_DIM_MAX * sizeof(uint8)];
    uint32 atlasWidth;
    uint32 atlasHeight;
};

bool LoadFontFace(ThreadContext* thread,
    FT_Library library,
    const char* path, uint32 height,
    DEBUGPlatformReadFileFunc* DEBUGPlatformReadFile,
    DEBUGPlatformFreeFileMemoryFunc* DEBUGPlatformFreeFileMemory,
    FontFace* fontFace);

int GetTextWidth(const FontFace* face, const char* text);
void RenderText(const FontFace* face, const char* text,
    Vec2Int pos, Vec4 color,
    GameBackbuffer* backbuffer);
void RenderText(const FontFace* face, const char* text,
    Vec2Int pos, Vec2 anchor, Vec4 color,
    GameBackbuffer* backbuffer);