#include "d3drender.h"
#include <wrl.h>
using Microsoft::WRL::ComPtr;

#include <d3d11on12.h>
#include "d3dx11.h"
#include "d3dx12.h"
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <stdexcept>

ComPtr<ID3D12Device> g_pd3dDevice;
ComPtr<ID3D12CommandQueue> g_pd3dCommandQueue;
ComPtr<IDXGISwapChain3> g_pd3dSwapChain;
ComPtr<ID3D12DescriptorHeap> g_prtvDescHeap;
UINT g_rtvDescriptorSize = 0;
ComPtr<ID3D12Resource> g_pRenderTarget[2];
ComPtr<ID3D12CommandAllocator> g_pd3dCommandAllocator;
ComPtr<ID3D12RootSignature> g_rootSign;
ComPtr<ID3D12GraphicsCommandList> g_commandList;
ComPtr<ID3D12PipelineState> g_pipelineState;
ComPtr<ID3D12Resource> g_pVB;
D3D12_VERTEX_BUFFER_VIEW g_vertexBufferView;
ComPtr<ID3D12Fence> g_pFence;
UINT64 g_fenceValue = 0;
HANDLE g_hfenceEvent = NULL;
CD3DX12_VIEWPORT g_viewport;
CD3DX12_RECT g_scissorRect;
UINT g_frameCnt = 2;
UINT g_frameIndex = 0;

struct Vertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
};

inline std::string HrToString(HRESULT hr)
{
    char s_str[64] = {};
    sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
    return std::string(s_str);
}

class HrException : public std::runtime_error
{
public:
    HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
    HRESULT Error() const { return m_hr; }
private:
    const HRESULT m_hr;
};

void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw HrException(hr);
    }
}

void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter)
{
    ComPtr<IDXGIAdapter1> adapter;
    *ppAdapter = nullptr;

    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Don't select the Basic Render Driver adapter.
            // If you want a software adapter, pass in "/warp" on the command line.
            continue;
        }

        // Check to see if the adapter supports Direct3D 12, but don't create the
        // actual device yet.
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
        {
            break;
        }
    }

    *ppAdapter = adapter.Detach();
}

void LoadPipeline(HWND hWnd)
{
    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    // Step1：图形调试器支持
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            // Enable additional debug layers.
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif
    // Step2：创建设备
    ComPtr<IDXGIFactory4> factory;
    CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));

    ComPtr<IDXGIAdapter1> hardwareAdapter;
    GetHardwareAdapter(factory.Get(), &hardwareAdapter);

    D3D12CreateDevice(
        hardwareAdapter.Get(),
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&g_pd3dDevice)
    );

    // Step3：创建命令队列（GPU和CPU通信，使用一堆命令队列依次执行，是否可以理解为，CPU和GPU通信方式修改成了异步方式了？）
    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    g_pd3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_pd3dCommandQueue));

    // Step4：创建交换链
    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = g_frameCnt;      // 至少有两个
    swapChainDesc.Width = 640;
    swapChainDesc.Height = 480;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        g_pd3dCommandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
        hWnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
    ));
    swapChain.As(&g_pd3dSwapChain);

    // Step5：创建渲染器目标视图(RTV) 描述符堆，类似一个资源池，可以用来分配给渲染目标等。
    // Describe and create a render target view (RTV) descriptor heap.
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = g_frameCnt;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    g_pd3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&g_prtvDescHeap));
    g_rtvDescriptorSize = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Step6：创建帧资源（每个帧的渲染器目标视图），每次创建，都需要找描述符堆申请资源
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(g_prtvDescHeap->GetCPUDescriptorHandleForHeapStart());

    for (UINT n = 0; n < g_frameCnt; n++)
    {
        ThrowIfFailed(g_pd3dSwapChain->GetBuffer(n, IID_PPV_ARGS(&g_pRenderTarget[n])));
        g_pd3dDevice->CreateRenderTargetView(g_pRenderTarget[n].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, g_rtvDescriptorSize);
    }

    // Step7：创建命令分配器，管理命令列表
    g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_pd3dCommandAllocator));
}

void LoadAssets()
{
    // Step8：创建根签名，类似签名证书根证书一样，资源一旦“打”上这个签名（结构体中指向根签名对象），就表示要进入渲染管道进行渲染使用
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    g_pd3dDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&g_rootSign));

    // Step9：编译着色器
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif
    ThrowIfFailed(D3DCompileFromFile(L"G:\\Solution\\D3D12Test\\shader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
    ThrowIfFailed(D3DCompileFromFile(L"G:\\Solution\\D3D12Test\\shader.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

    // Step10：定义顶点输入布局
    // Define the vertex input layout.
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    g_viewport.TopLeftX = 100.0f;
    g_viewport.TopLeftY = 10.0f;
    g_viewport.Width = 400.0f;
    g_viewport.Height = 250.0f;
    g_scissorRect.left = 0.0f;
    g_scissorRect.top = 0.0f;
    g_scissorRect.right = 640.0f;
    g_scissorRect.bottom = 480.0f;

    // Step11：创建图形管道状态对象，图形管道状态对象包括顶点着色器、像素着色器、顶点输入布局、顶点拓扑结构、渲染目标格式等
    // Describe and create the graphics pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = g_rootSign.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    g_pd3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_pipelineState));

    // Step12：创建命令列表
    g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_pd3dCommandAllocator.Get(), g_pipelineState.Get(), IID_PPV_ARGS(&g_commandList));

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    g_commandList->Close();

    // Step13：创建并加载顶点缓冲区
    Vertex triangleVertices[] =
    {
        { { 0.0f, 0.25f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { 0.25f, -0.25f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { -0.25f, -0.25f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
    };

    const UINT vertexBufferSize = sizeof(triangleVertices);

    // Note: using upload heaps to transfer static data like vert buffers is not 
    // recommended. Every time the GPU needs it, the upload heap will be marshalled 
    // over. Please read up on Default Heap usage. An upload heap is used here for 
    // code simplicity and because there are very few verts to actually transfer.
    g_pd3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&g_pVB));

    // Copy the triangle data to the vertex buffer.
    UINT8* pVertexDataBegin;
    CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
    g_pVB->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
    memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
    g_pVB->Unmap(0, nullptr);

    // Step14：创建顶点缓冲区视图
    // Initialize the vertex buffer view.
    g_vertexBufferView.BufferLocation = g_pVB->GetGPUVirtualAddress();
    g_vertexBufferView.StrideInBytes = sizeof(Vertex);
    g_vertexBufferView.SizeInBytes = vertexBufferSize;

    // Step15：创建fence，用于GPU和CPU同步
    g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_pFence));
    g_fenceValue = 1;

    // Create an event handle to use for frame synchronization.
    g_hfenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (g_hfenceEvent == nullptr)
    {
        return;
    }

    // Step16：等待GPU完成准备工作
    // Wait for the command list to execute; we are reusing the same command 
    // list in our main loop but for now, we just want to wait for setup to 
    // complete before continuing.

    WaitForPreviousFrame();
}

void InitDevice(HWND hWnd)
{
    // 加载管线
    LoadPipeline(hWnd);

    // 记载资产
    LoadAssets();
}

void WaitForPreviousFrame()
{
    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
    // sample illustrates how to use fences for efficient resource usage and to
    // maximize GPU utilization.

    // Signal and increment the fence value.
    const UINT64 fence = g_fenceValue;
    g_pd3dCommandQueue->Signal(g_pFence.Get(), fence);
    g_fenceValue++;

    // Wait until the previous frame is finished.
    if (g_pFence->GetCompletedValue() < fence)
    {
        g_pFence->SetEventOnCompletion(fence, g_hfenceEvent);
        WaitForSingleObject(g_hfenceEvent, INFINITE);
    }

    g_frameIndex = g_pd3dSwapChain->GetCurrentBackBufferIndex();
}

void PopulateCommandList()
{
    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    g_pd3dCommandAllocator->Reset();

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    g_commandList->Reset(g_pd3dCommandAllocator.Get(), g_pipelineState.Get());

    // Set necessary state.
    g_commandList->SetGraphicsRootSignature(g_rootSign.Get());
    g_commandList->RSSetViewports(1, &g_viewport);
    g_commandList->RSSetScissorRects(1, &g_scissorRect);

    // Indicate that the back buffer will be used as a render target.
    g_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(g_pRenderTarget[g_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(g_prtvDescHeap->GetCPUDescriptorHandleForHeapStart(), 0, g_rtvDescriptorSize);
    g_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Record commands.
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    g_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    g_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_commandList->IASetVertexBuffers(0, 1, &g_vertexBufferView);
    g_commandList->DrawInstanced(3, 1, 0, 0);

    // Indicate that the back buffer will now be used to present.
    g_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(g_pRenderTarget[g_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    g_commandList->Close();
}

void Render()
{
    if (!g_pFence)
    {
        return;
    }
    // Step17：填充命令列表
    PopulateCommandList();

    // Step18：执行命令列表
    ID3D12CommandList* ppCommandLists[] = { g_commandList.Get() };
    g_pd3dCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    OnPrePresent();

    // Step19：触发swapchain，显示帧
    g_pd3dSwapChain->Present(1, 0);

    // Step20：等待GPU完成动作
    WaitForPreviousFrame();
}

void UnInitDevice()
{
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    WaitForPreviousFrame();

    CloseHandle(g_hfenceEvent);
}

void OnPrePresent()
{
    static bool bFirst = true;

    if (bFirst)
    {
        bFirst = false;
        DXGI_SWAP_CHAIN_DESC swapdesc;
        g_pd3dSwapChain->GetDesc(&swapdesc);

        ComPtr<ID3D11Device> pd3d11Device;
        ComPtr<ID3D11DeviceContext> pd3d11Context;
        // d3d12需要使用d3d11on12技术，通过d3d11接口获取数据
        D3D11On12CreateDevice(g_pd3dDevice.Get(), 0, NULL, 0, NULL, 0, 0,
            &pd3d11Device, &pd3d11Context, NULL);

        ComPtr<ID3D11On12Device> pd3d11On12Device;
        pd3d11Device->QueryInterface(__uuidof(ID3D11On12Device), &pd3d11On12Device);

        UINT cur_idx = g_pd3dSwapChain->GetCurrentBackBufferIndex();
        ComPtr<ID3D12Resource> backbuffer;
        g_pd3dSwapChain->GetBuffer(cur_idx, __uuidof(ID3D12Resource),
            (void**)&backbuffer);
        //backbuffer->Release();

        ComPtr<ID3D11Resource> backbuffer11;
        D3D11_RESOURCE_FLAGS rf11 = {};
        pd3d11On12Device->CreateWrappedResource(
            backbuffer.Get(), &rf11,
            D3D12_RESOURCE_STATE_COPY_SOURCE,
            D3D12_RESOURCE_STATE_PRESENT, __uuidof(ID3D11Resource),
            (void**)&backbuffer11);

        pd3d11On12Device->ReleaseWrappedResources(backbuffer11.GetAddressOf(), 1);
        pd3d11On12Device->AcquireWrappedResources(backbuffer11.GetAddressOf(), 1);

        // 创建一个CPU和GPU皆能访问的texture2D资源
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = swapdesc.BufferDesc.Width;
        desc.Height = swapdesc.BufferDesc.Height;
        desc.Format = swapdesc.BufferDesc.Format;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

        ComPtr<ID3D11Texture2D> tex;
        pd3d11Device->CreateTexture2D(&desc, nullptr, &tex);
        pd3d11Context->CopyResource(tex.Get(), backbuffer11.Get());

        pd3d11On12Device->ReleaseWrappedResources(backbuffer11.GetAddressOf(), 1);
        pd3d11Context->Flush();

        ThrowIfFailed(D3DX11SaveTextureToFileW(pd3d11Context.Get(), tex.Get(), D3DX11_IMAGE_FILE_FORMAT::D3DX11_IFF_BMP, L"D:\\d3d12.bmp"));
    }
}