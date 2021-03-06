#pragma once

#include <sys/types.h>

#include <alsa/asoundlib.h>

#include "km_defines.h"
#include "main_platform.h"

#define LINUX_STATE_FILE_NAME_COUNT  512
#define BYTES_PER_PIXEL 4

// Looked up with xev command
#define KEYCODE_W           25
#define KEYCODE_A           38
#define KEYCODE_S           39
#define KEYCODE_D           40
#define KEYCODE_Q           24
#define KEYCODE_E           26
#define KEYCODE_UP          111
#define KEYCODE_DOWN        116
#define KEYCODE_LEFT        113
#define KEYCODE_RIGHT       114
#define KEYCODE_ESCAPE      9
#define KEYCODE_ENTER       36
#define KEYCODE_SPACE       65
#define KEYCODE_P           33
#define KEYCODE_L           46

#define KEYCODE_SHIFT_L     50
#define KEYCODE_SHIFT_R     62
#define KEYCODE_CTRL_L      37
#define KEYCODE_CTRL_R      105
#define KEYCODE_ALT_L       64
#define KEYCODE_ALT_R       108
#define KEYCODE_SUPER       133

#define KEYCODE_PLUS        21
#define KEYCODE_MINUS       20

#define KEYCODE_F1          67
#define KEYCODE_F10         76
#define KEYCODE_F11         95
#define KEYCODE_F12         96

#define KEYCODE_SHIFT_MASK  0x01
#define KEYCODE_CTRL_MASK   0x04
#define KEYCODE_ALT_MASK    0x08

struct LinuxWindowDimension
{
    uint32 Width;
    uint32 Height;
};

struct LinuxAudio
{
    snd_pcm_t* pcmHandle;
    
};

struct LinuxDebugTimeMarker
{
    uint32 OutputPlayCursor;
    uint32 OutputWriteCursor;
    uint32 OutputLocation;
    uint32 OutputByteCount;
    uint32 ExpectedFlipPlayCursor;

    uint32 FlipPlayCursor;
    uint32 FlipWriteCursor;
};

struct LinuxGameCode
{
    void* gameLibHandle;
    ino_t gameLibId;

    // NOTE: Callbacks can be 0!  You must check before calling
    GameUpdateAndRenderFunc* gameUpdateAndRender;

    bool32 isValid;
};

struct LinuxState
{
	uint64 gameMemorySize;
	void* gameMemoryBlock;

    int32 recordingHandle;
    int32 inputRecordingIndex;

    int32 playbackHandle;
    int32 inputPlayingIndex;

    char exeFilePath[LINUX_STATE_FILE_NAME_COUNT];
    char* exeOnePastLastSlash;
};
