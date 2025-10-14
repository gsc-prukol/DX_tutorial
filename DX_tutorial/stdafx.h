#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif

#include <windows.h>

HWND hwnd = nullptr;
LPCTSTR WindowsName = L"DX12 Tutorial";
LPCTSTR WindowsTitle = L"DX12 Tutorial Window";


// create a window
bool InitializeWindow(HINSTANCE hInstance,
	int ShowWnd,
	int width, int height,
	bool fullscreen);

// main application loop
void Mainloop();

// callback function for windows messages
LRESULT CALLBACK WndProc(HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam);
