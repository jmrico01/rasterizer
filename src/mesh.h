#pragma once

#include "km_lib.h"
#include "km_math.h"
#include "load_bmp.h"
#include "main_platform.h"

#define MAX_TRIANGLES 500000

struct Material
{
    Vec3 ambient;
    Vec3 diffuse;
    Vec3 specular;
    int shininess;
};

struct Triangle
{
    Vec3 v[3];
    Vec2 uv[3];
    Vec3 n[3];
};

struct Mesh
{
    DynamicArray<Triangle> triangles;
};

struct TriangleRenderInfo
{
    Vec2Int screenPos[3];
    uint32 depth[3];
    Vec3 pos[3];
    Vec2 uv[3];
    Vec3 normal[3];
};

// Memory used by shaders for scratch work in rendering
struct MeshScratch
{
    int numTriangles;
    TriangleRenderInfo triangles[MAX_TRIANGLES];
};

Mesh LoadMeshFromObj(ThreadContext* thread,
    const char* fileName,
    DEBUGPlatformReadFileFunc* DEBUGPlatformReadFile,
    DEBUGPlatformFreeFileMemoryFunc* DEBUGPlatformFreeFileMemory);

void RenderMeshWire(const Mesh& mesh,
    Mat4 model, Mat4 view, Mat4 proj, Vec3 color,
    GameBackbuffer* backbuffer, MeshScratch* scratch);
void RenderMeshFlat(const Mesh& mesh,
    Mat4 model, Mat4 view, Mat4 proj,
    bool32 backfaceCulling,
    Vec3 cameraPos, Vec3 lightPos, Material material,
    GameBackbuffer* backbuffer, MeshScratch* scratch);
void RenderMeshGouraud(const Mesh& mesh,
    Mat4 model, Mat4 view, Mat4 proj,
    bool32 backfaceCulling,
    Vec3 cameraPos, Vec3 lightPos, Material material,
    GameBackbuffer* backbuffer, MeshScratch* scratch);
void RenderMeshPhong(const Mesh& mesh,
    Mat4 model, Mat4 view, Mat4 proj,
    bool32 backfaceCulling,
    Vec3 cameraPos, Vec3 lightPos, Material material,
    GameBackbuffer* backbuffer, MeshScratch* scratch);
void RenderMeshPhongTextured(const Mesh& mesh,
    Mat4 model, Mat4 view, Mat4 proj,
    bool32 backfaceCulling,
    Vec3 cameraPos, Vec3 lightPos, Material material,
    Bitmap* diffuseMap, Bitmap* specularMap, Bitmap* normalMap,
    GameBackbuffer* backbuffer, MeshScratch* scratch);

void FreeMesh(Mesh* mesh);
