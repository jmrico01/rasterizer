#include "main.h"

#include <stdio.h> // TODO: breaks convention, remove this

#include "main_platform.h"
#include "km_debug.h"
#include "km_defines.h"
#include "km_input.h"
#include "km_math.h"
#include "render.h"
#include "gui.h"
#include "text.h"
#include "mesh.h"

#define DEFAULT_CAM_Z 3.0f
#define CAM_ZOOM_STEP 0.999f
#define CAM_MOVE_STEP 0.25f

#define UI_MARGIN 20
#define UI_ITEM_SPACING 6

const Vec4 defaultIdleColor = { 0.2f, 0.2f, 0.2f, 1.0f };
const Vec4 defaultHoverColor = { 0.55f, 0.55f, 0.45f, 1.0f };
const Vec4 defaultPressColor = { 0.8f, 0.8f, 0.65f, 1.0f };
const Vec4 defaultTextColor = { 0.9f, 0.9f, 0.9f, 1.0f };

const Vec4 interestIdleColor = { 0.2f, 0.4f, 0.4f, 1.0f };
const Vec4 interestHoverColor = { 0.4f, 0.6f, 0.6f, 1.0f };
const Vec4 interestPressColor = { 0.6f, 0.8f, 0.8f, 1.0f };
const Vec4 interestTextColor = { 0.7f, 0.9f, 0.9f, 1.0f };

struct ChangeMeshFieldData
{
    ThreadContext* thread;
    GameState* gameState;
    GameBackbuffer* backbuffer;
    DEBUGPlatformReadFileFunc* DEBUGPlatformReadFile;
    DEBUGPlatformFreeFileMemoryFunc* DEBUGPlatformFreeFileMemory;
};

internal int StringLength(const char* string)
{
	int length = 0;
	while (*string++) {
		length++;
    }

	return length;
}
internal void CatStrings(
	size_t sourceACount, const char* sourceA,
	size_t sourceBCount, const char* sourceB,
	size_t destCount, char* dest)
{
	for (size_t i = 0; i < sourceACount; i++) {
		*dest++ = *sourceA++;
    }

	for (size_t i = 0; i < sourceBCount; i++) {
		*dest++ = *sourceB++;
    }

	*dest++ = '\0';
}

internal void ChangeLightPosCallback(InputField* inputField, void* data)
{
    GameState* gameState = (GameState*)data;
    float32 floatVal = (float32)strtod(inputField->text, nullptr);

    for (int i = 0; i < 3; i++) {
        if (inputField == &gameState->lightPosFields[i]) {
            gameState->lightPos.e[i] = floatVal;
            return;
        }
    }
}

internal void ToggleBackfaceCullingCallback(Button* button, void* data)
{
    GameState* gameState = (GameState*)data;
    gameState->backfaceCulling = !gameState->backfaceCulling;
}

internal void ChangeShadingModeCallback(Button* button, void* data)
{
    GameState* gameState = (GameState*)data;

    for (int i = 0; i < SHADEMODE_LAST; i++) {
        if (button == &gameState->shadeModeButtons[i]) {
            gameState->shadeMode = (ShadeMode)i;
        }
    }
}

internal void ChangeMaterialCallback(InputField* inputField, void* data)
{
    GameState* gameState = (GameState*)data;

    float32 floatVal = (float32)strtod(inputField->text, nullptr);
    int intVal = (int)strtol(inputField->text, nullptr, 10);
    if (intVal > 1) {
        floatVal /= 255.0f;
    }

    for (int i = 0; i < 3; i++) {
        if (inputField == &gameState->ambientFields[i]) {
            gameState->globalMaterial.ambient.e[i] = floatVal;
            return;
        }
    }
    for (int i = 0; i < 3; i++) {
        if (inputField == &gameState->diffuseFields[i]) {
            gameState->globalMaterial.diffuse.e[i] = floatVal;
            return;
        }
    }
    for (int i = 0; i < 3; i++) {
        if (inputField == &gameState->specularFields[i]) {
            gameState->globalMaterial.specular.e[i] = floatVal;
            return;
        }
    }
    if (inputField == &gameState->shininessField) {
        gameState->globalMaterial.shininess = intVal;
        return;
    }
}

internal void ChangeMeshFieldCallback(InputField* inputField, void* data)
{
    ChangeMeshFieldData* cmfData = (ChangeMeshFieldData*)data;
    GameState* gameState = cmfData->gameState;

    for (int i = 0; i < (int)gameState->meshFields.size; i++) {
        if (&gameState->meshFields[i].inputField == inputField) {
            char modelPath[256];
            sprintf(modelPath, "data/models/%s", inputField->text);
            FreeMesh(&gameState->meshFields[i].mesh);
            gameState->meshFields[i].mesh = LoadMeshFromObj(cmfData->thread,
                modelPath,
                cmfData->DEBUGPlatformReadFile,
                cmfData->DEBUGPlatformFreeFileMemory
            );
        }
    }
}

internal void UpdateMeshFieldLayout(GameState* gameState,
    GameBackbuffer* backbuffer)
{
    Vec2Int size = {
        200,
        (int)gameState->fontFaceMedium.height + UI_ITEM_SPACING
    };
    Vec2Int origin = {
        backbuffer->width - UI_MARGIN - 200,
        UI_MARGIN
    };
    Vec2Int stride = {
        0, size.y + UI_ITEM_SPACING
    };
    Vec2Int closeButtonOffset = {
        -(size.y + UI_ITEM_SPACING), 0
    };
    Vec2Int closeButtonSize = {
        size.y, size.y
    };

    for (int i = 0; i < (int)gameState->meshFields.size; i++) {
        Vec2Int pos = origin + stride * i;
        gameState->meshFields[i].inputField.box.origin = pos;
        gameState->meshFields[i].inputField.box.size = size;

        gameState->meshFields[i].closeButton.box.origin =
            pos + closeButtonOffset;
        gameState->meshFields[i].closeButton.box.size = closeButtonSize;
    }
    
    MeshField topMeshField = gameState->meshFields[
        gameState->meshFields.size - 1];
    Vec2Int meshFieldsTopPos = topMeshField.inputField.box.origin;
    meshFieldsTopPos += stride;
    gameState->addMeshFieldButton.box.origin = meshFieldsTopPos;
    gameState->addMeshFieldButton.box.size = size;
}

internal void RemoveMeshFieldCallback(Button* button, void* data)
{
    ChangeMeshFieldData* cmfData = (ChangeMeshFieldData*)data;
    GameState* gameState = cmfData->gameState;

    if (gameState->meshFields.size == 1) {
        return;
    }

    for (int i = 0; i < (int)gameState->meshFields.size; i++) {
        if (&gameState->meshFields[i].closeButton == button) {
            FreeMesh(&gameState->meshFields[i].mesh);
            gameState->meshFields.Remove(i);
        }
    }
    
    UpdateMeshFieldLayout(gameState, cmfData->backbuffer);
}

internal void AddMeshField(ThreadContext* thread,
    GameState* gameState,
    const char* modelName, GameBackbuffer* backbuffer,
    DEBUGPlatformReadFileFunc* DEBUGPlatformReadFile,
    DEBUGPlatformFreeFileMemoryFunc* DEBUGPlatformFreeFileMemory)
{
    char modelPath[256];
    sprintf(modelPath, "data/models/%s", modelName);

    MeshField meshField;
    meshField.mesh = LoadMeshFromObj(thread, modelPath,
        DEBUGPlatformReadFile,
        DEBUGPlatformFreeFileMemory
    );
    meshField.inputField = CreateInputField(
        Vec2Int::zero, Vec2Int::zero,
        modelName, &ChangeMeshFieldCallback,
        defaultIdleColor, defaultHoverColor, defaultPressColor,
        defaultTextColor
    );
    meshField.closeButton = CreateButton(
        Vec2Int::zero, Vec2Int::zero,
        "X", &RemoveMeshFieldCallback,
        defaultIdleColor, defaultHoverColor, defaultPressColor,
        defaultTextColor
    );

    gameState->meshFields.Append(meshField);
}

internal void AddMeshFieldButtonCallback(Button* button, void* data)
{
    ChangeMeshFieldData* cmfData = (ChangeMeshFieldData*)data;
    GameState* gameState = cmfData->gameState;

    AddMeshField(cmfData->thread, gameState, "cube.obj", cmfData->backbuffer,
        cmfData->DEBUGPlatformReadFile,
        cmfData->DEBUGPlatformFreeFileMemory);
    UpdateMeshFieldLayout(gameState, cmfData->backbuffer);
}

extern "C" GAME_UPDATE_AND_RENDER_FUNC(GameUpdateAndRender)
{
	DEBUG_ASSERT(sizeof(GameState) <= memory->permanentStorageSize);
	GameState* gameState = (GameState*)memory->permanentStorage;

    if (memory->DEBUGShouldInitGlobals) {
	    // Initialize global function names
#if GAME_SLOW
        debugPrint_ = memory->DEBUGPlatformPrint;
#endif

        memory->DEBUGShouldInitGlobals = false;
    }
	if (!memory->isInitialized) {
        gameState->cameraPos = { 0.0f, 0.0f, DEFAULT_CAM_Z };
        gameState->modelRot = QuatFromAngleUnitAxis(PI_F / 6.0f, Vec3::unitX)
            * QuatFromAngleUnitAxis(-PI_F / 4.0f, Vec3::unitY);
        
        gameState->shadeMode = SHADEMODE_WIRE;
        // initialized later
        //gameState->lightPos = Vec3 { 2.0f, 2.0f, 10.0f };
        gameState->globalMaterial.ambient = Vec3 { 0.0f, 0.0f, 0.0f };
        gameState->globalMaterial.diffuse = Vec3 { 1.0f, 1.0f, 1.0f };
        gameState->globalMaterial.specular = Vec3 { 1.0f, 1.0f, 1.0f };
        gameState->globalMaterial.shininess = 5;
        gameState->backfaceCulling = false;
        
        FT_Error error = FT_Init_FreeType(&gameState->library);
        if (error) {
            DEBUG_PRINT("FreeType init error: %d\n", error);
        }
        LoadFontFace(thread, gameState->library,
            "data/fonts/computer-modern/serif.ttf", 14,
            memory->DEBUGPlatformReadFile, memory->DEBUGPlatformFreeFileMemory,
            &gameState->fontFaceSmall);
        LoadFontFace(thread, gameState->library,
            "data/fonts/computer-modern/serif.ttf", 18,
            memory->DEBUGPlatformReadFile, memory->DEBUGPlatformFreeFileMemory,
            &gameState->fontFaceMedium);
        LoadFontFace(thread, gameState->library,
            "data/fonts/computer-modern/serif.ttf", 24,
            memory->DEBUGPlatformReadFile, memory->DEBUGPlatformFreeFileMemory,
            &gameState->fontFaceLarge);
        
        Vec2Int shadeModeButtonsSize = {
            170, (int)gameState->fontFaceMedium.height + UI_ITEM_SPACING
        };
        Vec2Int shadeModeButtonsOrigin = {
            UI_MARGIN, UI_MARGIN
        };
        Vec2Int shadeModeButtonsStride = {
            0, shadeModeButtonsSize.y + UI_ITEM_SPACING
        };
        const char* shadeModeButtonNames[SHADEMODE_LAST] = {
            "Phong (Textured)", "Phong", "Gouraud", "Flat", "Wire"
        };
        for (int i = 0; i < SHADEMODE_LAST; i++) {
            Vec2Int pos = shadeModeButtonsOrigin + shadeModeButtonsStride * i;
            gameState->shadeModeButtons[i] = CreateButton(
                pos, shadeModeButtonsSize,
                shadeModeButtonNames[i],
                &ChangeShadingModeCallback,
                defaultIdleColor, defaultHoverColor, defaultPressColor,
                defaultTextColor
            );
        }

        Vec2Int colorFieldsSize = {
            50, (int)gameState->fontFaceSmall.height + UI_ITEM_SPACING
        };
        Vec2Int colorFieldsOrigin = {
            shadeModeButtonsOrigin.x + shadeModeButtonsSize.x
                + UI_ITEM_SPACING * 3,
            UI_MARGIN
        };
        Vec2Int colorFieldsStride = {
            colorFieldsSize.x + UI_ITEM_SPACING,
            colorFieldsSize.y + (int)gameState->fontFaceSmall.height
                + UI_ITEM_SPACING * 2
        };
        for (int i = 0; i < 3; i++) {
            Vec2Int pos = colorFieldsOrigin;
            pos.x += colorFieldsStride.x * i;
            gameState->ambientFields[i] = CreateInputField(
                pos, colorFieldsSize,
                "0", &ChangeMaterialCallback,
                defaultIdleColor, defaultHoverColor, defaultPressColor,
                defaultTextColor
            );
        }
        for (int i = 0; i < 3; i++) {
            Vec2Int pos = colorFieldsOrigin;
            pos.y += colorFieldsStride.y;
            pos.x += colorFieldsStride.x * i;
            gameState->diffuseFields[i] = CreateInputField(
                pos, colorFieldsSize,
                "1", &ChangeMaterialCallback,
                defaultIdleColor, defaultHoverColor, defaultPressColor,
                defaultTextColor
            );
        }
        for (int i = 0; i < 3; i++) {
            Vec2Int pos = colorFieldsOrigin;
            pos.y += colorFieldsStride.y * 2;
            pos.x += colorFieldsStride.x * i;
            gameState->specularFields[i] = CreateInputField(
                pos, colorFieldsSize,
                "1", &ChangeMaterialCallback,
                defaultIdleColor, defaultHoverColor, defaultPressColor,
                defaultTextColor
            );
        }
        Vec2Int shinyPos = colorFieldsOrigin;
        shinyPos.y += colorFieldsStride.y * 3;
        gameState->shininessField = CreateInputField(
            shinyPos, colorFieldsSize,
            "5", &ChangeMaterialCallback,
            defaultIdleColor, defaultHoverColor, defaultPressColor,
            defaultTextColor
        );

        Vec2Int backfaceCullingButtonSize = {
            180, (int)gameState->fontFaceMedium.height + UI_ITEM_SPACING
        };
        Vec2Int backfaceCullingButtonOrigin = {
            colorFieldsOrigin.x + colorFieldsStride.x * 3
                + UI_ITEM_SPACING * 2,
            UI_MARGIN
        };
        gameState->backfaceCullingButton = CreateButton(
            backfaceCullingButtonOrigin, backfaceCullingButtonSize,
            "Back-Face Culling", &ToggleBackfaceCullingCallback,
            defaultIdleColor, defaultHoverColor, defaultPressColor,
            defaultTextColor
        );

        Vec2Int lightPosFieldsSize = {
            50, (int)gameState->fontFaceSmall.height + UI_ITEM_SPACING
        };
        Vec2Int lightPosFieldsOrigin = {
            backfaceCullingButtonOrigin.x,
            backfaceCullingButtonOrigin.y + backfaceCullingButtonSize.y
                + UI_ITEM_SPACING * 3
        };
        Vec2Int lightPosFieldsStride = {
            colorFieldsSize.x + UI_ITEM_SPACING, 0
        };
        const char* lightPosFieldsInit[3] = {
            "10", "4", "-0.5"
        };
        for (int i = 0; i < 3; i++) {
            Vec2Int pos = lightPosFieldsOrigin + lightPosFieldsStride * i;
            gameState->lightPosFields[i] = CreateInputField(
                pos, lightPosFieldsSize,
                lightPosFieldsInit[i], &ChangeLightPosCallback,
                defaultIdleColor, defaultHoverColor, defaultPressColor,
                defaultTextColor
            );
            ChangeLightPosCallback(&gameState->lightPosFields[i],
                (void*)gameState);
        }

        gameState->meshFields.Init();
        AddMeshField(thread, gameState, "afrhead.obj", backbuffer,
            memory->DEBUGPlatformReadFile,
            memory->DEBUGPlatformFreeFileMemory);
        AddMeshField(thread, gameState, "afreye.obj", backbuffer,
            memory->DEBUGPlatformReadFile,
            memory->DEBUGPlatformFreeFileMemory);
        gameState->addMeshFieldButton = CreateButton(
            Vec2Int::zero, Vec2Int::zero,
            "Add Mesh",
            &AddMeshFieldButtonCallback,
            defaultIdleColor, defaultHoverColor, defaultPressColor,
            defaultTextColor
        );
        UpdateMeshFieldLayout(gameState, backbuffer);

        LoadBMP(thread,
            "data/textures/african_head/african_head_diffuse.bmp",
            &gameState->ahDiffuseMap,
            memory->DEBUGPlatformReadFile,
            memory->DEBUGPlatformFreeFileMemory);
        LoadBMP(thread,
            "data/textures/african_head/african_head_spec_24bit.bmp",
            &gameState->ahSpecularMap,
            memory->DEBUGPlatformReadFile,
            memory->DEBUGPlatformFreeFileMemory);
        LoadBMP(thread,
            "data/textures/african_head/african_head_nm.bmp",
            &gameState->ahNormalMap,
            memory->DEBUGPlatformReadFile,
            memory->DEBUGPlatformFreeFileMemory);
        
        LoadBMP(thread,
            "data/textures/african_head/african_head_eye_inner_diffuse.bmp",
            &gameState->aeDiffuseMap,
            memory->DEBUGPlatformReadFile,
            memory->DEBUGPlatformFreeFileMemory);
        LoadBMP(thread,
            "data/textures/african_head/african_head_eye_inner_spec.bmp",
            &gameState->aeSpecularMap,
            memory->DEBUGPlatformReadFile,
            memory->DEBUGPlatformFreeFileMemory);
        LoadBMP(thread,
            "data/textures/african_head/african_head_eye_inner_nm.bmp",
            &gameState->aeNormalMap,
            memory->DEBUGPlatformReadFile,
            memory->DEBUGPlatformFreeFileMemory);
        
        LoadBMP(thread,
            "data/textures/boggie/body_diffuse.bmp",
            &gameState->boggieBodyDiffuseMap,
            memory->DEBUGPlatformReadFile,
            memory->DEBUGPlatformFreeFileMemory);
        LoadBMP(thread,
            "data/textures/boggie/body_spec.bmp",
            &gameState->boggieBodySpecularMap,
            memory->DEBUGPlatformReadFile,
            memory->DEBUGPlatformFreeFileMemory);
        
        LoadBMP(thread,
            "data/textures/boggie/eyes_diffuse.bmp",
            &gameState->boggieEyesDiffuseMap,
            memory->DEBUGPlatformReadFile,
            memory->DEBUGPlatformFreeFileMemory);
        LoadBMP(thread,
            "data/textures/boggie/eyes_spec.bmp",
            &gameState->boggieEyesSpecularMap,
            memory->DEBUGPlatformReadFile,
            memory->DEBUGPlatformFreeFileMemory);
        
        LoadBMP(thread,
            "data/textures/boggie/head_diffuse.bmp",
            &gameState->boggieHeadDiffuseMap,
            memory->DEBUGPlatformReadFile,
            memory->DEBUGPlatformFreeFileMemory);
        LoadBMP(thread,
            "data/textures/boggie/head_spec.bmp",
            &gameState->boggieHeadSpecularMap,
            memory->DEBUGPlatformReadFile,
            memory->DEBUGPlatformFreeFileMemory);
        
        LoadBMP(thread,
            "data/textures/diablo/diablo3_pose_diffuse.bmp",
            &gameState->diabloDiffuseMap,
            memory->DEBUGPlatformReadFile,
            memory->DEBUGPlatformFreeFileMemory);
        LoadBMP(thread,
            "data/textures/diablo/diablo3_pose_spec.bmp",
            &gameState->diabloSpecularMap,
            memory->DEBUGPlatformReadFile,
            memory->DEBUGPlatformFreeFileMemory);
        LoadBMP(thread,
            "data/textures/diablo/diablo3_pose_nm.bmp",
            &gameState->diabloNormalMap,
            memory->DEBUGPlatformReadFile,
            memory->DEBUGPlatformFreeFileMemory);

		memory->isInitialized = true;
	}

    // Camera control
    if (input->mouseButtons[0].isDown) {
        float speed = 0.01f;
        gameState->modelRot =
            QuatFromAngleUnitAxis(input->mouseDelta.x * speed, Vec3::unitY)
            * QuatFromAngleUnitAxis(-input->mouseDelta.y * speed, Vec3::unitX)
            * gameState->modelRot;
        gameState->modelRot = Normalize(gameState->modelRot);
    }
    gameState->cameraPos.z = DEFAULT_CAM_Z
        * powf(CAM_ZOOM_STEP, (float)input->mouseWheel);
    if (WasKeyPressed(input, KM_KEY_ARROW_UP)) {
        gameState->cameraPos.y += CAM_MOVE_STEP;
    }
    if (WasKeyPressed(input, KM_KEY_ARROW_DOWN)) {
        gameState->cameraPos.y -= CAM_MOVE_STEP;
    }
    if (WasKeyPressed(input, KM_KEY_ARROW_LEFT)) {
        gameState->cameraPos.x -= CAM_MOVE_STEP;
    }
    if (WasKeyPressed(input, KM_KEY_ARROW_RIGHT)) {
        gameState->cameraPos.x += CAM_MOVE_STEP;
    }
    
    // Update shade mode buttons
    for (int i = 0; i < SHADEMODE_LAST; i++) {
        gameState->shadeModeButtons[i].box.color = defaultIdleColor;
        gameState->shadeModeButtons[i].box.hoverColor = defaultHoverColor;
        gameState->shadeModeButtons[i].box.pressColor = defaultPressColor;
    }
    gameState->shadeModeButtons[gameState->shadeMode].box.color =
        interestIdleColor;
    gameState->shadeModeButtons[gameState->shadeMode].box.hoverColor =
        interestHoverColor;
    gameState->shadeModeButtons[gameState->shadeMode].box.pressColor =
        interestPressColor;
    UpdateButtons(gameState->shadeModeButtons, SHADEMODE_LAST,
        input, (void*)gameState);

    // Update material settings fields
    UpdateInputFields(gameState->ambientFields, 3, input, (void*)gameState);
    UpdateInputFields(gameState->diffuseFields, 3, input, (void*)gameState);
    UpdateInputFields(gameState->specularFields, 3, input, (void*)gameState);
    UpdateInputFields(&gameState->shininessField, 1, input, (void*)gameState);

    // Update light position fields
    UpdateInputFields(gameState->lightPosFields, 3, input, (void*)gameState);
    
    // Update backface culling button
    if (gameState->backfaceCulling) {
        gameState->backfaceCullingButton.box.color = interestIdleColor;
        gameState->backfaceCullingButton.box.hoverColor = interestHoverColor;
        gameState->backfaceCullingButton.box.pressColor = interestPressColor;
    }
    else {
        gameState->backfaceCullingButton.box.color = defaultIdleColor;
        gameState->backfaceCullingButton.box.hoverColor = defaultHoverColor;
        gameState->backfaceCullingButton.box.pressColor = defaultPressColor;
    }
    UpdateButtons(&gameState->backfaceCullingButton, 1,
        input, (void*)gameState);

    // Update model fields
    UpdateMeshFieldLayout(gameState, backbuffer);
    
    ChangeMeshFieldData cmfData;
    cmfData.thread = thread;
    cmfData.gameState = gameState;
    cmfData.backbuffer = backbuffer;
    cmfData.DEBUGPlatformReadFile = memory->DEBUGPlatformReadFile;
    cmfData.DEBUGPlatformFreeFileMemory = memory->DEBUGPlatformFreeFileMemory;
    UpdateButtons(&gameState->addMeshFieldButton, 1,
        input, (void*)&cmfData);
    for (int i = 0; i < (int)gameState->meshFields.size; i++) {
        UpdateInputFields(&gameState->meshFields[i].inputField, 1,
            input, (void*)&cmfData);
        UpdateButtons(&gameState->meshFields[i].closeButton, 1,
            input, (void*)&cmfData);
    }

    // Clear screen
    Vec3 clearColor = { 0.0f, 0.0f, 0.05f };
    uint32 clearDepth = 0;
    ClearBackbuffer(backbuffer, clearColor, clearDepth);

    // -------------------- 3D rendering --------------------
    Mat4 proj = Projection(110.0f,
        (float32)backbuffer->width / (float32)backbuffer->height,
        0.1f, 10.0f);
    Mat4 view = Translate(-gameState->cameraPos);
    Mat4 model = UnitQuatToMat4(gameState->modelRot);

    for (int i = 0; i < (int)gameState->meshFields.size; i++) {
        DEBUG_ASSERT(sizeof(MeshScratch) <= memory->transientStorageSize);
        MeshScratch* meshScratch = (MeshScratch*)memory->transientStorage;

        switch (gameState->shadeMode) {
            case SHADEMODE_WIRE: {
                RenderMeshWire(gameState->meshFields[i].mesh,
                    model, view, proj, Vec3 { 1.0f, 0.0f, 0.0f },
                    backbuffer, meshScratch);
            } break;
            case SHADEMODE_FLAT: {
                RenderMeshFlat(gameState->meshFields[i].mesh,
                    model, view, proj,
                    gameState->backfaceCulling,
                    gameState->cameraPos, gameState->lightPos,
                    gameState->globalMaterial,
                    backbuffer, meshScratch);
            } break;
            case SHADEMODE_GOURAUD: {
                RenderMeshGouraud(gameState->meshFields[i].mesh,
                    model, view, proj,
                    gameState->backfaceCulling,
                    gameState->cameraPos, gameState->lightPos,
                    gameState->globalMaterial,
                    backbuffer, meshScratch);
            } break;
            case SHADEMODE_PHONG: {
                RenderMeshPhong(gameState->meshFields[i].mesh,
                    model, view, proj,
                    gameState->backfaceCulling,
                    gameState->cameraPos, gameState->lightPos,
                    gameState->globalMaterial,
                    backbuffer, meshScratch);
            } break;
            case SHADEMODE_PHONG_MAT: {
                Bitmap* diffuseMap = nullptr;
                Bitmap* specularMap = nullptr;
                Bitmap* normalMap = nullptr;
                if (strncmp(gameState->meshFields[i].inputField.text,
                "afreye.obj", INPUT_BUFFER_SIZE) == 0) {
                    diffuseMap = &gameState->aeDiffuseMap;
                    specularMap = &gameState->aeSpecularMap;
                    normalMap = &gameState->aeNormalMap;
                }
                else if (strncmp(gameState->meshFields[i].inputField.text,
                "afrhead.obj", INPUT_BUFFER_SIZE) == 0) {
                    diffuseMap = &gameState->ahDiffuseMap;
                    specularMap = &gameState->ahSpecularMap;
                    normalMap = &gameState->ahNormalMap;
                }
                else if (strncmp(gameState->meshFields[i].inputField.text,
                "boggiebody.obj", INPUT_BUFFER_SIZE) == 0) {
                    diffuseMap = &gameState->boggieBodyDiffuseMap;
                    specularMap = &gameState->boggieBodySpecularMap;
                }
                else if (strncmp(gameState->meshFields[i].inputField.text,
                "boggieeyes.obj", INPUT_BUFFER_SIZE) == 0) {
                    diffuseMap = &gameState->boggieEyesDiffuseMap;
                    specularMap = &gameState->boggieEyesSpecularMap;
                }
                else if (strncmp(gameState->meshFields[i].inputField.text,
                "boggiehead.obj", INPUT_BUFFER_SIZE) == 0) {
                    diffuseMap = &gameState->boggieHeadDiffuseMap;
                    specularMap = &gameState->boggieHeadSpecularMap;
                }
                else if (strncmp(gameState->meshFields[i].inputField.text,
                "diablo.obj", INPUT_BUFFER_SIZE) == 0) {
                    diffuseMap = &gameState->diabloDiffuseMap;
                    specularMap = &gameState->diabloSpecularMap;
                    normalMap = &gameState->diabloNormalMap;
                }
                if (diffuseMap != nullptr && specularMap != nullptr) {
                    RenderMeshPhongTextured(gameState->meshFields[i].mesh,
                        model, view, proj,
                        gameState->backfaceCulling,
                        gameState->cameraPos, gameState->lightPos,
                        gameState->globalMaterial,
                        diffuseMap, specularMap, normalMap,
                        backbuffer, meshScratch);
                }
            } break;

            default: {
            } break;
        }
    }
    // ------------------------------------------------------

    RenderText(&gameState->fontFaceLarge, "Software Rasterizer",
        Vec2Int { UI_MARGIN, backbuffer->height - UI_MARGIN },
        Vec2 { 0.0f, 1.0f },
        Vec4 { 0.7f, 0.9f, 0.9f, 1.0f },
        backbuffer
    );
    RenderText(&gameState->fontFaceLarge, "Jose M Rico <jrico>",
        Vec2Int {
            UI_MARGIN,
            backbuffer->height - UI_MARGIN
                - (int)gameState->fontFaceLarge.height - UI_ITEM_SPACING
        },
        Vec2 { 0.0f, 1.0f },
        Vec4 { 0.7f, 0.9f, 0.9f, 1.0f },
        backbuffer
    );

    // Draw shading mode buttons
    Vec2Int shadeModeTopPos = gameState->shadeModeButtons[
        SHADEMODE_LAST - 1].box.origin;
    shadeModeTopPos.x += gameState->shadeModeButtons[
        SHADEMODE_LAST - 1].box.size.x / 2;
    shadeModeTopPos.y += gameState->shadeModeButtons[
        SHADEMODE_LAST - 1].box.size.y;
    shadeModeTopPos.y += UI_ITEM_SPACING;
    RenderText(&gameState->fontFaceLarge, "Shading Mode",
        shadeModeTopPos, Vec2 { 0.5f, 0.0f }, defaultTextColor, backbuffer);
    DrawButtons(gameState->shadeModeButtons, SHADEMODE_LAST,
        backbuffer, &gameState->fontFaceMedium);
    
    // Draw material settings fields
    Vec2Int colorFieldsOffset = {
        0, gameState->ambientFields[0].box.size.y + UI_ITEM_SPACING
    };
    Vec2Int topPos = gameState->ambientFields[0].box.origin + colorFieldsOffset;
    RenderText(&gameState->fontFaceMedium, "Ambient",
        topPos, defaultTextColor, backbuffer);
    DrawInputFields(gameState->ambientFields, 3,
        backbuffer, &gameState->fontFaceSmall);

    topPos = gameState->diffuseFields[0].box.origin + colorFieldsOffset;
    RenderText(&gameState->fontFaceMedium, "Diffuse",
        topPos, defaultTextColor, backbuffer);
    DrawInputFields(gameState->diffuseFields, 3,
        backbuffer, &gameState->fontFaceSmall);

    topPos = gameState->specularFields[0].box.origin + colorFieldsOffset;
    RenderText(&gameState->fontFaceMedium, "Specular",
        topPos, defaultTextColor, backbuffer);
    DrawInputFields(gameState->specularFields, 3,
        backbuffer, &gameState->fontFaceSmall);

    topPos = gameState->shininessField.box.origin + colorFieldsOffset;
    RenderText(&gameState->fontFaceMedium, "Shininess",
        topPos, defaultTextColor, backbuffer);
    DrawInputFields(&gameState->shininessField, 1,
        backbuffer, &gameState->fontFaceSmall);
    
    // Draw backface culling button
    DrawButtons(&gameState->backfaceCullingButton, 1,
        backbuffer, &gameState->fontFaceMedium);

    // Draw light pos fields
    topPos = gameState->lightPosFields[0].box.origin + colorFieldsOffset;
    RenderText(&gameState->fontFaceMedium, "Light Position",
        topPos, defaultTextColor, backbuffer);
    DrawInputFields(gameState->lightPosFields, 3,
        backbuffer, &gameState->fontFaceSmall);
    
    // Draw model fields
    DEBUG_ASSERT(gameState->meshFields.size > 0);
    Vec2Int meshFieldsTopPos = gameState->addMeshFieldButton.box.origin;
    meshFieldsTopPos.x += gameState->addMeshFieldButton.box.size.x / 2;
    meshFieldsTopPos.y += gameState->addMeshFieldButton.box.size.y;
    meshFieldsTopPos.y += UI_ITEM_SPACING;
    RenderText(&gameState->fontFaceLarge, "Loaded Meshes",
        meshFieldsTopPos, Vec2 { 0.5f, 0.0f }, defaultTextColor, backbuffer);
    DrawButtons(&gameState->addMeshFieldButton, 1,
        backbuffer, &gameState->fontFaceMedium);
    for (int i = 0; i < (int)gameState->meshFields.size; i++) {
        DrawInputFields(&gameState->meshFields[i].inputField, 1,
            backbuffer, &gameState->fontFaceMedium);
        DrawButtons(&gameState->meshFields[i].closeButton, 1,
            backbuffer, &gameState->fontFaceMedium);
    }

    // Screen resolution
    char resolutionString[512];
    sprintf(resolutionString, "Resolution: %d x %d",
        backbuffer->width, backbuffer->height);
    RenderText(&gameState->fontFaceMedium, resolutionString,
        Vec2Int {
            backbuffer->width - UI_MARGIN,
            backbuffer->height - UI_MARGIN
        },
        Vec2 { 1.0f, 1.0f },
        Vec4 { 0.7f, 0.9f, 0.9f, 1.0f },
        backbuffer
    );
    
    // FPS counter
    char fpsString[512];
    sprintf(fpsString, "FPS: %f", 1.0 / deltaTime);
    RenderText(&gameState->fontFaceMedium, fpsString,
        Vec2Int {
            backbuffer->width - UI_MARGIN,
            backbuffer->height - UI_MARGIN
                - (int)gameState->fontFaceMedium.height - UI_ITEM_SPACING
        },
        Vec2 { 1.0f, 1.0f },
        Vec4 { 0.7f, 0.9f, 0.9f, 1.0f },
        backbuffer
    );
}

#include "km_input.cpp"
#include "km_lib.cpp"
#include "render.cpp"
#include "gui.cpp"
#include "text.cpp"
#include "mesh.cpp"
#include "load_bmp.cpp"