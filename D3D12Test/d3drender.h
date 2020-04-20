#pragma once
#include "framework.h"

void InitDevice(HWND hWnd);
void Render();
void UnInitDevice();
void LoadPipeline(HWND hWnd);
void LoadAssets();
void WaitForPreviousFrame();
void PopulateCommandList();