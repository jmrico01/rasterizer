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

void TestCallback(Button* button, void* data)
{
    GameState* gameState = (GameState*)data;
    DEBUG_PRINT("testing a callback\n");
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
        FT_Error error = FT_Init_FreeType(&gameState->library);
        if (error) {
            DEBUG_PRINT("FreeType init error: %d\n", error);
        }
        LoadFontFace(thread, gameState->library,
            "data/fonts/computer-modern/serif.ttf", 24,
            memory->DEBUGPlatformReadFile, memory->DEBUGPlatformFreeFileMemory,
            &gameState->fontFace);
        
        gameState->box = CreateClickableBox(
            Vec2Int { 200, 200 }, Vec2Int { 200, 60 },
            Vec4 { 0.4f, 0.4f, 0.4f, 1.0f },
            Vec4 { 0.6f, 0.6f, 0.6f, 1.0f },
            Vec4 { 0.9f, 0.9f, 0.9f, 1.0f }
        );
        gameState->inputField = CreateInputField(
            Vec2Int { 20, 200 }, Vec2Int { 150, 60 },
            "Text",
            Vec4 { 0.4f, 0.4f, 0.4f, 1.0f },
            Vec4 { 0.6f, 0.6f, 0.6f, 1.0f },
            Vec4 { 0.9f, 0.9f, 0.9f, 1.0f },
            Vec4 { 0.9f, 0.9f, 0.9f, 1.0f }
        );
        gameState->button = CreateButton(
            Vec2Int { 20, 500 }, Vec2Int { 150, 150 },
            "Button Text",
            &TestCallback,
            Vec4 { 0.4f, 0.4f, 0.4f, 1.0f },
            Vec4 { 0.6f, 0.6f, 0.6f, 1.0f },
            Vec4 { 0.9f, 0.9f, 0.9f, 1.0f },
            Vec4 { 0.9f, 0.9f, 0.9f, 1.0f }
        );

		memory->isInitialized = true;
	}
    
    UpdateClickableBoxes(&gameState->box, 1, input);
    UpdateInputFields(&gameState->inputField, 1, input);
    UpdateButtons(&gameState->button, 1, input, (void*)gameState);

    Vec4 clearColor = { 0.05f, 0.1f, 0.2f, 1.0f };
    ClearBackbuffer(backbuffer, clearColor);

    /*RenderOverwriteGrayscaleBitmap(backbuffer, input->mousePos,
        gameState->fontFace.atlasData,
        gameState->fontFace.atlasWidth, gameState->fontFace.atlasHeight);*/
    
    RenderText(&gameState->fontFace, "Hello, world!",
        Vec2Int { 100, 100 }, Vec4::one, backbuffer);
    
    DrawClickableBoxes(&gameState->box, 1, backbuffer);
    DrawInputFields(&gameState->inputField, 1,
        backbuffer, &gameState->fontFace);
    DrawButtons(&gameState->button, 1, backbuffer, &gameState->fontFace);

    // FPS counter
    char fpsString[512];
    sprintf(fpsString, "FPS: %f", 1.0 / deltaTime);
    RenderText(&gameState->fontFace, fpsString,
        Vec2Int { backbuffer->width - 10, backbuffer->height - 10 },
        Vec2 {1.0f, 1.0f}, Vec4::one, backbuffer);
}

#include "km_input.cpp"
#include "render.cpp"
#include "gui.cpp"
#include "text.cpp"