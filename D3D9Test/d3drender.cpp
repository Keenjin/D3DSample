#include "d3drender.h"
#include <atlstr.h>

IDirect3D9Ex* g_pD3D9Ex = NULL;
IDirect3DDevice9Ex* g_pD3D9DeviceEx = NULL;
IDirect3DVertexBuffer9* g_pVB = NULL;
IDirect3DSurface9* g_pImgFromFile = NULL;


struct CUSTOMVERTEX
{
	D3DXVECTOR4 position; // The position
	D3DCOLOR    color;    // The color
};

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)

void InitDevice(HWND hWnd)
{
	HRESULT hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &g_pD3D9Ex);
	if (FAILED(hr))
	{
		ATL::CAtlString strErr;
		strErr.Format(L"errcode: 0x%0x", hr);
		MessageBoxW(hWnd, L"Direct3DCreate9Ex failed. " + strErr, NULL, MB_OK);
		return;
	}

	RECT rcWnd;
	GetClientRect(hWnd, &rcWnd);

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	d3dpp.hDeviceWindow = hWnd;
	//d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	//d3dpp.EnableAutoDepthStencil = TRUE;
	// 创建的IDirect3DDevice9，相当于一个显示器设备，后续的处理，也是专门针对显示器设备而来的
	if (FAILED(hr = g_pD3D9Ex->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,        // 软件顶点处理。如果显卡支持硬件顶点处理，最好是使用显卡
		&d3dpp, NULL, &g_pD3D9DeviceEx)))
	{
		ATL::CAtlString strErr;
		strErr.Format(L"errcode: 0x%0x", hr);
		MessageBoxW(hWnd, L"CreateDeviceEx failed. " + strErr, NULL, MB_OK);
		return;
	}

	//g_pD3D9DeviceEx->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	//g_pD3D9DeviceEx->SetRenderState(D3DRS_LIGHTING, FALSE);
	//g_pD3D9DeviceEx->SetRenderState(D3DRS_ZENABLE, FALSE);

	// 定义顶点
	/*hr = g_pD3D9DeviceEx->CreateVertexBuffer(3 * sizeof(CUSTOMVERTEX), 0, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &g_pVB, NULL);
	if (FAILED(hr))
	{
		ATL::CAtlString strErr;
		strErr.Format(L"errcode: 0x%0x", hr);
		MessageBoxW(hWnd, L"CreateVertexBuffer failed. " + strErr, NULL, MB_OK);
		return;
	}
	CUSTOMVERTEX* pVertices;
	if (FAILED(g_pVB->Lock(0, 0, (void**)&pVertices, 0)))
	{
		ATL::CAtlString strErr;
		strErr.Format(L"errcode: 0x%0x", hr);
		MessageBoxW(hWnd, L"VB->Lock failed. " + strErr, NULL, MB_OK);
		return;
	}

	pVertices[0].position = D3DXVECTOR4(100.0f, 150.0f, 1.0f, 1.0f);
	pVertices[0].color = D3DCOLOR_XRGB(255, 0, 0);
	pVertices[1].position = D3DXVECTOR4(250.0f, 150.0f, 1.0f, 1.0f);
	pVertices[1].color = D3DCOLOR_XRGB(0, 255, 0);
	pVertices[2].position = D3DXVECTOR4(150.0f, 250.0f, 1.0f, 1.0f);
	pVertices[2].color = D3DCOLOR_XRGB(0, 0, 255);
	
	g_pVB->Unlock();*/
	D3DXIMAGE_INFO ii;
	D3DXGetImageInfoFromFile(L"G:\\Solution\\Debug\\test.jpg", &ii);
	g_pD3D9DeviceEx->CreateOffscreenPlainSurface(rcWnd.right-rcWnd.left, rcWnd.bottom-rcWnd.top, ii.Format, D3DPOOL_SYSTEMMEM, &g_pImgFromFile, NULL);

	D3DXLoadSurfaceFromFileW(g_pImgFromFile, NULL, NULL, L"G:\\Solution\\Debug\\test.jpg", NULL, D3DX_FILTER_NONE, 0, NULL);
}

void Render()
{
	if (g_pD3D9DeviceEx/* && g_pVB*/)
	{
		g_pD3D9DeviceEx->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0);

		if (SUCCEEDED(g_pD3D9DeviceEx->BeginScene()))
		{
			//g_pD3D9DeviceEx->SetStreamSource(0, g_pVB, 0, sizeof(CUSTOMVERTEX));
			//g_pD3D9DeviceEx->SetFVF(D3DFVF_CUSTOMVERTEX);
			//g_pD3D9DeviceEx->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);

			IDirect3DSurface9* pBackBuffer = NULL;
			//g_pD3D9DeviceEx->GetBackBuffer(0,0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
			g_pD3D9DeviceEx->GetRenderTarget(0, &pBackBuffer);
			//g_pD3D9DeviceEx->StretchRect(g_pImgFromFile, NULL, pBackBuffer, NULL, D3DTEXTUREFILTERTYPE::D3DTEXF_NONE);
			g_pD3D9DeviceEx->UpdateSurface(g_pImgFromFile, NULL, pBackBuffer, NULL);
			pBackBuffer->Release();

			g_pD3D9DeviceEx->EndScene();
		}

		OnPrePresent();

		g_pD3D9DeviceEx->PresentEx(0, 0, 0, 0, 0);
	}
}

void UnInitDevice()
{
	if (g_pVB)
	{
		g_pVB->Release();
	}

	if (g_pD3D9DeviceEx)
	{
		g_pD3D9DeviceEx->Release();
	}

	if (g_pD3D9Ex)
	{
		g_pD3D9Ex->Release();
	}
}

void OnPrePresent()
{
	static bool bFirst = true;

	if (bFirst)
	{
		bFirst = false;
		// Present之前，创建一个兼容的离屏表面
		IDirect3DSurface9* pBackBuffer = NULL;
		g_pD3D9DeviceEx->GetRenderTarget(0, &pBackBuffer);

		D3DSURFACE_DESC desc;
		pBackBuffer->GetDesc(&desc);
		IDirect3DSurface9* pCopyBuffer = NULL;
		g_pD3D9DeviceEx->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &pCopyBuffer, NULL);
		pCopyBuffer->UnlockRect();
		//g_pD3D9DeviceEx->StretchRect(pBackBuffer, NULL, pCopyBuffer, NULL, D3DTEXTUREFILTERTYPE::D3DTEXF_NONE);
		g_pD3D9DeviceEx->GetRenderTargetData(pBackBuffer, pCopyBuffer);
		D3DXSaveSurfaceToFileW(L"d:\\d3d9.bmp", D3DXIMAGE_FILEFORMAT::D3DXIFF_BMP, pCopyBuffer, NULL, NULL);
		pCopyBuffer->Release();
		pBackBuffer->Release();
	}
}
