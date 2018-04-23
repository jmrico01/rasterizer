#include "win32_main.h"

#include <stdio.h>
#include <stdarg.h>
#include <intrin.h> // __rdtsc

#include "km_debug.h"

/*
	TODO

	- WINDOWS 7 VIRTUAL MACHINE

	- Saved game locations
	- Getting a handle to our own exe file
	- Asset loading path
	- Threading (launch a thread)
	- Sleep/timeBeginPeriod (don't melt the processor)
	- ClipCursor() (multimonitor support)
	- WM_SETCURSOR (control cursor visibility)
	- QueryCancelAutoplay
	- WM_ACTIVATEAPP (for when we are not the active app)
*/

#define START_WIDTH 1280
#define START_HEIGHT 800
#define SAMPLERATE 48000
#define AUDIO_BUFFER_SIZE_MILLISECONDS  2000

// TODO this is a global for now
global_var char pathToApp_[MAX_PATH];
global_var bool32 running_ = true;
global_var GameInput* input_ = nullptr;             // for WndProc WM_CHAR
global_var Win32Backbuffer backbuffer_;

global_var WINDOWPLACEMENT DEBUGwpPrev = { sizeof(DEBUGwpPrev) };

// WGL functions
typedef BOOL WINAPI wglSwapIntervalEXTFunc(int interval);
global_var wglSwapIntervalEXTFunc* wglSwapInterval_ = NULL;
#define wglSwapInterval wglSwapInterval_

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

internal Win32WindowDimension Win32GetWindowDimension(HWND hWnd)
{
    Win32WindowDimension result;
    
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    result.width = clientRect.right - clientRect.left;
    result.height = clientRect.bottom - clientRect.top;

    return result;
}

internal void Win32ResizeBackbuffer(Win32Backbuffer* buffer,
    uint32 width, uint32 height)
{
    if (buffer->data) {
        VirtualFree(buffer->data, 0, MEM_RELEASE);
        VirtualFree(buffer->depth, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;

    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = buffer->height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    int bytesPerPixel = 4;
    int bitmapMemorySize = (buffer->width * buffer->height) * bytesPerPixel;
    buffer->data = VirtualAlloc(0, bitmapMemorySize,
        MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    buffer->bytesPerPixel = bytesPerPixel;

    buffer->depth = (uint32*)VirtualAlloc(0, bitmapMemorySize,
        MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

internal void Win32DisplayBufferInWindow(Win32Backbuffer* buffer,
    HDC hDC, int windowWidth, int windowHeight)
{
    // TODO: Aspect ratio correction
    // TODO: Play with stretch modes
    StretchDIBits(hDC,
        0, 0, windowWidth, windowHeight,
        0, 0, buffer->width, buffer->height,
        buffer->data, &buffer->info,
        DIB_RGB_COLORS, SRCCOPY);
}


internal void Win32ToggleFullscreen(HWND hWnd)
{
	// This follows Raymond Chen's perscription for fullscreen toggling. See:
	// https://blogs.msdn.microsoft.com/oldnewthing/20100412-00/?p=14353

	DWORD dwStyle = GetWindowLong(hWnd, GWL_STYLE);
	if (dwStyle & WS_OVERLAPPEDWINDOW) {
		// Switch to fullscreen
		MONITORINFO monitorInfo = { sizeof(monitorInfo) };
		HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
		if (GetWindowPlacement(hWnd, &DEBUGwpPrev)
		&& GetMonitorInfo(hMonitor, &monitorInfo)) {
			SetWindowLong(hWnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
			SetWindowPos(hWnd, HWND_TOP,
				monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
				monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
				SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            Win32ResizeBackbuffer(&backbuffer_,
                monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top);
		}
	}
	else {
		// Switch to windowed
		SetWindowLong(hWnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(hWnd, &DEBUGwpPrev);
		SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER
            | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		RECT clientRect;
		GetClientRect(hWnd, &clientRect);
        Win32ResizeBackbuffer(&backbuffer_,
            clientRect.right - clientRect.left,
            clientRect.bottom - clientRect.top);
	}
}

internal void RemoveFileNameFromPath(
    const char* filePath, char* dest, uint64 destLength)
{
    unsigned int lastSlash = 0;
	// TODO confused... some cross-platform code inside a win32 file
	//	maybe I meant to pull this out sometime?
#ifdef _WIN32
    char pathSep = '\\';
#else
    char pathSep = '/';
#endif
    while (filePath[lastSlash] != '\0') {
        lastSlash++;
    }
    // TODO unsafe!
    while (filePath[lastSlash] != pathSep) {
        lastSlash--;
    }
    if (lastSlash + 2 > destLength) {
        return;
    }
    for (unsigned int i = 0; i < lastSlash + 1; i++) {
        dest[i] = filePath[i];
    }
    dest[lastSlash + 1] = '\0';
}

internal void Win32GetExeFilePath(Win32State* state)
{
	// NOTE
	// Never use MAX_PATH in code that is user-facing, because it is
	// dangerous and can lead to bad results
	DWORD size = GetModuleFileName(NULL,
        state->exeFilePath, sizeof(state->exeFilePath));
	state->exeOnePastLastSlash = state->exeFilePath;
	for (char* scan = state->exeFilePath; *scan; scan++) {
		if (*scan == '\\') {
			state->exeOnePastLastSlash = scan + 1;
        }
    }
}
internal void Win32BuildExePathFileName(Win32State* state, char* fileName,
	int destCount, char* dest)
{
	CatStrings(state->exeOnePastLastSlash - state->exeFilePath,
        state->exeFilePath, StringLength(fileName), fileName,
		destCount, dest);
}

internal void Win32GetInputFileLocation(
	Win32State* state, bool32 inputStream,
    int slotIndex, int destCount, char* dest)
{
	char temp[64];
	wsprintf(temp, "recording_%d_%s.kmi",
        slotIndex, inputStream ? "input" : "state");
	Win32BuildExePathFileName(state, temp, destCount, dest);
}

inline FILETIME Win32GetLastWriteTime(char* fileName)
{
	WIN32_FILE_ATTRIBUTE_DATA data;
	if (!GetFileAttributesEx(fileName, GetFileExInfoStandard, &data)) {
		// TODO log?
		FILETIME zero = {};
		return zero;
	}
	return data.ftLastWriteTime;
}

internal Win32GameCode Win32LoadGameCode(
    char* gameCodeDLLPath, char* tempCodeDLLPath)
{
	Win32GameCode result = {};
	result.isValid = false;

	result.lastDLLWriteTime = Win32GetLastWriteTime(gameCodeDLLPath);
	CopyFile(gameCodeDLLPath, tempCodeDLLPath, FALSE);
	result.gameCodeDLL = LoadLibrary(tempCodeDLLPath);
	if (result.gameCodeDLL) {
		result.gameUpdateAndRender =
			(GameUpdateAndRenderFunc*)GetProcAddress(result.gameCodeDLL, "GameUpdateAndRender");

		result.isValid = result.gameUpdateAndRender != 0;
	}

	if (!result.isValid) {
		result.gameUpdateAndRender = 0;
	}

	return result;
}

internal void Win32UnloadGameCode(Win32GameCode* gameCode)
{
	if (gameCode->gameCodeDLL) {
		FreeLibrary(gameCode->gameCodeDLL);
    }

	gameCode->gameCodeDLL = NULL;
	gameCode->isValid = false;
	gameCode->gameUpdateAndRender = 0;
}

internal inline uint32 SafeTruncateUInt64(uint64 value)
{
	// TODO defines for maximum values
	DEBUG_ASSERT(value <= 0xFFFFFFFF);
	uint32 result = (uint32)value;
	return result;
}

#if GAME_INTERNAL

DEBUG_PLATFORM_PRINT_FUNC(DEBUGPlatformPrint)
{
    const int MSG_MAX_LENGTH = 1024;
    const char* MSG_PREFIX = "";
    const char* MSG_SUFFIX = "";
    char msg[MSG_MAX_LENGTH];

    int cx1 = snprintf(msg, MSG_MAX_LENGTH, "%s", MSG_PREFIX);
    if (cx1 < 0) return;

    va_list args;
    va_start(args, format);
    int cx2 = vsnprintf(msg + cx1, MSG_MAX_LENGTH - cx1, format, args);
    va_end(args);
    if (cx2 < 0) return;

    int cx3 = snprintf(msg + cx1 + cx2, MSG_MAX_LENGTH - cx1 - cx2,
        "%s", MSG_SUFFIX);
    if (cx3 < 0) return;

    if ((cx1 + cx2 + cx3) >= MSG_MAX_LENGTH) {
        // error message too long. warn? ignore for now
    }

    OutputDebugString(msg);
}

DEBUG_PLATFORM_FREE_FILE_MEMORY_FUNC(DEBUGPlatformFreeFileMemory)
{
	if (file->data) {
		VirtualFree(file->data, 0, MEM_RELEASE);
    }
}

DEBUG_PLATFORM_READ_FILE_FUNC(DEBUGPlatformReadFile)
{
	DEBUGReadFileResult result;
    result.data = nullptr;
    result.size = 0;
    
    char fullPath[MAX_PATH];
    CatStrings(StringLength(pathToApp_), pathToApp_,
        StringLength(fileName), fileName, MAX_PATH, fullPath);

	HANDLE hFile = CreateFile(fullPath, GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		// TODO log
		return result;
	}

	LARGE_INTEGER fileSize;
	if (!GetFileSizeEx(hFile, &fileSize)) {
		// TODO log
		return result;
	}

	uint32 fileSize32 = SafeTruncateUInt64(fileSize.QuadPart);
	result.data = VirtualAlloc(0, fileSize32,
		MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!result.data) {
		// TODO log
		return result;
	}

	DWORD bytesRead;
	if (!ReadFile(hFile, result.data, fileSize32, &bytesRead, NULL) ||
		fileSize32 != bytesRead) {
		// TODO log
		DEBUGPlatformFreeFileMemory(thread, &result);
		return result;
	}

	result.size = fileSize32;
	CloseHandle(hFile);
	return result;
}

DEBUG_PLATFORM_WRITE_FILE_FUNC(DEBUGPlatformWriteFile)
{
	HANDLE hFile = CreateFile(fileName, GENERIC_WRITE, NULL,
		NULL, CREATE_ALWAYS, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		// TODO log
		return false;
	}

	DWORD bytesWritten;
	if (!WriteFile(hFile, memory, memorySize, &bytesWritten, NULL))
	{
		// TODO log
		return false;
	}

	CloseHandle(hFile);
	return bytesWritten == memorySize;
}

#endif

LRESULT CALLBACK WndProc(
	HWND hWnd, UINT message,
	WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	switch (message) {
		case WM_ACTIVATEAPP: {
			// TODO handle
		} break;
		case WM_CLOSE: {
			// TODO handle this with a message?
			running_ = false;
		} break;
		case WM_DESTROY: {
			// TODO handle this as an error?
			running_ = false;
		} break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC hDC = BeginPaint(hWnd, &paint);
            Win32WindowDimension dimension = Win32GetWindowDimension(hWnd);
            Win32DisplayBufferInWindow(&backbuffer_, hDC,
                dimension.width, dimension.height);
            EndPaint(hWnd, &paint);
        } break;

        case WM_SIZE: {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            Win32ResizeBackbuffer(&backbuffer_, width, height);
        } break;

		case WM_SYSKEYDOWN: {
            DEBUG_PANIC("WM_SYSKEYDOWN in WndProc");
		} break;
		case WM_SYSKEYUP: {
            DEBUG_PANIC("WM_SYSKEYUP in WndProc");
		} break;
		case WM_KEYDOWN: {
		} break;
		case WM_KEYUP: {
		} break;

        case WM_CHAR: {
            char c = (char)wParam;
            input_->keyboardString[input_->keyboardStringLen++] = c;
            input_->keyboardString[input_->keyboardStringLen] = '\0';
        } break;

		default: {
			result = DefWindowProc(hWnd, message, wParam, lParam);
		} break;
	}

	return result;
}

internal int Win32KeyCodeToKM(int vkCode)
{
    // Numbers, letters, text
    if (vkCode >= 0x30 && vkCode <= 0x39) {
        return vkCode - 0x30 + KM_KEY_0;
    }
    else if (vkCode >= 0x41 && vkCode <= 0x5a) {
        return vkCode - 0x41 + KM_KEY_A;
    }
    else if (vkCode == VK_SPACE) {
        return KM_KEY_SPACE;
    }
    else if (vkCode == VK_BACK) {
        return KM_KEY_BACKSPACE;
    }
    // Arrow keys
    else if (vkCode == VK_UP) {
        return KM_KEY_ARROW_UP;
    }
    else if (vkCode == VK_DOWN) {
        return KM_KEY_ARROW_DOWN;
    }
    else if (vkCode == VK_LEFT) {
        return KM_KEY_ARROW_LEFT;
    }
    else if (vkCode == VK_RIGHT) {
        return KM_KEY_ARROW_RIGHT;
    }
    // Special keys
    else if (vkCode == VK_ESCAPE) {
        return KM_KEY_ESCAPE;
    }
    else if (vkCode == VK_SHIFT) {
        return KM_KEY_SHIFT;
    }
    else if (vkCode == VK_CONTROL) {
        return KM_KEY_CTRL;
    }
    else if (vkCode == VK_TAB) {
       return KM_KEY_TAB;
    }
    else if (vkCode == VK_RETURN) {
        return KM_KEY_ENTER;
    }
    else if (vkCode >= 0x60 && vkCode <= 0x69) {
        return vkCode - 0x60 + KM_KEY_NUMPAD_0;
    }
    else {
        return -1;
    }
}

internal void Win32ProcessMessages(HWND hWnd, GameInput* gameInput)
{
	MSG msg;
	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
		switch (msg.message) {
		case WM_QUIT: {
			running_ = false;
		} break;

		case WM_SYSKEYDOWN: {
			uint32 vkCode = (uint32)msg.wParam;
			bool32 altDown = (msg.lParam & (1 << 29));

			if (vkCode == VK_F4 && altDown) {
				running_ = false;
            }
		} break;
		case WM_SYSKEYUP: {
		} break;
		case WM_KEYDOWN: {
			uint32 vkCode = (uint32)msg.wParam;
			bool32 wasDown = ((msg.lParam & (1 << 30)) != 0);
			bool32 isDown = ((msg.lParam & (1 << 31)) == 0);
            int transitions = (wasDown != isDown) ? 1 : 0;
			DEBUG_ASSERT(isDown);

            int kmKeyCode = Win32KeyCodeToKM(vkCode);
            if (kmKeyCode != -1) {
                gameInput->keyboard[kmKeyCode].isDown = isDown;
                gameInput->keyboard[kmKeyCode].transitions = transitions;
            }

			if (vkCode == VK_ESCAPE) {
                // TODO eventually handle this in the game layer
				running_ = false;
            }
            if (vkCode == VK_F11) {
                Win32ToggleFullscreen(hWnd);
            }

            // Pass over to WndProc for WM_CHAR messages (string input)
            input_ = gameInput;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
		} break;
		case WM_KEYUP: {
			uint32 vkCode = (uint32)msg.wParam;
			bool32 wasDown = ((msg.lParam & (1 << 30)) != 0);
			bool32 isDown = ((msg.lParam & (1 << 31)) == 0);
            int transitions = (wasDown != isDown) ? 1 : 0;
			DEBUG_ASSERT(!isDown);

            int kmKeyCode = Win32KeyCodeToKM(vkCode);
            if (kmKeyCode != -1) {
                gameInput->keyboard[kmKeyCode].isDown = isDown;
                gameInput->keyboard[kmKeyCode].transitions = transitions;
            }
		} break;

        case WM_LBUTTONDOWN: {
            gameInput->mouseButtons[0].isDown = true;
            gameInput->mouseButtons[0].transitions = 1;
        } break;
        case WM_LBUTTONUP: {
            gameInput->mouseButtons[0].isDown = false;
            gameInput->mouseButtons[0].transitions = 1;
        } break;
        case WM_RBUTTONDOWN: {
            gameInput->mouseButtons[1].isDown = true;
            gameInput->mouseButtons[1].transitions = 1;
        } break;
        case WM_RBUTTONUP: {
            gameInput->mouseButtons[1].isDown = false;
            gameInput->mouseButtons[1].transitions = 1;
        } break;

        case WM_MOUSEWHEEL: {
            gameInput->mouseWheel += GET_WHEEL_DELTA_WPARAM(msg.wParam);
        } break;

		default: {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} break;
		}
	}
}

internal void Win32ClearInput(GameInput* input, GameInput* inputPrev)
{
    for (int i = 0; i < 5; i++) {
        input->mouseButtons[i].isDown = inputPrev->mouseButtons[i].isDown;
        input->mouseButtons[i].transitions = 0;
    }
    input->mousePos = inputPrev->mousePos;
    input->mouseWheel = inputPrev->mouseWheel;

    for (int i = 0; i < KM_KEY_LAST; i++) {
        input->keyboard[i].isDown = inputPrev->keyboard[i].isDown;
        input->keyboard[i].transitions = 0;
    }
    input->keyboardString[0] = '\0';
    input->keyboardStringLen = 0;
}

internal HWND Win32CreateWindow(
	HINSTANCE hInstance,
	const char* className, const char* windowName,
	int x, int y, int clientWidth, int clientHeight)
{
	WNDCLASSEX wndClass = { sizeof(wndClass) };
	wndClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wndClass.lpfnWndProc = WndProc;
	wndClass.hInstance = hInstance;
	//wndClass.hIcon = NULL;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.lpszClassName = className;

	if (!RegisterClassEx(&wndClass)) {
		// TODO log
		return NULL;
	}

	RECT windowRect		= {};
	windowRect.left		= x;
	windowRect.top		= y;
	windowRect.right	= x + clientWidth;
	windowRect.bottom	= y + clientHeight;

	if (!AdjustWindowRectEx(&windowRect, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
    FALSE, 0)) {
		// TODO log
		GetLastError();
		return NULL;
	}

	HWND hWindow = CreateWindowEx(
		0,
		className,
		windowName,
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		windowRect.left, windowRect.top,
		windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
		0,
		0,
		hInstance,
		0);

	if (!hWindow) {
		// TODO log
		return NULL;
	}

	return hWindow;
}

int CALLBACK WinMain(
	HINSTANCE hInstance, HINSTANCE hPrevInst,
	LPSTR cmdline, int cmd_show)
{
    #if GAME_SLOW
    debugPrint_ = DEBUGPlatformPrint;
    #endif

	Win32State state = {};
	Win32GetExeFilePath(&state);

    RemoveFileNameFromPath(state.exeFilePath, pathToApp_, MAX_PATH);
    DEBUG_PRINT("Path to executable: %s\n", pathToApp_);

	// Create window
	HWND hWnd = Win32CreateWindow(hInstance,
		"OpenGLWindowClass", "rasterizer",
		100, 100, START_WIDTH, START_HEIGHT);
	if (!hWnd) {
		return 1;
    }
    DEBUG_PRINT("Created Win32 window\n");

    // Get DC, use it forever because we specified CS_OWNDC
    HDC hDC = GetDC(hWnd);

    Win32ResizeBackbuffer(&backbuffer_, START_WIDTH, START_HEIGHT);

	// Try to get monitor refresh rate
	// TODO make this more reliable
	int monitorRefreshHz = 60;
    {
		int win32RefreshRate = GetDeviceCaps(hDC, VREFRESH);
		if (win32RefreshRate > 1) {
			monitorRefreshHz = win32RefreshRate;
        }
	}

#if GAME_INTERNAL
	LPVOID baseAddress = (LPVOID)TERABYTES((uint64)2);;
#else
	LPVOID baseAddress = 0;
#endif

	GameMemory gameMemory = {};
	gameMemory.permanentStorageSize = MEGABYTES(512);
	gameMemory.transientStorageSize = MEGABYTES(128);

	gameMemory.DEBUGPlatformPrint = DEBUGPlatformPrint;
	gameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
	gameMemory.DEBUGPlatformReadFile = DEBUGPlatformReadFile;
	gameMemory.DEBUGPlatformWriteFile = DEBUGPlatformWriteFile;

    gameMemory.DEBUGShouldInitGlobals = true;

	// TODO Look into using large virtual pages for this
    // potentially big allocation
	uint64 totalSize = gameMemory.permanentStorageSize
        + gameMemory.transientStorageSize;
	// TODO check allocation fail?
	gameMemory.permanentStorage = VirtualAlloc(baseAddress, (size_t)totalSize,
		MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	gameMemory.transientStorage = ((uint8*)gameMemory.permanentStorage +
		gameMemory.permanentStorageSize);

	state.gameMemorySize = totalSize;
	state.gameMemoryBlock = gameMemory.permanentStorage;
	if (!gameMemory.permanentStorage || !gameMemory.transientStorage) {
		// TODO log
		return 1;
	}
	DEBUG_PRINT("Initialized game memory\n");

	char gameCodeDLLPath[MAX_PATH];
	Win32BuildExePathFileName(&state, "rasterizer_game.dll",
        MAX_PATH, gameCodeDLLPath);
	char tempCodeDLLPath[MAX_PATH];
	Win32BuildExePathFileName(&state, "rasterizer_game_temp.dll",
        MAX_PATH, tempCodeDLLPath);

	GameInput input[2] = {};
	GameInput *newInput = &input[0];
	GameInput *oldInput = &input[1];

	// Initialize timing information
	int64 timerFreq;
	{
		LARGE_INTEGER timerFreqResult;
		QueryPerformanceFrequency(&timerFreqResult);
		timerFreq = timerFreqResult.QuadPart;
	}

	LARGE_INTEGER timerLast;
	QueryPerformanceCounter(&timerLast);
	uint64 cyclesLast = __rdtsc();

	Win32GameCode gameCode =
        Win32LoadGameCode(gameCodeDLLPath, tempCodeDLLPath);

	running_ = true;
	while (running_) {
		// TODO this gets called twice very quickly in succession
		FILETIME newDLLWriteTime = Win32GetLastWriteTime(gameCodeDLLPath);
		if (CompareFileTime(&newDLLWriteTime, &gameCode.lastDLLWriteTime) > 0) {
			Win32UnloadGameCode(&gameCode);
			gameCode = Win32LoadGameCode(gameCodeDLLPath, tempCodeDLLPath);
            gameMemory.DEBUGShouldInitGlobals = true;
		}

        // Process keyboard input & other messages
		Win32ProcessMessages(hWnd, newInput);

		POINT mousePos;
		GetCursorPos(&mousePos);
		ScreenToClient(hWnd, &mousePos);
        Vec2Int mousePosPrev = newInput->mousePos;
        newInput->mousePos.x = mousePos.x;
		newInput->mousePos.y = backbuffer_.height - mousePos.y;
        newInput->mouseDelta = newInput->mousePos - mousePosPrev;
        if (mousePos.x < 0 || mousePos.x > backbuffer_.width
        || mousePos.y < 0 || mousePos.y > backbuffer_.height) {
            for (int i = 0; i < 5; i++) {
                int transitions = newInput->mouseButtons[i].isDown ? 1 : 0;
                newInput->mouseButtons[i].isDown = false;
                newInput->mouseButtons[i].transitions = transitions;
            }
        }

		/*newInput->mouseButtons[0].isDown = (int32)GetKeyState(VK_LBUTTON);
		newInput->mouseButtons[1].isDown = (int32)GetKeyState(VK_RBUTTON);
		newInput->mouseButtons[2].isDown = (int32)GetKeyState(VK_MBUTTON);
		newInput->mouseButtons[3].isDown = (int32)GetKeyState(VK_XBUTTON1);
		newInput->mouseButtons[4].isDown = (int32)GetKeyState(VK_XBUTTON2);*/

		ThreadContext thread = {};
        GameBackbuffer gameBackbuffer = {};
        gameBackbuffer.data = backbuffer_.data;
        gameBackbuffer.width = backbuffer_.width;
        gameBackbuffer.height = backbuffer_.height;
        gameBackbuffer.bytesPerPixel = backbuffer_.bytesPerPixel;
        gameBackbuffer.depth = backbuffer_.depth;
        
        LARGE_INTEGER timerEnd;
        QueryPerformanceCounter(&timerEnd);
        uint64 cyclesEnd = __rdtsc();
        int64 cyclesElapsed = cyclesEnd - cyclesLast;
        int64 timerElapsed = timerEnd.QuadPart - timerLast.QuadPart;
        float64 elapsed = (float64)timerElapsed / timerFreq;
        float64 fps = (float64)timerFreq / timerElapsed;
        int32 mCyclesPerFrame = (int32)(cyclesElapsed / (1000 * 1000));
        timerLast = timerEnd;
        cyclesLast = cyclesEnd;
        /*DEBUG_PRINT("%fs/f, %ff/s, %dMc/f\n",
            elapsed, fps, mCyclesPerFrame);*/

		if (gameCode.gameUpdateAndRender) {
			gameCode.gameUpdateAndRender(&thread, &gameMemory,
                &gameBackbuffer, newInput, elapsed);
        }

        Win32WindowDimension dimension = Win32GetWindowDimension(hWnd);
        Win32DisplayBufferInWindow(&backbuffer_, hDC,
            dimension.width, dimension.height);

		GameInput *temp = newInput;
		newInput = oldInput;
		oldInput = temp;
        Win32ClearInput(newInput, oldInput);
	}

	return 0;
}
