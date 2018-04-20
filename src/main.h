#pragma once

#define PATH_MAX_LENGTH 512

#include "gui.h"
#include "text.h"

struct GameState
{
    FT_Library library;
    FontFace fontFace;

    ClickableBox box;
    InputField inputField;
    Button button;
};