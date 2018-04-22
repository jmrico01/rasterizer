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

void RenderOverwriteColorBitmap(
    GameBackbuffer* backbuffer, Vec2Int pos,
    Bitmap* bitmap)
{
    uint8* backbufferData = (uint8*)backbuffer->data;
    int backbufferBytesPerRow = backbuffer->width * backbuffer->bytesPerPixel;

    for (int j = 0; j < bitmap->height; j++) {
        for (int i = 0; i < bitmap->width; i++) {
            if (pos.x + i < 0 || pos.x + i >= backbuffer->width) {
                continue;
            }
            if (pos.y + j < 0 || pos.y + j >= backbuffer->height) {
                continue;
            }
            int backbufferOff = (pos.y + j) * backbufferBytesPerRow
                + (pos.x + i) * backbuffer->bytesPerPixel;
            int bitmapOff = j * bitmap->width * bitmap->bytesPerPixel
                + i * bitmap->bytesPerPixel;
            uint8 b = bitmap->data[bitmapOff];
            uint8 g = bitmap->data[bitmapOff + 1];
            uint8 r = bitmap->data[bitmapOff + 2];
            uint32* pixel = (uint32*)(backbufferData + backbufferOff);
            *pixel = (255 << 24) | (r << 16) | (g << 8) | b;
        }
    }
}

// NOTE: it is possible for the bounding box min to be greater than max
internal BoundingBox ComputeClampedTriangleBoundingBox(
    Vec2Int v0, Vec2Int v1, Vec2Int v2, int minX, int maxX, int minY, int maxY)
{
    BoundingBox bb;
    bb.min.x = MaxInt(MinInt(MinInt(v0.x, v1.x), v2.x), minX);
    bb.min.y = MaxInt(MinInt(MinInt(v0.y, v1.y), v2.y), minY);
    bb.max.x = MinInt(MaxInt(MaxInt(v0.x, v1.x), v2.x), maxX);
    bb.max.y = MinInt(MaxInt(MaxInt(v0.y, v1.y), v2.y), maxY);
    return bb;
}

internal inline float32 ComputeTriangleSignedArea(
    Vec2Int v0, Vec2Int v1, Vec2Int v2)
{
    return 0.5f * (
        (v1.x - v0.x) * (v2.y - v0.y)
        - (v2.x - v0.x) * (v1.y - v0.y));
}

internal inline Vec3 ComputeBarycentricCoords(
    Vec2Int v0, Vec2Int v1, Vec2Int v2, Vec2Int p)
{
    Vec3 result;
    float32 area = ComputeTriangleSignedArea(v0, v1, v2);
    result.x = ComputeTriangleSignedArea(v0, v1, p);
    result.x /= area;
    result.y = ComputeTriangleSignedArea(v1, v2, p);
    result.y /= area;
    result.z = ComputeTriangleSignedArea(v2, v0, p);
    result.z /= area;
    return result;
}

// True if the pixel+depth is closer to the camera than the current depth
internal inline bool32 TestPixelDepth(GameBackbuffer* backbuffer,
    Vec2Int pix, uint32 depth)
{
    return depth >= backbuffer->depth[pix.y * backbuffer->width + pix.x];
}

internal void SetPixelColor(GameBackbuffer* backbuffer,
    Vec2Int pix, uint32 depth, Vec3 color)
{
    DEBUG_ASSERT(0 <= pix.x && pix.x < backbuffer->width);
    DEBUG_ASSERT(0 <= pix.y && pix.y < backbuffer->height);
    COLOR_VEC3_TO_UINT8(color, r, g, b);

    uint8* backbufferData = (uint8*)backbuffer->data;
    uint32* pixel = (uint32*)(backbufferData +
        pix.y * backbuffer->width * backbuffer->bytesPerPixel
        + pix.x * backbuffer->bytesPerPixel);
    *pixel = (r << 16) | (g << 8) | b;
    int ind = pix.y * backbuffer->width + pix.x;
    backbuffer->depth[ind] = depth;
}

// Point sampling
internal Vec3 SampleBitmap(const Bitmap* bitmap, Vec2 uv)
{
    int i = ClampInt((int)(uv.x * bitmap->width), 0, bitmap->width - 1);
    int j = ClampInt((int)(uv.y * bitmap->height), 0, bitmap->height - 1);
    int ind = j * bitmap->width * bitmap->bytesPerPixel
        + i * bitmap->bytesPerPixel;
    uint8 b = bitmap->data[ind];
    uint8 g = bitmap->data[ind + 1];
    uint8 r = bitmap->data[ind + 2];
    return Vec3 {
        (float32)r / 255.0f,
        (float32)g / 255.0f,
        (float32)b / 255.0f
    };
}
internal Vec3 SampleNormalMap(const Bitmap* normalMap, Vec2 uv)
{
    int i = ClampInt((int)(uv.x * normalMap->width), 0, normalMap->width - 1);
    int j = ClampInt((int)(uv.y * normalMap->height), 0, normalMap->height - 1);
    int ind = j * normalMap->width * normalMap->bytesPerPixel
        + i * normalMap->bytesPerPixel;
    uint8 b = normalMap->data[ind];
    uint8 g = normalMap->data[ind + 1];
    uint8 r = normalMap->data[ind + 2];
    float32 mag = sqrtf((float32)(b*b + g*g + r*r));
    return Vec3 {
        (float32)r / mag,
        (float32)g / mag,
        (float32)b / mag
    };
}

void RenderTrianglesWire(GameBackbuffer* backbuffer,
    const MeshScratch* scratch,
    Vec3 color)
{
    for (int t = 0; t < scratch->numTriangles; t++) {
        TriangleRenderInfo triangle = scratch->triangles[t];
        Vec2 verts[3];
        verts[0] = ToVec2(triangle.screenPos[0]);
        verts[1] = ToVec2(triangle.screenPos[1]);
        verts[2] = ToVec2(triangle.screenPos[2]);
        Vec2 edges[3];
        edges[0] = verts[1] - verts[0];
        edges[1] = verts[2] - verts[1];
        edges[2] = verts[0] - verts[2];
        for (int i = 0; i < 3; i++) {
            float32 edgeLen = Mag(edges[i]);
            edges[i] /= edgeLen;
            for (float l = 0; l < edgeLen; l += 0.5f) {
                Vec2Int pix;
                pix.x = RoundFloat32((float32)verts[i].x + edges[i].x * l);
                pix.y = RoundFloat32((float32)verts[i].y + edges[i].y * l);
                if (0 <= pix.x && pix.x < backbuffer->width
                && 0 <= pix.y && pix.y < backbuffer->height) {
                    SetPixelColor(backbuffer, pix, UINT32_MAX, color);
                }
                else {
                    break;
                }
            }
        }
    }
}

void RenderTrianglesFlat(GameBackbuffer* backbuffer,
    const MeshScratch* scratch,
    Vec3 cameraPos, Vec3 lightPos, Material material)
{
    for (int t = 0; t < scratch->numTriangles; t++) {
        TriangleRenderInfo triangle = scratch->triangles[t];
        Vec3 centroid = (triangle.pos[0]
            + triangle.pos[1]
            + triangle.pos[2]) / 3.0f;
        Vec3 normal = (triangle.normal[0]
            + triangle.normal[1]
            + triangle.normal[2]) / 3.0f;
        Vec3 color = CalculatePhongColor(centroid, normal,
            cameraPos, lightPos, material);
        
        BoundingBox bound = ComputeClampedTriangleBoundingBox(
            triangle.screenPos[0], triangle.screenPos[1], triangle.screenPos[2],
            0, backbuffer->width - 1, 0, backbuffer->height - 1);
        Vec2Int pix;
        for (pix.x = bound.min.x; pix.x < bound.max.x; pix.x++) {
            for (pix.y = bound.min.y; pix.y < bound.max.y; pix.y++) {
                Vec3 bCoords = ComputeBarycentricCoords(
                    triangle.screenPos[0], triangle.screenPos[1],
                    triangle.screenPos[2], pix);
                if (0.0f <= bCoords.x && bCoords.x <= 1.0f
                && 0.0f <= bCoords.y && bCoords.y <= 1.0f
                && 0.0f <= bCoords.z && bCoords.z <= 1.0f) {
                    uint32 depth = (uint32)(triangle.depth[2] * bCoords.x
                        + triangle.depth[0] * bCoords.y
                        + triangle.depth[1] * bCoords.z);
                    depth = ClampUInt32(depth, 0, UINT32_MAX);
                    if (!TestPixelDepth(backbuffer, pix, depth)) {
                        continue;
                    }

                    SetPixelColor(backbuffer, pix, (uint32)depth, color);
                }
            }
        }
    }
}

void RenderTrianglesGouraud(GameBackbuffer* backbuffer,
    const MeshScratch* scratch,
    Vec3 cameraPos, Vec3 lightPos, Material material)
{
    Vec3 colors[3];
    for (int t = 0; t < scratch->numTriangles; t++) {
        TriangleRenderInfo triangle = scratch->triangles[t];
        colors[0] = CalculatePhongColor(triangle.pos[0], triangle.normal[0],
            cameraPos, lightPos, material);
        colors[1] = CalculatePhongColor(triangle.pos[1], triangle.normal[1],
            cameraPos, lightPos, material);
        colors[2] = CalculatePhongColor(triangle.pos[2], triangle.normal[2],
            cameraPos, lightPos, material);
        
        BoundingBox bound = ComputeClampedTriangleBoundingBox(
            triangle.screenPos[0], triangle.screenPos[1], triangle.screenPos[2],
            0, backbuffer->width - 1, 0, backbuffer->height - 1);
        Vec2Int pix;
        for (pix.x = bound.min.x; pix.x < bound.max.x; pix.x++) {
            for (pix.y = bound.min.y; pix.y < bound.max.y; pix.y++) {
                Vec3 bCoords = ComputeBarycentricCoords(
                    triangle.screenPos[0], triangle.screenPos[1],
                    triangle.screenPos[2], pix);
                if (0.0f <= bCoords.x && bCoords.x <= 1.0f
                && 0.0f <= bCoords.y && bCoords.y <= 1.0f
                && 0.0f <= bCoords.z && bCoords.z <= 1.0f) {
                    uint32 depth = (uint32)(triangle.depth[2] * bCoords.x
                        + triangle.depth[0] * bCoords.y
                        + triangle.depth[1] * bCoords.z);
                    depth = ClampUInt32(depth, 0, UINT32_MAX);
                    if (!TestPixelDepth(backbuffer, pix, depth)) {
                        continue;
                    }

                    Vec3 color = colors[2] * bCoords.x
                        + colors[0] * bCoords.y
                        + colors[1] * bCoords.z;
                    SetPixelColor(backbuffer, pix, (uint32)depth, color);
                }
            }
        }
    }
}

void RenderTrianglesPhong(GameBackbuffer* backbuffer,
    const MeshScratch* scratch,
    Vec3 cameraPos, Vec3 lightPos, Material material)
{
    for (int t = 0; t < scratch->numTriangles; t++) {
        TriangleRenderInfo triangle = scratch->triangles[t];

        BoundingBox bound = ComputeClampedTriangleBoundingBox(
            triangle.screenPos[0], triangle.screenPos[1], triangle.screenPos[2],
            0, backbuffer->width - 1, 0, backbuffer->height - 1);
        Vec2Int pix;
        for (pix.x = bound.min.x; pix.x < bound.max.x; pix.x++) {
            for (pix.y = bound.min.y; pix.y < bound.max.y; pix.y++) {
                Vec3 bCoords = ComputeBarycentricCoords(
                    triangle.screenPos[0], triangle.screenPos[1],
                    triangle.screenPos[2], pix);
                if (0.0f <= bCoords.x && bCoords.x <= 1.0f
                && 0.0f <= bCoords.y && bCoords.y <= 1.0f
                && 0.0f <= bCoords.z && bCoords.z <= 1.0f) {
                    uint32 depth = (uint32)(triangle.depth[2] * bCoords.x
                        + triangle.depth[0] * bCoords.y
                        + triangle.depth[1] * bCoords.z);
                    depth = ClampUInt32(depth, 0, UINT32_MAX);
                    if (!TestPixelDepth(backbuffer, pix, depth)) {
                        continue;
                    }

                    Vec3 vert = triangle.pos[2] * bCoords.x
                        + triangle.pos[0] * bCoords.y
                        + triangle.pos[1] * bCoords.z;
                    Vec3 normal = triangle.normal[2] * bCoords.x
                            + triangle.normal[0] * bCoords.y
                            + triangle.normal[1] * bCoords.z;
                    Vec3 color = CalculatePhongColor(vert, normal,
                        cameraPos, lightPos, material);
                    SetPixelColor(backbuffer, pix, (uint32)depth, color);
                }
            }
        }
    }
}

void RenderTrianglesPhongTextured(GameBackbuffer* backbuffer,
    const MeshScratch* scratch,
    Bitmap* diffuseMap, Bitmap* specularMap, Bitmap* normalMap,
    Vec3 cameraPos, Vec3 lightPos, Material material)
{
    for (int t = 0; t < scratch->numTriangles; t++) {
        TriangleRenderInfo triangle = scratch->triangles[t];

        BoundingBox bound = ComputeClampedTriangleBoundingBox(
            triangle.screenPos[0], triangle.screenPos[1], triangle.screenPos[2],
            0, backbuffer->width - 1, 0, backbuffer->height - 1);
        Vec2Int pix;
        for (pix.x = bound.min.x; pix.x < bound.max.x; pix.x++) {
            for (pix.y = bound.min.y; pix.y < bound.max.y; pix.y++) {
                Vec3 bCoords = ComputeBarycentricCoords(
                    triangle.screenPos[0], triangle.screenPos[1],
                    triangle.screenPos[2], pix);
                if (0.0f <= bCoords.x && bCoords.x <= 1.0f
                && 0.0f <= bCoords.y && bCoords.y <= 1.0f
                && 0.0f <= bCoords.z && bCoords.z <= 1.0f) {
                    uint32 depth = (uint32)(triangle.depth[2] * bCoords.x
                        + triangle.depth[0] * bCoords.y
                        + triangle.depth[1] * bCoords.z);
                    depth = ClampUInt32(depth, 0, UINT32_MAX);
                    if (!TestPixelDepth(backbuffer, pix, depth)) {
                        continue;
                    }

                    Vec3 vert = triangle.pos[2] * bCoords.x
                        + triangle.pos[0] * bCoords.y
                        + triangle.pos[1] * bCoords.z;
                    Vec2 uv = triangle.uv[2] * bCoords.x
                        + triangle.uv[0] * bCoords.y
                        + triangle.uv[1] * bCoords.z;
                    
                    material.diffuse = SampleBitmap(diffuseMap, uv);
                    material.specular = SampleBitmap(specularMap, uv);
                    Vec3 normal;
                    if (normalMap == nullptr) {
                        normal = triangle.normal[2] * bCoords.x
                            + triangle.normal[0] * bCoords.y
                            + triangle.normal[1] * bCoords.z;
                    }
                    else {
                        normal = SampleNormalMap(normalMap, uv);
                    }
                    Vec3 color = CalculatePhongColor(vert, normal,
                        cameraPos, lightPos, material);
                    SetPixelColor(backbuffer, pix, (uint32)depth, color);
                }
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
    float32 powDotVR = 1.0f;
    for (int i = 0; i < material.shininess; i++) {
        powDotVR *= dotVR;
    }
    color += material.specular * powDotVR;

    color.r = ClampFloat32(color.r, 0.0f, 1.0f);
    color.g = ClampFloat32(color.g, 0.0f, 1.0f);
    color.b = ClampFloat32(color.b, 0.0f, 1.0f);

    return color;
}