#include "render.h"

struct BoundingBox
{
    Vec2Int min, max;
};

// Declares 4 uint8 variables: nameR, nameG, nameB, nameA
#define COLOR_VEC3_TO_UINT8(v4, nameR, nameG, nameB) \
    uint8 (nameR) = (uint8)((v4).r * 255.0f); \
    uint8 (nameG) = (uint8)((v4).g * 255.0f); \
    uint8 (nameB) = (uint8)((v4).b * 255.0f);

internal void SetPixelColor(GameBackbuffer* backbuffer,
    Vec3Int coords, Vec3 color)
{
    DEBUG_ASSERT(0 <= coords.x && coords.x < backbuffer->width);
    DEBUG_ASSERT(0 <= coords.y && coords.y < backbuffer->height);
    COLOR_VEC3_TO_UINT8(color, r, g, b);

    int ind = coords.y * backbuffer->width + coords.x;
    if ((uint32)coords.z > backbuffer->depth[ind]) {
        uint8* backbufferData = (uint8*)backbuffer->data;
        uint32* pixel = (uint32*)(backbufferData +
            coords.y * backbuffer->width * backbuffer->bytesPerPixel
            + coords.x * backbuffer->bytesPerPixel);
        *pixel = (coords.z << 24) | (r << 16) | (g << 8) | b;
        backbuffer->depth[ind] = (uint32)coords.z;
    }
}

// Clears to the given color, with zero depth
void ClearBackbuffer(GameBackbuffer* backbuffer, Vec3 color, uint32 depth)
{
    COLOR_VEC3_TO_UINT8(color, r, g, b);

    uint8* row = (uint8*)backbuffer->data;    
    for (int j = 0; j < backbuffer->height; j++) {
        uint32* pixel = (uint32*)row;
        for (int i = 0; i < backbuffer->width; i++) {
            *pixel++ = (r << 16) | (g << 8) | b;
            backbuffer->depth[j * backbuffer->width + i] = depth;
        }
        
        row += backbuffer->width * backbuffer->bytesPerPixel;
    }
}

void RenderOverwriteRect(
    GameBackbuffer* backbuffer,
    Vec2Int pos, Vec2Int size, Vec3 color)
{
    uint8* backbufferData = (uint8*)backbuffer->data;
    int backbufferBytesPerRow = backbuffer->width * backbuffer->bytesPerPixel;
    COLOR_VEC3_TO_UINT8(color, r, g, b);

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
            *pixel = (255 << 24) | (r << 16) | (g << 8) | b;
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
    Vec3 colorMult)
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
    Vec3Int verts[3], Vec3 color)
{
    Vec3Int edges[3];
    edges[0] = verts[1] - verts[0];
    edges[1] = verts[2] - verts[1];
    edges[2] = verts[0] - verts[2];

    for (int i = 0; i < 3; i++) {
        Vec3 edge = {
            (float32)edges[i].x,
            (float32)edges[i].y,
            0.0f
        };
        float32 edgeLen = Mag(edge);
        edge /= edgeLen;
        for (float t = 0; t < edgeLen; t += 0.5f) {
            Vec3Int pix;
            pix.x = RoundFloat32((float32)verts[i].x + edge.x * t);
            pix.y = RoundFloat32((float32)verts[i].y + edge.y * t);
            pix.z = INT_MAX;
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

internal inline BoundingBox ComputeTriangleBoundingBox(Vec3Int triangle[3])
{
    BoundingBox bb;
    bb.min.x = MinInt(MinInt(triangle[0].x, triangle[1].x), triangle[2].x);
    bb.min.y = MinInt(MinInt(triangle[0].y, triangle[1].y), triangle[2].y);
    bb.max.x = MaxInt(MaxInt(triangle[0].x, triangle[1].x), triangle[2].x);
    bb.max.y = MaxInt(MaxInt(triangle[0].y, triangle[1].y), triangle[2].y);
    return bb;
}

internal inline float32 ComputeTriangleSignedArea(Vec3Int triangle[3])
{
    return 0.5f * (
        (triangle[1].x - triangle[0].x) * (triangle[2].y - triangle[0].y)
        - (triangle[2].x - triangle[0].x) * (triangle[1].y - triangle[0].y));
}
internal inline float32 ComputeTriangleSignedArea(
    Vec3Int v0, Vec3Int v1, Vec3Int v2)
{
    return 0.5f * (
        (v1.x - v0.x) * (v2.y - v0.y)
        - (v2.x - v0.x) * (v1.y - v0.y));
}

internal Vec3 ComputeBarycentricCoords(Vec3Int triangle[3], Vec3Int p)
{
    Vec3 result;
    float32 area2 = 2.0f * ComputeTriangleSignedArea(triangle);
    result.x = ComputeTriangleSignedArea(triangle[0], triangle[1], p);
    result.x /= area2;
    result.y = ComputeTriangleSignedArea(triangle[1], triangle[2], p);
    result.y /= area2;
    result.z = ComputeTriangleSignedArea(triangle[2], triangle[0], p);
    result.z /= area2;
    return result;
}

void RenderTriangleFlat(GameBackbuffer* backbuffer,
    Vec3Int verts[3], Vec3 color)
{
    BoundingBox bound = ComputeTriangleBoundingBox(verts);
    bound.min.x = MaxInt(bound.min.x, 0);
    bound.max.x = MinInt(bound.max.x, backbuffer->width - 1);
    bound.min.y = MaxInt(bound.min.y, 0);
    bound.max.y = MinInt(bound.max.y, backbuffer->height - 1);
    Vec3Int pix;
    for (pix.x = bound.min.x; pix.x < bound.max.x; pix.x++) {
        for (pix.y = bound.min.y; pix.y < bound.max.y; pix.y++) {
            Vec3 bCoords = ComputeBarycentricCoords(verts, pix);
            if (bCoords.x >= 0.0f && bCoords.y >= 0.0f && bCoords.z >= 0.0f) {
                pix.z = (int)(verts[2].z * bCoords.x
                    + verts[0].z * bCoords.y
                    + verts[1].z * bCoords.z);
                pix.z = ClampInt(pix.z, 0, INT_MAX);
                SetPixelColor(backbuffer, pix, color);
            }
        }
    }
}

void RenderTriangleGouraud(GameBackbuffer* backbuffer,
    Vec3Int verts[3], Vec3 colors[3])
{
    BoundingBox bound = ComputeTriangleBoundingBox(verts);
    bound.min.x = MaxInt(bound.min.x, 0);
    bound.max.x = MinInt(bound.max.x, backbuffer->width - 1);
    bound.min.y = MaxInt(bound.min.y, 0);
    bound.max.y = MinInt(bound.max.y, backbuffer->height - 1);
    Vec3Int pix;
    for (pix.x = bound.min.x; pix.x < bound.max.x; pix.x++) {
        for (pix.y = bound.min.y; pix.y < bound.max.y; pix.y++) {
            Vec3 bCoords = ComputeBarycentricCoords(verts, pix);
            if (bCoords.x >= 0.0f && bCoords.y >= 0.0f && bCoords.z >= 0.0f) {
                pix.z = (int)(verts[2].z * bCoords.x
                    + verts[0].z * bCoords.y
                    + verts[1].z * bCoords.z);
                pix.z = ClampInt(pix.z, 0, INT_MAX);
                Vec3 color = colors[2] * bCoords.x
                    + colors[0] * bCoords.y
                    + colors[1] * bCoords.z;
                color.r = ClampFloat32(color.r, 0.0f, 1.0f);
                color.g = ClampFloat32(color.g, 0.0f, 1.0f);
                color.b = ClampFloat32(color.b, 0.0f, 1.0f);
                SetPixelColor(backbuffer, pix, color);
            }
        }
    }
}

void RenderTrianglePhong(GameBackbuffer* backbuffer,
    Vec3Int verts[3], Vec3 camVerts[3], Vec3 camNormals[3],
    Vec3 cameraPos, Vec3 lightPos, Material material)
{
    BoundingBox bound = ComputeTriangleBoundingBox(verts);
    bound.min.x = MaxInt(bound.min.x, 0);
    bound.max.x = MinInt(bound.max.x, backbuffer->width - 1);
    bound.min.y = MaxInt(bound.min.y, 0);
    bound.max.y = MinInt(bound.max.y, backbuffer->height - 1);
    Vec3Int pix;
    for (pix.x = bound.min.x; pix.x < bound.max.x; pix.x++) {
        for (pix.y = bound.min.y; pix.y < bound.max.y; pix.y++) {
            Vec3 bCoords = ComputeBarycentricCoords(verts, pix);
            if (bCoords.x >= 0.0f && bCoords.y >= 0.0f && bCoords.z >= 0.0f) {
                pix.z = (int)(verts[2].z * bCoords.x
                    + verts[0].z * bCoords.y
                    + verts[1].z * bCoords.z);
                pix.z = ClampInt(pix.z, 0, INT_MAX);
                Vec3 vert = camVerts[2] * bCoords.x
                    + camVerts[0] * bCoords.y
                    + camVerts[1] * bCoords.z;
                Vec3 normal = camNormals[2] * bCoords.x
                    + camNormals[0] * bCoords.y
                    + camNormals[1] * bCoords.z;
                //debug
                float32 mag = Mag(normal);
                DEBUG_ASSERT(mag < 1.0f - 0.001f || mag > 1.0f + 0.001f);
                //enddebug
                Vec3 color = CalculatePhongColor(vert, normal,
                    cameraPos, lightPos, material);
                SetPixelColor(backbuffer, pix, color);
            }
        }
    }
}

Vec3 CalculatePhongColor(Vec3 vertex, Vec3 normal,
    Vec3 cameraPos, Vec3 lightPos, Material material)
{
    Vec3 color = Vec3::zero;

    // Ambient
    color += material.ambient;

    // Diffuse
    Vec3 light = Normalize(lightPos - vertex);
    float32 dotLN = MaxFloat32(Dot(light, normal), 0.0f);
    color += material.diffuse * dotLN;

    // Specular
    Vec3 view = Normalize(cameraPos - vertex);
    Vec3 reflected = -light - 2.0f * Dot(-light, normal) * normal;
    float32 dotVR = MaxFloat32(Dot(view, reflected), 0.0f);
    color += material.specular * powf(dotVR, (float32)material.shininess);

    color.r = ClampFloat32(color.r, 0.0f, 1.0f);
    color.g = ClampFloat32(color.g, 0.0f, 1.0f);
    color.b = ClampFloat32(color.b, 0.0f, 1.0f);

    return color;
}