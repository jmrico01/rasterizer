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

#define DEFAULT_CAM_Z 2.0f
#define ZOOM_STEP 0.999f

#define UI_MARGIN 20
#define UI_ITEM_SPACING 6

void ChangeShadingMode(Button* button, void* data)
{
    GameState* gameState = (GameState*)data;

    if (strncmp(button->text, "Wire", INPUT_BUFFER_SIZE) == 0) {
        DEBUG_PRINT("Changed shading mode to Wire\n");
        gameState->shadeMode = SHADEMODE_WIRE;
    }
    else if (strncmp(button->text, "Flat", INPUT_BUFFER_SIZE) == 0) {
        DEBUG_PRINT("Changed shading mode to Flat\n");
        gameState->shadeMode = SHADEMODE_FLAT;
    }
    else if (strncmp(button->text, "Gouraud", INPUT_BUFFER_SIZE) == 0) {
        DEBUG_PRINT("Changed shading mode to Gouraud\n");
        gameState->shadeMode = SHADEMODE_GOURAUD;
    }
    else if (strncmp(button->text, "Phong", INPUT_BUFFER_SIZE) == 0) {
        DEBUG_PRINT("Changed shading mode to Phong\n");
        gameState->shadeMode = SHADEMODE_PHONG;
    }
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
        
        FT_Error error = FT_Init_FreeType(&gameState->library);
        if (error) {
            DEBUG_PRINT("FreeType init error: %d\n", error);
        }
        LoadFontFace(thread, gameState->library,
            "data/fonts/computer-modern/serif.ttf", 18,
            memory->DEBUGPlatformReadFile, memory->DEBUGPlatformFreeFileMemory,
            &gameState->fontFaceSmall);
        LoadFontFace(thread, gameState->library,
            "data/fonts/computer-modern/serif.ttf", 24,
            memory->DEBUGPlatformReadFile, memory->DEBUGPlatformFreeFileMemory,
            &gameState->fontFaceMedium);
        LoadFontFace(thread, gameState->library,
            "data/fonts/computer-modern/serif.ttf", 36,
            memory->DEBUGPlatformReadFile, memory->DEBUGPlatformFreeFileMemory,
            &gameState->fontFaceLarge);
        
        Vec2Int shadeModeButtonsSize = {
            200, (int)gameState->fontFaceSmall.height + UI_ITEM_SPACING
        };
        Vec2Int shadeModeButtonsOrigin = {
            UI_MARGIN, UI_MARGIN
        };
        Vec2Int shadeModeButtonsStride = {
            0, shadeModeButtonsSize.y + UI_ITEM_SPACING
        };
        const char* shadeModeButtonNames[4] = {
            "Phong", "Gouraud", "Flat", "Wire"
        };
        for (int i = 0; i < 4; i++) {
            Vec2Int pos = shadeModeButtonsOrigin + shadeModeButtonsStride * i;
            gameState->shadeModeButtons[i] = CreateButton(
                pos, shadeModeButtonsSize,
                shadeModeButtonNames[i],
                &ChangeShadingMode,
                Vec4 { 0.4f, 0.4f, 0.4f, 1.0f },
                Vec4 { 0.6f, 0.6f, 0.6f, 1.0f },
                Vec4 { 0.9f, 0.9f, 0.9f, 1.0f },
                Vec4 { 0.9f, 0.9f, 0.9f, 1.0f }
            );
        }

        gameState->cube = LoadMeshFromObj(thread, "data/models/teapot.obj",
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
        * powf(ZOOM_STEP, (float)input->mouseWheel);
    
    for (int i = 0; i < 4; i++) {
        gameState->shadeModeButtons[i].box.color =
            Vec4 { 0.2f, 0.2f, 0.2f, 1.0f };
        gameState->shadeModeButtons[i].box.hoverColor =
            Vec4 { 0.5f, 0.5f, 0.5f, 1.0f };
        gameState->shadeModeButtons[i].box.pressColor =
            Vec4 { 0.7f, 0.7f, 0.7f, 1.0f };
    }
    gameState->shadeModeButtons[gameState->shadeMode].box.color =
        Vec4 { 0.2f, 0.4f, 0.4f, 1.0f };
    gameState->shadeModeButtons[gameState->shadeMode].box.hoverColor =
        Vec4 { 0.4f, 0.6f, 0.6f, 1.0f };
    gameState->shadeModeButtons[gameState->shadeMode].box.pressColor =
        Vec4 { 0.6f, 0.8f, 0.8f, 1.0f };

    UpdateButtons(gameState->shadeModeButtons, 4, input, (void*)gameState);

    // Clear screen
    Vec4 clearColor = { 0.05f, 0.1f, 0.2f, 1.0f };
    ClearBackbuffer(backbuffer, clearColor);

    // -------------------- 3D rendering --------------------
    Mat4 proj = Projection(110.0f,
        (float32)backbuffer->width / (float32)backbuffer->height,
        0.1f, 10.0f);
    Mat4 view = Translate(-gameState->cameraPos)
        * UnitQuatToMat4(gameState->modelRot);
    
    Mat4 mvp = proj * view;

    switch (gameState->shadeMode) {
        case SHADEMODE_WIRE: {
            RenderMeshWire(gameState->cube, mvp, backbuffer);
        } break;
        case SHADEMODE_FLAT: {
        } break;
        case SHADEMODE_GOURAUD: {
        } break;
        case SHADEMODE_PHONG: {
        } break;

        default: {
        } break;
    }
    // ------------------------------------------------------

    RenderText(&gameState->fontFaceMedium, "Software Rasterizer",
        Vec2Int { UI_MARGIN, backbuffer->height - UI_MARGIN },
        Vec2 { 0.0f, 1.0f },
        Vec4 { 0.7f, 0.9f, 0.9f, 1.0f },
        backbuffer
    );
    RenderText(&gameState->fontFaceSmall, "Jose M Rico <jrico>",
        Vec2Int {
            UI_MARGIN,
            backbuffer->height - UI_MARGIN
                - (int)gameState->fontFaceMedium.height - UI_ITEM_SPACING
        },
        Vec2 { 0.0f, 1.0f },
        Vec4 { 0.7f, 0.9f, 0.9f, 1.0f },
        backbuffer
    );

    // Draw shading mode buttons
    Vec2Int topPos = gameState->shadeModeButtons[3].box.origin;
    topPos.x += gameState->shadeModeButtons[3].box.size.x / 2;
    topPos.y += gameState->shadeModeButtons[3].box.size.y;
    topPos.y += UI_ITEM_SPACING;
    RenderText(&gameState->fontFaceMedium, "Shading Mode",
        topPos, Vec2 { 0.5f, 0.0f },
        Vec4 { 0.9f, 0.9f, 0.9f, 1.0f },
        backbuffer);
    DrawButtons(gameState->shadeModeButtons, 4,
        backbuffer, &gameState->fontFaceSmall);

    // Screen resolution
    char resolutionString[512];
    sprintf(resolutionString, "Resolution: %d x %d",
        backbuffer->width, backbuffer->height);
    RenderText(&gameState->fontFaceSmall, resolutionString,
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
    RenderText(&gameState->fontFaceSmall, fpsString,
        Vec2Int {
            backbuffer->width - UI_MARGIN,
            backbuffer->height - UI_MARGIN
                - (int)gameState->fontFaceSmall.height - UI_ITEM_SPACING
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