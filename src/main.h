#pragma once

#define PATH_MAX_LENGTH 512

#include "gui.h"
#include "text.h"
#include "mesh.h"

enum ShadeMode
{
    SHADEMODE_PHONG = 0,
    SHADEMODE_GOURAUD,
    SHADEMODE_FLAT,
    SHADEMODE_WIRE
};

struct MeshField
{
    Mesh mesh;
    InputField inputField;
    Button closeButton;
};

struct GameState
{
    Vec3 cameraPos;
    Quat modelRot;

    ShadeMode shadeMode;
    Vec3 lightPos;
    Material globalMaterial;

    FT_Library library;
    FontFace fontFaceSmall;
    FontFace fontFaceMedium;
    FontFace fontFaceLarge;

    Button shadeModeButtons[4];

    InputField ambientFields[3];
    InputField diffuseFields[3];
    InputField specularFields[3];
    InputField shininessField;

    Button addMeshFieldButton;
    DynamicArray<MeshField> meshFields;
};