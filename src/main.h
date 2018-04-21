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
    InputField input;
};

struct GameState
{
    Vec3 cameraPos;
    Quat modelRot;

    ShadeMode shadeMode;

    FT_Library library;
    FontFace fontFaceSmall;
    FontFace fontFaceMedium;
    FontFace fontFaceLarge;

    Button shadeModeButtons[4];

    DynamicArray<MeshField> meshFields;
    Mesh cube;
};