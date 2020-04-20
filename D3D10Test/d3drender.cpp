#include "d3drender.h"
#include <d3d10.h>
#include "d3dx10.h"
#include <d3dcompiler.h>
#include <directxmath.h>

ID3D10Device* g_pd3dDevice = NULL;
IDXGISwapChain* g_pSwapChain = NULL;
ID3D10RenderTargetView* g_pRenderTargetView = NULL;
ID3D10InputLayout* g_pVertexLayout = NULL;
ID3D10Buffer* g_pVertexBuffer = NULL;
ID3D10VertexShader* g_pVertexShader = NULL;
ID3D10PixelShader* g_pPixelShader = NULL;

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

    if (FAILED(D3D10CreateDeviceAndSwapChain(NULL, D3D10_DRIVER_TYPE_REFERENCE, NULL,
        0, D3D10_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice)))
    {
        return;
    }

    // Create a render target view
    ID3D10Texture2D* pBackBuffer;
    if (FAILED(g_pSwapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), (LPVOID*)&pBackBuffer)))
        return;
    HRESULT hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr))
        return;
    g_pd3dDevice->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);

    D3D10_VIEWPORT vp;
    vp.Width = 640;
    vp.Height = 480;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pd3dDevice->RSSetViewports(1, &vp);

    // Define the input layout
    D3D10_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = sizeof(layout) / sizeof(layout[0]);

    // Create the input layout
    ID3DBlob* blob;
    D3DReadFileToBlob(L"G:\\Solution\\Debug\\VertexShader.cso", &blob);
    g_pd3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), &g_pVertexShader);
    // 创建顶点布局
    g_pd3dDevice->CreateInputLayout(layout, numElements,
        blob->GetBufferPointer(), blob->GetBufferSize(), &g_pVertexLayout);
    blob->Release();

    // Set the input layout
    g_pd3dDevice->IASetInputLayout(g_pVertexLayout);

    D3DReadFileToBlob(L"G:\\Solution\\Debug\\PixelShader.cso", &blob);
    g_pd3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), &g_pPixelShader);
    blob->Release();
    g_pd3dDevice->VSSetShader(g_pVertexShader);
    g_pd3dDevice->PSSetShader(g_pPixelShader);

    // Create vertex buffer
    SimpleVertex vertices[] =
    {
        {  DirectX::XMFLOAT3(0.0f, 0.5f, 0.5f),  DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
        {  DirectX::XMFLOAT3(0.5f, -0.5f, 0.5f),  DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
        {  DirectX::XMFLOAT3(-0.5f, -0.5f, 0.5f),  DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
    };
    D3D10_BUFFER_DESC bd;
    bd.Usage = D3D10_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertex) * 3;
    bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;
    D3D10_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = vertices;
    if (FAILED(g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer)))
        return;

    // Set vertex buffer
    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    g_pd3dDevice->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

    // Set primitive topology
    g_pd3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Render()
{
    //
    // Clear the backbuffer
    //
    if (g_pd3dDevice && g_pSwapChain)
    {
        float ClearColor[4] = { 0.0f, 0.125f, 0.6f, 1.0f }; // RGBA
        g_pd3dDevice->ClearRenderTargetView(g_pRenderTargetView, ClearColor);
        
        // Render a triangle
        g_pd3dDevice->Draw(3, 0);

        OnPrePresent();

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
    if (g_pSwapChain) g_pSwapChain->Release();
}

void OnPrePresent()
{
    static bool bFirst = true;

    if (bFirst)
    {
        bFirst = false;
        // 获取后备缓存
        ID3D10Resource* pBackBuffer = NULL;
        g_pSwapChain->GetBuffer(0, __uuidof(ID3D10Resource), (void**)&pBackBuffer);
        DXGI_SWAP_CHAIN_DESC swapdesc;
        g_pSwapChain->GetDesc(&swapdesc);


        // 创建一个CPU和GPU皆能访问的texture2D资源
        D3D10_TEXTURE2D_DESC desc = {};
        desc.Width = swapdesc.BufferDesc.Width;
        desc.Height = swapdesc.BufferDesc.Height;
        desc.Format = swapdesc.BufferDesc.Format;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D10_USAGE_STAGING;
        desc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;

        ID3D10Texture2D* tex = NULL;
        g_pd3dDevice->CreateTexture2D(&desc, nullptr, &tex);
        g_pd3dDevice->CopyResource(tex, pBackBuffer);

        D3DX10SaveTextureToFile(tex, D3DX10_IMAGE_FILE_FORMAT::D3DX10_IFF_BMP, L"D:\\d3d10.bmp");

        if (pBackBuffer) pBackBuffer->Release();
        if (tex) tex->Release();
    }
}