#include "d3drender.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>

ID3D11Device* g_pd3dDevice = NULL;
IDXGISwapChain* g_pSwapChain = NULL;
ID3D11DeviceContext* g_pd3dImmediateContext = NULL;
ID3D11RenderTargetView* g_pRenderTargetView = NULL;
ID3D11VertexShader* g_pVertexShader = NULL;
ID3D11PixelShader* g_pPixelShader = NULL;
ID3D11InputLayout* g_pVertexLayout = NULL;
ID3D11Buffer* g_pVertexBuffer = NULL;

struct SimpleVertex
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT4 color;
};

void InitDevice(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 640;
    sd.BufferDesc.Height = 480;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

	D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, NULL, &g_pd3dImmediateContext);
    
    // Create a render target view
    ID3D11Texture2D* pBackBuffer;
    if (FAILED(g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer)))
        return;
    HRESULT hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr))
        return;
    g_pd3dImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);

    D3D11_VIEWPORT vp;
    vp.Width = 640;
    vp.Height = 480;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pd3dImmediateContext->RSSetViewports(1, &vp);

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = sizeof(layout) / sizeof(layout[0]);

    // Create the input layout
    ID3DBlob* blob;
    D3DReadFileToBlob(L"G:\\Solution\\Debug\\VertexShader.cso", &blob);
    g_pd3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), NULL, &g_pVertexShader);
    // 创建顶点布局
    g_pd3dDevice->CreateInputLayout(layout, numElements,
        blob->GetBufferPointer(), blob->GetBufferSize(), &g_pVertexLayout);
    blob->Release();

    // Set the input layout
    g_pd3dImmediateContext->IASetInputLayout(g_pVertexLayout);

    D3DReadFileToBlob(L"G:\\Solution\\Debug\\PixelShader.cso", &blob);
    g_pd3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), NULL, &g_pPixelShader);
    blob->Release();
    g_pd3dImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
    g_pd3dImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);

    // Create vertex buffer
    SimpleVertex vertices[] =
    {
        { DirectX::XMFLOAT3(0.0f, 0.5f, 0.5f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
        { DirectX::XMFLOAT3(0.5f, -0.5f, 0.5f), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
        { DirectX::XMFLOAT3(-0.5f, -0.5f, 0.5f), DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
    };
    D3D11_BUFFER_DESC bd;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertex) * 3;
    bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = vertices;
    if (FAILED(g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer)))
        return;

    // Set vertex buffer
    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    g_pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

    // Set primitive topology
    g_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Render()
{
    //
   // Clear the backbuffer
   //
    if (g_pd3dDevice && g_pSwapChain)
    {
        float ClearColor[4] = { 0.0f, 0.125f, 0.6f, 1.0f }; // RGBA
        g_pd3dImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);

        // Render a triangle
        g_pd3dImmediateContext->Draw(3, 0);

        g_pSwapChain->Present(0, 0);
    }
}

void UnInitDevice()
{
    if (g_pVertexBuffer) g_pVertexBuffer->Release();
    if (g_pVertexLayout) g_pVertexLayout->Release();
    if (g_pVertexShader) g_pVertexShader->Release();
    if (g_pRenderTargetView) g_pRenderTargetView->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();
    if (g_pd3dImmediateContext) g_pd3dImmediateContext->Release();
    if (g_pSwapChain) g_pSwapChain->Release();
}
