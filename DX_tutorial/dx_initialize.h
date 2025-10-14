#pragma once

#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }

#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

extern int Width;
extern int Height;

extern bool FullScreen;
// we will exit the program when this becomes false
extern bool Running;
extern HANDLE fenceEvent;

// functions declarations
bool InitD3D(HWND hwnd);
void Update();
void UpdatePipeline();
void Render();
void Cleanup();
void WaitForPreviousFrame();
