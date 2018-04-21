#include "text.h"

bool LoadFontFace(ThreadContext* thread,
    FT_Library library,
    const char* path, uint32 height,
    DEBUGPlatformReadFileFunc* DEBUGPlatformReadFile,
    DEBUGPlatformFreeFileMemoryFunc* DEBUGPlatformFreeFileMemory,
    FontFace* fontFace)
{
    fontFace->height = height;

    // Load font face using FreeType.
    FT_Face ftFace;
    DEBUGReadFileResult fontFile = DEBUGPlatformReadFile(thread, path);
    FT_Open_Args openArgs = {};
    openArgs.flags = FT_OPEN_MEMORY;
    openArgs.memory_base = (const FT_Byte*)fontFile.data;
    openArgs.memory_size = (FT_Long)fontFile.size;
    FT_Error error = FT_Open_Face(library, &openArgs, 0, &ftFace);
    if (error == FT_Err_Unknown_File_Format) {
        DEBUG_PRINT("Unsupported file format for %s\n", path);
        return false;
    }
    else if (error) {
        DEBUG_PRINT("Font file couldn't be read: %s\n", path);
        return false;
    }

    error = FT_Set_Pixel_Sizes(ftFace, 0, height);
    if (error) {
        DEBUG_PRINT("Failed to set font pixel size\n");
        return false;
    }

    // Fill in the non-UV parameters of GlyphInfo struct array.
    for (int ch = 0; ch < MAX_GLYPHS; ch++) {
        error = FT_Load_Char(ftFace, ch, FT_LOAD_RENDER);
        if (error) {
            fontFace->glyphInfo[ch].width = 0;
            fontFace->glyphInfo[ch].height = 0;
            fontFace->glyphInfo[ch].offsetX = 0;
            fontFace->glyphInfo[ch].offsetY = 0;
            fontFace->glyphInfo[ch].advanceX = 0;
            fontFace->glyphInfo[ch].advanceY = 0;
            continue;
        }
        FT_GlyphSlot glyph = ftFace->glyph;

        fontFace->glyphInfo[ch].width = glyph->bitmap.width;
        fontFace->glyphInfo[ch].height = glyph->bitmap.rows;
        fontFace->glyphInfo[ch].offsetX = glyph->bitmap_left;
        fontFace->glyphInfo[ch].offsetY =
            glyph->bitmap_top - (int)glyph->bitmap.rows;
        fontFace->glyphInfo[ch].advanceX = glyph->advance.x;
        fontFace->glyphInfo[ch].advanceY = glyph->advance.y;
    }

    const uint32 pad = 2;
    // Find the lowest dimension atlas that fits all characters to be loaded.
    // Atlas dimension is always square and power-of-two.
    for (uint32 dim = ATLAS_DIM_MIN; dim <= ATLAS_DIM_MAX; dim *= 2) {
        uint32 originI = pad;
        uint32 originJ = pad;
        bool fits = true;
        for (int ch = 0; ch < MAX_GLYPHS; ch++) {
            uint32 glyphWidth = fontFace->glyphInfo[ch].width;
            if (originI + glyphWidth + pad >= dim) {
                originI = pad;
                originJ += height + pad;
            }
            originI += glyphWidth + pad;

            if (originJ + pad >= dim) {
                fits = false;
                break;
            }
        }
        if (fits) {
            fontFace->atlasWidth = dim;
            fontFace->atlasHeight = dim;
            break;
        }
    }

    if (fontFace->atlasWidth == 0 || fontFace->atlasHeight == 0) {
        DEBUG_PRINT("PANIC! Atlas not big enough\n"); // TODO error handling
        return false;
    }
    //printf("atlasSize: %u x %u\n", atlasWidth, atlasHeight);

    // Allocate and initialize atlas texture data.
    for (uint32 j = 0; j < fontFace->atlasHeight; j++) {
        for (uint32 i = 0; i < fontFace->atlasWidth; i++) {
            fontFace->atlasData[j * fontFace->atlasWidth + i] = 0;
        }
    }

    uint32 originI = pad;
    uint32 originJ = pad;
    for (int ch = 0; ch < MAX_GLYPHS; ch++) {
        error = FT_Load_Char(ftFace, ch, FT_LOAD_RENDER);
        if (error) {
            continue;
        }
        FT_GlyphSlot glyph = ftFace->glyph;

        uint32 glyphWidth = glyph->bitmap.width;
        uint32 glyphHeight = glyph->bitmap.rows;
        if (originI + glyphWidth + pad >= fontFace->atlasWidth) {
            originI = pad;
            originJ += height + pad;
        }

        // Write glyph bitmap into atlas.
        for (uint32 j = 0; j < glyphHeight; j++) {
            for (uint32 i = 0; i < glyphWidth; i++) {
                int indAtlas = (originJ + j) * fontFace->atlasWidth
                    + originI + i;
                int indBuffer = (glyphHeight - 1 - j) * glyphWidth + i;
                fontFace->atlasData[indAtlas] = glyph->bitmap.buffer[indBuffer];
            }
        }
        // Save UV coordinate data.
        Vec2Int atlasCoords = { (int)originI, (int)originJ };
        fontFace->glyphInfo[ch].atlasCoords = atlasCoords;

        originI += glyphWidth + pad;
    }

    FT_Done_Face(ftFace);
    DEBUGPlatformFreeFileMemory(thread, &fontFile);

    return true;
}

int GetTextWidth(const FontFace* face, const char* text)
{
    float x = 0.0f;
    float y = 0.0f;
    for (const char* p = text; *p != 0; p++) {
        GlyphInfo glyphInfo = face->glyphInfo[*p];
        x += (float)glyphInfo.advanceX / 64.0f;
        y += (float)glyphInfo.advanceY / 64.0f;
    }

    return (int)x;
}

void RenderText(const FontFace* face, const char* text,
    Vec2Int pos, Vec4 color,
    GameBackbuffer* backbuffer)
{
    int i = 0, j = 0;
    for (const char* p = text; *p != 0; p++) {
        GlyphInfo glyphInfo = face->glyphInfo[*p];
        Vec2Int glyphPos = {
            pos.x + i + glyphInfo.offsetX,
            pos.y + j + glyphInfo.offsetY
        };
        RenderAddClampGrayscaleBitmapSection(backbuffer, glyphPos,
            face->atlasData, face->atlasWidth, face->atlasHeight,
            glyphInfo.atlasCoords,
            glyphInfo.width, glyphInfo.height,
            color
        );

        i += glyphInfo.advanceX / 64;
        j += glyphInfo.advanceY / 64;
    }
}

// Anchor is in range (0-1, 0-1).
void RenderText(const FontFace* face, const char* text,
    Vec2Int pos, Vec2 anchor, Vec4 color,
    GameBackbuffer* backbuffer)
{
    int textWidth = GetTextWidth(face, text);
    pos.x -= (int)(anchor.x * textWidth);
    pos.y -= (int)(anchor.y * face->height);

    RenderText(face, text, pos, color, backbuffer);
}