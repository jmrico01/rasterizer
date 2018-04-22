#include "mesh.h"

#include <stdio.h>
#include <map>

#include "km_math.h"
#include "km_debug.h"

#define OBJ_LINE_MAX 512

struct Vertex
{
    uint32 halfEdge;
    Vec3 pos;
    Vec3 normal;
    Vec3 color;
    float32 avgEdgeLength;
};

struct Face
{
    uint32 halfEdge;
    Vec3 normal;
    float32 area;
};

struct HalfEdge
{
    uint32 next;
    uint32 twin;
    uint32 vertex;
    uint32 face;
};

struct HalfEdgeMesh
{
    DynamicArray<Vertex> vertices;
    DynamicArray<Face> faces;
    DynamicArray<HalfEdge> halfEdges;
};

internal int GetNextLine(const char* src, char* dst, int dstLen)
{
    int read = 0;
    const char* s = src;
    while (*s != '\n' && *s != '\0' && read < dstLen - 1) {
        dst[read++] = *(s++);
    }
    dst[read] = '\0';

    if (read == 0 && *s == '\0') {
        return -1;
    }
    return read;
}

internal Vec3 ParseVec3(char* str)
{
    Vec3 result = { 0.0f, 0.0f, 0.0f };
    char* el = str;
    int i = 0;
    for (char* c = str; *c != '\0'; c++) {
        if (*c == ' ' || *c == '\n') {
            char* endptr = el;
            *c = '\0';
            result.e[i++] = (float32)strtod(el, &endptr);
            if (i == 3) {
                break;
            }
            // TODO: check error
            el = c + 1;
        }
    }

    return result;
}

internal Vec2 ParseVec2(char* str)
{
    Vec2 result = { 0.0f, 0.0f };
    char* el = str;
    while (*el == ' ') {
        el++;
    }
    int i = 0;
    for (char* c = str; *c != '\0'; c++) {
        if (*c == ' ' || *c == '\n') {
            char* endptr = el;
            *c = '\0';
            result.e[i++] = (float32)strtod(el, &endptr);
            if (i == 2) {
                break;
            }
            // TODO: check error
            el = c + 1;
        }
    }

    return result;
}

internal Vec3 NormalFromTriangle(Vec3 v0, Vec3 v1, Vec3 v2)
{
    Vec3 a = v1 - v0;
    Vec3 b = v2 - v0;
    return Normalize(Cross(a, b));
}
internal Vec3 NormalFromTriangle(const Vec3 triangle[3])
{
    Vec3 a = triangle[1] - triangle[0];
    Vec3 b = triangle[2] - triangle[0];
    return Normalize(Cross(a, b));
}

internal void FacesOnVertex(const HalfEdgeMesh& mesh, uint32 v,
    DynamicArray<uint32>& out)
{
    uint32 edge = mesh.vertices[v].halfEdge;
    uint32 e = edge;
    do {
        out.Append(mesh.halfEdges[e].face);
        uint32 twin = mesh.halfEdges[e].twin;
        e = mesh.halfEdges[twin].next;
    } while (e != edge);
}

// Computes normal for the face f, assuming all vertices in f are coplanar.
internal Vec3 ComputeFaceNormal(const HalfEdgeMesh& mesh, uint32 f)
{
    Vec3 triangle[3];
    int numEdges = 0;
    uint32 edge = mesh.faces[f].halfEdge;
    uint32 e = edge;
    do {
        triangle[numEdges++] = mesh.vertices[mesh.halfEdges[e].vertex].pos;
        if (numEdges >= 3) {
            break;
        }
        e = mesh.halfEdges[e].next;
    } while (e != edge);

    return NormalFromTriangle(triangle);
}
internal void ComputeFaceNormals(HalfEdgeMesh* mesh)
{
    for (uint32 f = 0; f < mesh->faces.size; f++) {
        Vec3 normal = ComputeFaceNormal((const HalfEdgeMesh&)*mesh, f);
        mesh->faces[f].normal = normal;
    }
}

internal void ComputeVertexNormals(HalfEdgeMesh* mesh)
{
    DynamicArray<uint32> faces;
    faces.Init();
    for (uint32 v = 0; v < mesh->vertices.size; v++) {
        Vec3 sumNormals = Vec3::zero;
        FacesOnVertex((const HalfEdgeMesh&)*mesh, v, faces);
        for (uint32 i = 0; i < faces.size; i++) {
            sumNormals += mesh->faces[faces[i]].normal;
        }
        mesh->vertices[v].normal = Normalize(sumNormals);

        faces.Clear();
    }

    faces.Free();
}

internal HalfEdgeMesh HalfEdgeMeshFromObj(ThreadContext* thread,
    const char* fileName,
    DEBUGPlatformReadFileFunc* DEBUGPlatformReadFile,
    DEBUGPlatformFreeFileMemoryFunc* DEBUGPlatformFreeFileMemory)
{
    HalfEdgeMesh mesh;
    mesh.vertices.Init();
    mesh.faces.Init();
    mesh.halfEdges.Init();

    DEBUGReadFileResult objFile = DEBUGPlatformReadFile(thread, fileName);
    if (!objFile.data) {
        DEBUG_PRINT("Failed to open OBJ file at: %s\n", fileName);
        return mesh;
    }

    char* fileStr = (char*)objFile.data;
    int read;
    char line[OBJ_LINE_MAX];
    DynamicArray<int> faceInds;
    faceInds.Init();

    while ((read = GetNextLine(fileStr, line, OBJ_LINE_MAX)) >= 0) {
        fileStr += read + 1;
        // TODO: I'm too lazy to fix this
        line[read] = '\n';
        line[read + 1] = '\0';
        //DEBUG_PRINT("line: %s", line);
        if (line[0] == '#') {
            continue;
        }
        else if (line[0] == 'v') {
            if (line[1] == ' ') {
                Vertex vertex;
                vertex.pos = ParseVec3(&line[2]);
                vertex.halfEdge = 0; // This is set later
                vertex.color = Vec3::one;
                mesh.vertices.Append(vertex);
            }
            else {
                // idk
            }
        }
        else if (line[0] == 'f') {
            char* el = &line[2];
            for (char* c = &line[2]; *c != '\0'; c++) {
                if (*c == ' ' || *c == '\n') {
                    char* endptr = el;
                    *c = '\0';
                    faceInds.Append((int)strtol(el, &endptr, 10));
                    // TODO check error
                    el = c + 1;
                }
            }
            faceInds.Append(-1);
        }
        else {
            // idk
        }
    }

    std::map<std::pair<uint32, uint32>, uint32> edgeMap;

    uint32 faceVerts = 0;
    for (uint32 i = 0; i < faceInds.size; i++) {
        if (faceInds[i] == -1 || i == faceInds.size - 1) {
            // Create face
            Face face;
            face.halfEdge = mesh.halfEdges.size - 1;
            mesh.faces.Append(face);
            
            faceVerts = 0;
            continue;
        }

        uint32 vertSrc = faceInds[i] - 1;
        uint32 vertDst = faceInds[i + 1] - 1;
        if (faceInds[i + 1] == -1) {
            vertDst = faceInds[i - faceVerts] - 1;
        }

        // Create edge, i -> i + 1
        // Last edge will have wrong "next"
        std::pair<int, int> forward(vertSrc, vertDst);
        std::pair<int, int> backward(vertDst, vertSrc);
        HalfEdge he;
        he.next = mesh.halfEdges.size + 1;
        if (faceInds[i + 1] == -1) {
            he.next = mesh.halfEdges.size - faceVerts;
        }
        auto edgeBackward = edgeMap.find(backward);
        if (edgeBackward != edgeMap.end()) {
            he.twin = edgeBackward->second;
            mesh.halfEdges[edgeBackward->second].twin =
                mesh.halfEdges.size;
        }
        else {
            he.twin = 0;
        }
        he.vertex = vertDst;
        he.face = mesh.faces.size;

        // Assign forward edge i -> i + 1 to vertex i
        mesh.vertices[vertSrc].halfEdge = mesh.halfEdges.size;

        // Add edge to edgeMap and mesh.halfEdges array
        auto res = edgeMap.insert(
            std::make_pair(forward, mesh.halfEdges.size));
        if (!res.second) {
            DEBUG_PRINT("ERROR: Edge already in edgeMap\n");
        }
        mesh.halfEdges.Append(he);
        faceVerts++;
    }

    faceInds.Free();
    ComputeFaceNormals(&mesh);
    ComputeVertexNormals(&mesh);

    DEBUGPlatformFreeFileMemory(thread, &objFile);
    
    return mesh;
}

internal void FreeHalfEdgeMesh(HalfEdgeMesh* mesh)
{
    mesh->vertices.Free();
    mesh->faces.Free();
    mesh->halfEdges.Free();
}

Mesh LoadMeshFromObj(ThreadContext* thread,
    const char* fileName,
    DEBUGPlatformReadFileFunc* DEBUGPlatformReadFile,
    DEBUGPlatformFreeFileMemoryFunc* DEBUGPlatformFreeFileMemory)
{
    Mesh mesh;
    mesh.triangles.Init();

    DEBUGReadFileResult objFile = DEBUGPlatformReadFile(thread, fileName);
    if (!objFile.data) {
        DEBUG_PRINT("Failed to open OBJ file at: %s\n", fileName);
        return mesh;
    }

    char* fileStr = (char*)objFile.data;
    int read;
    char line[OBJ_LINE_MAX];
    DynamicArray<Vec3> vertices;
    vertices.Init();
    DynamicArray<Vec2> uvs;
    uvs.Init();
    DynamicArray<Vec3> normals;
    normals.Init();

    DynamicArray<int> faceVertInds;
    faceVertInds.Init();
    DynamicArray<int> faceUVInds;
    faceUVInds.Init();
    DynamicArray<int> faceNormInds;
    faceNormInds.Init();

    while ((read = GetNextLine(fileStr, line, OBJ_LINE_MAX)) >= 0) {
        fileStr += read + 1;
        // TODO: I'm too lazy to fix this
        line[read] = '\n';
        line[read + 1] = '\0';
        //DEBUG_PRINT("line: %s", line);
        if (line[0] == '#') {
            continue;
        }
        else if (line[0] == 'v') {
            if (line[1] == ' ') {
                int start = 2;
                while (line[start] == ' ') {
                    start++;
                }
                Vec3 v = ParseVec3(&line[start]);
                vertices.Append(v);
            }
            else if (line[1] == 't') {
                int start = 3;
                while (line[start] == ' ') {
                    start++;
                }
                Vec2 uv = ParseVec2(&line[start]);
                // TODO: uvs are flipped vertically here
                // BMPs should probably be flipped vertically instead
                uv.y = 1.0f - uv.y;
                uvs.Append(uv);
            }
            else if (line[1] == 'n') {
                int start = 3;
                while (line[start] == ' ') {
                    start++;
                }
                Vec3 n = ParseVec3(&line[start]);
                normals.Append(Normalize(n));
            }
            else {
                // idk
            }
        }
        else if (line[0] == 'f') {
            char* vert = &line[2];
            char* uv = &line[2];
            char* norm = &line[2];
            for (char* c = &line[2]; *c != '\0'; c++) {
                if (*c == '/') {
                    if (uv == vert) {
                        uv = c + 1;
                        *c = '\0';
                    }
                    else if (norm == vert) {
                        norm = c + 1;
                        *c = '\0';
                    }
                    else {
                        // idk
                    }
                }
                else if (*c == ' ' || *c == '\n') {
                    *c = '\0';
                    char* endptr = vert; // for strtol
                    faceVertInds.Append((int)strtol(vert, &endptr, 10) - 1);
                    // TODO: check error
                    if (uv != vert) {
                        endptr = uv;
                        faceUVInds.Append((int)strtol(uv, &endptr, 10) - 1);
                    }
                    if (norm != vert) {
                        endptr = norm;
                        faceNormInds.Append((int)strtol(norm, &endptr, 10) - 1);
                    }

                    vert = c + 1;
                    uv = c + 1;
                    norm = c + 1;
                }
            }
            faceVertInds.Append(-1);
            faceUVInds.Append(-1);
            faceNormInds.Append(-1);
        }
        else {
            // idk
        }
    }

    bool computedVertexNormals = false;
    if (normals.size == 0) {
        computedVertexNormals = true;
        HalfEdgeMesh halfEdgeMesh = HalfEdgeMeshFromObj(thread, fileName,
            DEBUGPlatformReadFile,
            DEBUGPlatformFreeFileMemory);
        DEBUG_ASSERT(halfEdgeMesh.vertices.size == vertices.size);
        for (int v = 0; v < (int)halfEdgeMesh.vertices.size; v++) {
            normals.Append(halfEdgeMesh.vertices[v].normal);
        }
        FreeHalfEdgeMesh(&halfEdgeMesh);
    }

    DynamicArray<int> faceVerts;
    faceVerts.Init();
    DynamicArray<int> faceUVs;
    faceUVs.Init();
    DynamicArray<int> faceNormals;
    faceNormals.Init();
    DEBUG_ASSERT(vertices.size != uvs.size
        || faceVertInds.size == faceUVInds.size);
    /*DEBUG_ASSERT(vertices.size != normals.size
        || faceVertInds.size == faceNormInds.size);*/
    for (int i = 0; i < (int)faceVertInds.size; i++) {
        if (faceVertInds[i] == -1) {
            DEBUG_ASSERT(uvs.size == 0 || faceUVInds[i] == -1);
            //DEBUG_ASSERT(normals.size == 0 || faceNormInds[i] == -1);
            DEBUG_ASSERT(faceVerts.size >= 3);
            Vec3 flatNormal = NormalFromTriangle(
                vertices[faceVerts[0]],
                vertices[faceVerts[1]],
                vertices[faceVerts[2]]
            );
            for (int v = 1; v < (int)faceVerts.size - 1; v++) {
                Vec3 v1 = vertices[faceVerts[v]];
                Vec3 v2 = vertices[faceVerts[v + 1]];
                Triangle triangle;
                triangle.v[0] = vertices[faceVerts[0]];
                triangle.v[1] = vertices[faceVerts[v]];
                triangle.v[2] = vertices[faceVerts[v + 1]];
                if (uvs.size > 0) {
                    triangle.uv[0] = uvs[faceUVs[0]];
                    triangle.uv[1] = uvs[faceUVs[v]];
                    triangle.uv[2] = uvs[faceUVs[v + 1]];
                }
                else {
                    triangle.uv[0] = Vec2::zero;
                    triangle.uv[1] = Vec2::zero;
                    triangle.uv[2] = Vec2::zero;
                }
                if (!computedVertexNormals) {
                    triangle.n[0] = normals[faceNormals[0]];
                    triangle.n[1] = normals[faceNormals[v]];
                    triangle.n[2] = normals[faceNormals[v + 1]];
                }
                else {
                    triangle.n[0] = normals[faceVerts[0]];
                    triangle.n[1] = normals[faceVerts[v]];
                    triangle.n[2] = normals[faceVerts[v + 1]];
                }
                mesh.triangles.Append(triangle);
            }
            faceVerts.Clear();
            faceUVs.Clear();
            faceNormals.Clear();
        }
        else {
            faceVerts.Append(faceVertInds[i]);
            if (uvs.size > 0) {
                faceUVs.Append(faceUVInds[i]);
            }
            if (!computedVertexNormals) {
                faceNormals.Append(faceNormInds[i]);
            }
        }
    }
    faceVerts.Free();
    faceNormals.Free();
    faceUVs.Free();

    faceVertInds.Free();
    faceUVInds.Free();
    faceNormInds.Free();

    vertices.Free();
    uvs.Free();
    normals.Free();

    DEBUGPlatformFreeFileMemory(thread, &objFile);

    // NOTE: must free mesh after this
    return mesh;
}

internal bool IsCCW(Vec3Int triangle[3])
{
    Vec2 e01 = {
        (float32)triangle[1].x - triangle[0].x,
        (float32)triangle[1].y - triangle[0].y
    };
    Vec2 e02 = {
        (float32)triangle[2].x - triangle[0].x,
        (float32)triangle[2].y - triangle[0].y
    };
    e02 = { -e02.y, e02.x };
    return Dot(e01, e02) <= 0.0f;
}

internal bool TriangleWorldToScreen(
    const Vec3 world[3], Mat4 mvp, Vec3Int screen[3],
    bool32 backfaceCulling,
    GameBackbuffer* backbuffer)
{
    bool shouldRender = false;
    for (int i = 0; i < 3; i++) {
        Vec4 v4 = { world[i].x, world[i].y, world[i].z, 1.0f };
        v4 = mvp * v4;
        v4 /= v4.w;
        if (v4.z < -1.0f || v4.z > 1.0f) {
            return false;
        }

        screen[i] = {
            (int)((v4.x + 1.0f) / 2.0f * backbuffer->width),
            (int)((v4.y + 1.0f) / 2.0f * backbuffer->height),
            (int)((-v4.z + 1.0f) / 2.0f * INT_MAX)
        };
        if (0 <= screen[i].x
        && screen[i].x < backbuffer->width
        && 0 <= screen[i].y
        && screen[i].y < backbuffer->height) {
            shouldRender = true;
        }
    }

    if (backfaceCulling && !IsCCW(screen)) {
        return false;
    }

    return shouldRender;
}

void RenderMeshWire(const Mesh& mesh, Mat4 mvp,
    GameBackbuffer* backbuffer)
{
    Vec3Int triangleScreen[3];
    for (int t = 0; t < (int)mesh.triangles.size; t++) {
        bool shouldRender = TriangleWorldToScreen(
            mesh.triangles[t].v, mvp, triangleScreen, false,
            backbuffer);
        if (shouldRender) {
            RenderTriangleWire(backbuffer, triangleScreen,
                Vec3 { 1.0f, 0.0f, 0.0f });
        }
    }
}

void RenderMeshFlat(const Mesh& mesh,
    Mat4 model, Mat4 view, Mat4 proj,
    bool32 backfaceCulling,
    Vec3 cameraPos, Vec3 lightPos, Material material,
    GameBackbuffer* backbuffer)
{
    Mat4 mvp = proj * view * model;
    Vec3Int triangleScreen[3];
    for (int t = 0; t < (int)mesh.triangles.size; t++) {
        bool shouldRender = TriangleWorldToScreen(
            mesh.triangles[t].v, mvp, triangleScreen,
            backfaceCulling, backbuffer);
        if (shouldRender) {
            Vec3 centroid = (mesh.triangles[t].v[0]
                + mesh.triangles[t].v[1]
                + mesh.triangles[t].v[2]) / 3.0f;
            Vec3 normal = (mesh.triangles[t].n[0]
                + mesh.triangles[t].n[1]
                + mesh.triangles[t].n[2]) / 3.0f;
            centroid = ToVec3(model * ToVec4(centroid, 1.0f));
            normal = ToVec3(model * ToVec4(normal, 0.0f));
            Vec3 color = CalculatePhongColor(centroid, normal,
                cameraPos, lightPos, material);
            RenderTriangleFlat(backbuffer, triangleScreen, color);
        }
    }
}

void RenderMeshGouraud(const Mesh& mesh,
    Mat4 model, Mat4 view, Mat4 proj,
    bool32 backfaceCulling,
    Vec3 cameraPos, Vec3 lightPos, Material material,
    GameBackbuffer* backbuffer)
{
    Mat4 mvp = proj * view * model;
    Vec3Int triangleScreen[3];
    Vec3 vertColors[3];
    for (int t = 0; t < (int)mesh.triangles.size; t++) {
        bool shouldRender = TriangleWorldToScreen(
            mesh.triangles[t].v, mvp, triangleScreen,
            backfaceCulling, backbuffer);
        if (shouldRender) {
            Vec3 vert, norm;
            for (int v = 0; v < 3; v++) {
                vert = ToVec3(model * ToVec4(mesh.triangles[t].v[v], 1.0f));
                norm = ToVec3(model * ToVec4(mesh.triangles[t].n[v], 0.0f));
                vertColors[v] = CalculatePhongColor(vert, norm,
                    cameraPos, lightPos, material);
            }
            RenderTriangleGouraud(backbuffer, triangleScreen, vertColors);
        }
    }
}


void RenderMeshPhong(const Mesh& mesh,
    Mat4 model, Mat4 view, Mat4 proj,
    bool32 backfaceCulling,
    Vec3 cameraPos, Vec3 lightPos, Material material,
    GameBackbuffer* backbuffer)
{
    Mat4 mvp = proj * view * model;
    Vec3Int triangleScreen[3];
    Vec3 cameraVerts[3];
    Vec3 cameraNormals[3];
    for (int t = 0; t < (int)mesh.triangles.size; t++) {
        bool shouldRender = TriangleWorldToScreen(
            mesh.triangles[t].v, mvp, triangleScreen,
            backfaceCulling, backbuffer);
        if (shouldRender) {
            for (int v = 0; v < 3; v++) {
                cameraVerts[v] = ToVec3(model
                    * ToVec4(mesh.triangles[t].v[v], 1.0f));
                cameraNormals[v] = ToVec3(model
                    * ToVec4(mesh.triangles[t].n[v], 0.0f));
            }
            RenderTrianglePhong(backbuffer, triangleScreen,
                cameraVerts, cameraNormals,
                cameraPos, lightPos, material);
        }
    }
}

void RenderMeshPhong(const Mesh& mesh,
    Mat4 model, Mat4 view, Mat4 proj,
    bool32 backfaceCulling,
    Vec3 cameraPos, Vec3 lightPos, Material material,
    Bitmap* diffuseMap, Bitmap* specularMap, Bitmap* normalMap,
    GameBackbuffer* backbuffer)
{
    Mat4 mvp = proj * view * model;
    Vec3Int triangleScreen[3];
    Vec3 cameraVerts[3];
    Vec3 cameraNormals[3];
    for (int t = 0; t < (int)mesh.triangles.size; t++) {
        bool shouldRender = TriangleWorldToScreen(
            mesh.triangles[t].v, mvp, triangleScreen,
            backfaceCulling, backbuffer);
        if (shouldRender) {
            for (int v = 0; v < 3; v++) {
                cameraVerts[v] = ToVec3(model
                    * ToVec4(mesh.triangles[t].v[v], 1.0f));
                cameraNormals[v] = ToVec3(model
                    * ToVec4(mesh.triangles[t].n[v], 0.0f));
            }
            RenderTrianglePhong(backbuffer, triangleScreen,
                cameraVerts, mesh.triangles[t].uv, cameraNormals,
                diffuseMap, specularMap, normalMap,
                cameraPos, lightPos, material);
        }
    }
}

internal inline bool32 IsCCW(Vec4 triangle[3])
{
    Vec2 e01 = {
        triangle[1].x - triangle[0].x,
        triangle[1].y - triangle[0].y
    };
    Vec2 e02 = {
        triangle[2].x - triangle[0].x,
        triangle[2].y - triangle[0].y
    };
    e02 = { -e02.y, e02.x };
    return Dot(e01, e02) <= 0.0f;
}

void RenderMeshPhongOpt(const Mesh& mesh,
    Mat4 model, Mat4 view, Mat4 proj,
    bool32 backfaceCulling,
    Vec3 cameraPos, Vec3 lightPos, Material material,
    Bitmap* diffuseMap, Bitmap* specularMap, Bitmap* normalMap,
    GameBackbuffer* backbuffer, MeshScratch* scratch)
{
    DEBUG_ASSERT(mesh.triangles.size <= MAX_TRIANGLES);
    Mat4 mvp = proj * view * model;
    int tCount = 0;
    scratch->numTriangles = 0;
    Vec4 transformed[3];
    for (int t = 0; t < (int)mesh.triangles.size; t++) {
        bool shouldRender = false;
        for (int i = 0; i < 3; i++) {
            transformed[i] = ToVec4(mesh.triangles[t].v[i], 1.0f);
            transformed[i] = mvp * transformed[i];
            transformed[i] /= transformed[i].w;
            if (transformed[i].z < -1.0f || transformed[i].z > 1.0f) {
                shouldRender = false;
                break;
            }
            if (-1.0f <= transformed[i].x && transformed[i].x <= 1.0f
            && -1.0f <= transformed[i].y && transformed[i].y <= 1.0f) {
                shouldRender = true;
            }
        }

        if (!shouldRender || (backfaceCulling && !IsCCW(transformed))) {
            continue;
        }

        for (int i = 0; i < 3; i++) {
            scratch->triangles[tCount].screenPos[i] = {
                (int)((transformed[i].x + 1.0f) / 2.0f * backbuffer->width),
                (int)((transformed[i].y + 1.0f) / 2.0f * backbuffer->height)
            };
            scratch->triangles[tCount].depth[i] =
                (uint32)((-transformed[i].z + 1.0f) / 2.0f * UINT32_MAX);

            scratch->triangles[tCount].pos[i] = ToVec3(model
                * ToVec4(mesh.triangles[t].v[i], 1.0f));
            scratch->triangles[tCount].uv[i] = mesh.triangles[t].uv[i];
            scratch->triangles[tCount].normal[i] = ToVec3(model
                * ToVec4(mesh.triangles[t].n[i], 0.0f));
        }
        tCount++;
    }
    scratch->numTriangles = tCount;

    RenderTrianglesPhong(backbuffer, scratch,
        diffuseMap, specularMap, normalMap,
        cameraPos, lightPos, material);
}

void FreeMesh(Mesh* mesh)
{
    mesh->triangles.Free();
}
