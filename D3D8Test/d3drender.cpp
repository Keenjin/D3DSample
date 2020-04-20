#include "d3drender.h"

IDirect3D8* g_d3d8 = NULL;
IDirect3DDevice8* g_d3d8Device = NULL;
IDirect3DVertexBuffer8* g_pVB = NULL;

struct CUSTOMVERTEX
{
	float x, y, z, rhw; // The position
	D3DCOLOR    color;    // The color
};

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)

void InitDevice(HWND hWnd)
{
	g_d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.hDeviceWindow = hWnd;
	HRESULT hr = g_d3d8->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &g_d3d8Device);

	g_d3d8Device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	g_d3d8Device->SetRenderState(D3DRS_LIGHTING, FALSE);
	g_d3d8Device->SetRenderState(D3DRS_ZENABLE, FALSE);

	// 定义顶点
	g_d3d8Device->CreateVertexBuffer(3 * sizeof(CUSTOMVERTEX), 0, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &g_pVB);
	CUSTOMVERTEX* pVertices;
	if (FAILED(g_pVB->Lock(0, 0, (BYTE**)&pVertices, 0)))
	{
		return;
	}

	pVertices[0].x = 100.0f;
	pVertices[0].y = 150.0f;
	pVertices[0].z = 1.0f;
	pVertices[0].rhw = 1.0f;
	pVertices[0].color = D3DCOLOR_XRGB(255, 0, 0);

	pVertices[1].x = 250.0f;
	pVertices[1].y = 150.0f;
	pVertices[1].z = 1.0f;
	pVertices[1].rhw = 1.0f;
	pVertices[1].color = D3DCOLOR_XRGB(0, 255, 0);

	pVertices[2].x = 150.0f;
	pVertices[2].y = 250.0f;
	pVertices[2].z = 1.0f;
	pVertices[2].rhw = 1.0f;
	pVertices[2].color = D3DCOLOR_XRGB(0, 0, 255);

	g_pVB->Unlock();
}

void Render()
{
	if (g_d3d8Device && g_pVB)
	{
		g_d3d8Device->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0);

		g_d3d8Device->BeginScene();

		HRESULT hr = g_d3d8Device->SetStreamSource(0, g_pVB, sizeof(CUSTOMVERTEX));
		g_d3d8Device->SetVertexShader(D3DFVF_CUSTOMVERTEX);
		hr = g_d3d8Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 1);

		g_d3d8Device->EndScene();

		OnPrePresent();

		g_d3d8Device->Present(0, 0, 0, 0);
	}
}

void UnInitDevice()
{
	if (g_pVB)
	{
		g_pVB->Release();
	}
	if (g_d3d8Device)
	{
		g_d3d8Device->Release();
	}

	if (g_d3d8)
	{
		g_d3d8->Release();
	}
}

void OnPrePresent()
{
	static bool bFirst = true;

	if (bFirst)
	{
		bFirst = false;
		// Present之前，创建一个兼容的离屏表面
		IDirect3DSurface8* pBackBuffer = NULL;
		g_d3d8Device->GetRenderTarget(&pBackBuffer);

		D3DSURFACE_DESC desc;
		pBackBuffer->GetDesc(&desc);
		IDirect3DSurface8* pCopyBuffer = NULL;
		g_d3d8Device->CreateImageSurface(desc.Width, desc.Height, desc.Format, &pCopyBuffer);
		pCopyBuffer->UnlockRect();
		g_d3d8Device->CopyRects(pBackBuffer, NULL, 0, pCopyBuffer, NULL);
		
		SaveSurfaceToFile(pCopyBuffer, L"d:\\d3d8.bmp");
		pCopyBuffer->Release();
		pBackBuffer->Release();
	}
}

void SaveSurfaceToFile(IDirect3DSurface8* pCopyBuffer, LPCWSTR lpstrFileName)
{
	BITMAPFILEHEADER       bmfHdr;           //位图文件头结构
	BITMAPINFOHEADER       bi;               //位图信息头结构
	LPBITMAPINFOHEADER   lpbi;               //指向位图信息头结构

	bi.biSize = sizeof(BITMAPINFOHEADER);
	D3DSURFACE_DESC desc;
	pCopyBuffer->GetDesc(&desc);
	bi.biWidth = desc.Width;
	bi.biHeight = desc.Height;
	bi.biPlanes = 1;
	HDC hDC = CreateDC(L"DISPLAY", NULL, NULL, NULL);
	int iBits = GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES);
	DeleteDC(hDC);
	if (iBits <= 1) bi.biBitCount = 1;
	else if (iBits <= 4) bi.biBitCount = 4;
	else if (iBits <= 8) bi.biBitCount = 8;
	else if (iBits <= 24) bi.biBitCount = 24;
	else bi.biBitCount = iBits;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	DWORD dwBmBitsSize = ((desc.Width *
		bi.biBitCount + 31) / 32) * 4
		* desc.Height;

	//计算调色板大小
	DWORD dwPaletteSize = 0;
	if (bi.biBitCount <= 8)
		dwPaletteSize = (1 << bi.biBitCount) * sizeof(RGBQUAD);

	//   设置位图文件头
	bmfHdr.bfType = 0x4D42;     //   "BM "
	DWORD dwDIBSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)
		+ dwPaletteSize + dwBmBitsSize;
	bmfHdr.bfSize = dwDIBSize;
	bmfHdr.bfReserved1 = 0;
	bmfHdr.bfReserved2 = 0;
	bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER)
		+ (DWORD)sizeof(BITMAPINFOHEADER)
		+ dwPaletteSize;

	//为位图内容分配内存
	HANDLE hDib = GlobalAlloc(GHND, dwBmBitsSize +
		dwPaletteSize + sizeof(BITMAPINFOHEADER));
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
	*lpbi = bi;
	//   处理调色板      
	HPALETTE hPal = (HPALETTE)GetStockObject(DEFAULT_PALETTE);
	HPALETTE hOldPal = NULL;
	if (hPal)
	{
		hDC = GetDC(NULL);
		hOldPal = SelectPalette(hDC, hPal, FALSE);
		RealizePalette(hDC);
	}
	//   获取该调色板下新的像素值
	D3DLOCKED_RECT data;
	pCopyBuffer->LockRect(&data, NULL, 0);
	
	
	//恢复调色板      
	if (hOldPal)
	{
		SelectPalette(hDC, hOldPal, TRUE);
		RealizePalette(hDC);
		ReleaseDC(NULL, hDC);
	}

	HANDLE hFile = CreateFile(lpstrFileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	//   写入位图文件头
	DWORD dwWritten = 0;
	WriteFile(hFile, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
	//   写入位图文件其余内容
	void* pData = lpbi + dwPaletteSize + sizeof(BITMAPINFOHEADER);
	memcpy(pData, data.pBits, dwBmBitsSize);
	pCopyBuffer->UnlockRect();

	WriteFile(hFile, (LPSTR)lpbi, dwDIBSize, &dwWritten, NULL);
	GlobalUnlock(hDib);
	GlobalFree(hDib);
	CloseHandle(hFile);
}
