#pragma once

#include "km_lib.h"
#include "km_math.h"
#include "main_platform.h"

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

Mesh LoadMeshFromObj(ThreadContext* thread,
    const char* fileName,
    DEBUGPlatformReadFileFunc* DEBUGPlatformReadFile,
    DEBUGPlatformFreeFileMemoryFunc* DEBUGPlatformFreeFileMemory);

void RenderMeshWire(const Mesh& mesh, Mat4 mvp,
    GameBackbuffer* backbuffer);
void RenderMeshFlat(const Mesh& mesh,
    Mat4 model, Mat4 view, Mat4 proj,
    Vec3 cameraPos, Vec3 lightPos, Material material,
    GameBackbuffer* backbuffer);
void RenderMeshGouraud(const Mesh& mesh,
    Mat4 model, Mat4 view, Mat4 proj,
    Vec3 cameraPos, Vec3 lightPos, Material material,
    GameBackbuffer* backbuffer);
void RenderMeshPhong(const Mesh& mesh,
    Mat4 model, Mat4 view, Mat4 proj,
    Vec3 cameraPos, Vec3 lightPos, Material material,
    GameBackbuffer* backbuffer);

void FreeMesh(Mesh* mesh);

/*HalfEdgeMesh HalfEdgeMeshFromObj(const char* fileName);
HalfEdgeMesh CopyHalfEdgeMesh(const HalfEdgeMesh& mesh);
void FreeHalfEdgeMesh(HalfEdgeMesh* mesh);

void PrintHalfEdgeMesh(const HalfEdgeMesh& mesh);
void PrintEdgeEndpoints(const HalfEdgeMesh& mesh, uint32 e);
void PrintHalfEdgeMeshFaces(const HalfEdgeMesh& mesh);

HalfEdgeMeshGL LoadHalfEdgeMeshGL(const HalfEdgeMesh& mesh, bool smoothNormals);
void FreeHalfEdgeMeshGL(HalfEdgeMeshGL* meshGL);

void DrawHalfEdgeMeshGL(const HalfEdgeMeshGL& meshGL, Mat4 proj, Mat4 view);

// Utility functions
void RemoveVertex(HalfEdgeMesh* mesh, uint32 v);
void RemoveEdge(HalfEdgeMesh* mesh, uint32 e);
void RemoveFace(HalfEdgeMesh* mesh, uint32 f);

uint32 SplitEdgeMakeVertex(HalfEdgeMesh* mesh, uint32 e, float t);
uint32 SplitFaceMakeEdge(HalfEdgeMesh* mesh, uint32 f, uint32 v1, uint32 v2);
void TriangulateMesh(HalfEdgeMesh* mesh);
void ComputeFaceNormals(HalfEdgeMesh* mesh);
void ComputeFaceAreas(HalfEdgeMesh* mesh);
void ComputeVertexNormals(HalfEdgeMesh* mesh);
void ComputeVertexAvgEdgeLengths(HalfEdgeMesh* mesh);

struct MouseCastFaceOut
{
    uint32 face;
    float dist;
};
void MouseCastMeshFaces(const HalfEdgeMesh& mesh, Vec2 mousePos,
    Mat4 proj, Mat4 view,
    int screenWidth, int screenHeight,
    DynamicArray<MouseCastFaceOut>& out);

// Mesh traversal functions
void VerticesOnFace(const HalfEdgeMesh& mesh, uint32 f,
    DynamicArray<uint32>& out);
void EdgesOnFace(const HalfEdgeMesh& mesh, uint32 f,
    DynamicArray<uint32>& out);
void FacesOnFace(const HalfEdgeMesh& mesh, uint32 f,
    DynamicArray<uint32>& out);

void VerticesOnEdge(const HalfEdgeMesh& mesh, uint32 e,
    DynamicArray<uint32>& out);
void FacesOnEdge(const HalfEdgeMesh& mesh, uint32 e,
    DynamicArray<uint32>& out);

void VerticesOnVertex(const HalfEdgeMesh& mesh, uint32 v,
    DynamicArray<uint32>& out);
void EdgesOnVertex(const HalfEdgeMesh& mesh, uint32 v,
    DynamicArray<uint32>& out);
void FacesOnVertex(const HalfEdgeMesh& mesh, uint32 v,
    DynamicArray<uint32>& out);*/