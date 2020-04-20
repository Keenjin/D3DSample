#pragma once

#include "framework.h"
#include <d3d9.h>
#include <d3dx9.h>

extern IDirect3D9* g_pD3D9;
extern IDirect3DDevice9Ex* g_pD3D9DeviceEx;

void InitDevice(HWND hWnd);
void Render();
void UnInitDevice();
void OnPrePresent();