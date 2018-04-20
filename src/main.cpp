#include "main.h"

#include <stdio.h> // TODO: breaks convention, remove this

#include "main_platform.h"
#include "km_debug.h"
#include "km_defines.h"
#include "km_input.h"
#include "km_math.h"
#include "render.h"
#include "text.h"

extern "C" GAME_UPDATE_AND_RENDER_FUNC(GameUpdateAndRender)
{
	DEBUG_ASSERT(sizeof(GameState) <= memory->permanentStorageSize);
	GameState* gameState = (GameState*)memory->permanentStorage;

    if (memory->DEBUGShouldInitGlobals) {
	    // Initialize global function names
        debugPrint_ = memory->DEBUGPlatformPrint;
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

		// TODO this may be more appropriate to do in the platform layer
		memory->isInitialized = true;
	}

    Vec4 clearColor = { 0.05f, 0.1f, 0.2f, 1.0f };
    ClearBackbuffer(backbuffer, clearColor);

    // FPS counter
    char fpsString[512];
    sprintf(fpsString, "FPS: %f", 1.0 / deltaTime);
    RenderText(&gameState->fontFace, fpsString,
        backbuffer->width - 10, backbuffer->height - 10,
        Vec2 {1.0f, 1.0f}, Vec4::one, backbuffer);

    RenderGrayscaleBitmap(backbuffer, input->mouseX, input->mouseY,
        gameState->fontFace.atlasData,
        gameState->fontFace.atlasWidth, gameState->fontFace.atlasHeight);
    
    RenderText(&gameState->fontFace, "Hello, world!",
        100, 100, Vec4::one, backbuffer);
}

#include "km_input.cpp"
#include "render.cpp"
#include "text.cpp"