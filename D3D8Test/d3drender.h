#pragma once

#include "framework.h"
#include "d3d8.h"

void InitDevice(HWND hWnd);
void Render();
void UnInitDevice();
void OnPrePresent();
void SaveSurfaceToFile(IDirect3DSurface8* pCopyBuffer, LPCWSTR lpstrFileName);