#pragma once

#define PATH_MAX_LENGTH 512

#include "gui.h"
#include "text.h"
#include "mesh.h"
#include "load_bmp.h"

enum ShadeMode
{
    SHADEMODE_PHONG_MAT = 0,
    SHADEMODE_PHONG,
    SHADEMODE_GOURAUD,
    SHADEMODE_FLAT,
    SHADEMODE_WIRE,

    SHADEMODE_LAST // always keep at the end
};

enum TextureSampleMode
{
    SAMPLEMODE_POINT = 0,
    SAMPLEMODE_BILINEAR,

    SAMPLEMODE_LAST // always keep at the end
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
    bool32 backfaceCulling;

    FT_Library library;
    FontFace fontFaceSmall;
    FontFace fontFaceMedium;
    FontFace fontFaceLarge;

    // -------------- UI elements ---------------
    Button shadeModeButtons[SHADEMODE_LAST];

    InputField ambientFields[3];
    InputField diffuseFields[3];
    InputField specularFields[3];
    InputField shininessField;

    InputField lightPosFields[3];

    Button backfaceCullingButton;

    Button sampleModeButtons[SAMPLEMODE_LAST];

    Button addMeshFieldButton;
    DynamicArray<MeshField> meshFields;
    // ------------------------------------------

    Bitmap ahDiffuseMap;
    Bitmap ahSpecularMap;
    Bitmap ahNormalMap;

    Bitmap aeDiffuseMap;
    Bitmap aeSpecularMap;
    Bitmap aeNormalMap;

    Bitmap boggieBodyDiffuseMap;
    Bitmap boggieBodySpecularMap;

    Bitmap boggieEyesDiffuseMap;
    Bitmap boggieEyesSpecularMap;

    Bitmap boggieHeadDiffuseMap;
    Bitmap boggieHeadSpecularMap;

    Bitmap diabloDiffuseMap;
    Bitmap diabloSpecularMap;
    Bitmap diabloNormalMap;
};