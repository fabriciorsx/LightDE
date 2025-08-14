#include "pch.h"
#include "Application.h"

struct Vertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT4 Color;
};

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_CREATE)
    {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreate->lpCreateParams));
        return 0;
    }
    Application* app = reinterpret_cast<Application*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (app)
        return app->MsgProc(hwnd, msg, wParam, lParam);
    else
        return DefWindowProc(hwnd, msg, wParam, lParam);
}

Application::Application(HINSTANCE hInstance) : m_hAppInst(hInstance)
{
    DirectX::XMStoreFloat4x4(&m_world, DirectX::XMMatrixIdentity());
}

Application::~Application()
{
    if (m_d3dDevice != nullptr)
        FlushCommandQueue();
}

bool Application::Initialize()
{
    if (!InitWindow()) return false;
    if (!InitDirect3D()) return false;

    OnResize();

    ThrowIfFailed(m_commandList->Reset(m_directCmdListAlloc.Get(), nullptr));
    BuildBoxGeometry();
    BuildRootSignature();
    BuildShadersAndPso();
    ThrowIfFailed(m_commandList->Close());

    ID3D12CommandList* cmdsLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
    FlushCommandQueue();

    return true;
}

int Application::Run()
{
    MSG msg = { 0 };
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Update(0.016f);
            Draw();
        }
    }
    return (int)msg.wParam;
}

void Application::Update(float dt)
{
    const float camSpeed = 10.0f * dt;
    if (GetAsyncKeyState('W') & 0x8000) m_Camera.Walk(camSpeed);
    if (GetAsyncKeyState('S') & 0x8000) m_Camera.Walk(-camSpeed);
    if (GetAsyncKeyState('A') & 0x8000) m_Camera.Strafe(-camSpeed);
    if (GetAsyncKeyState('D') & 0x8000) m_Camera.Strafe(camSpeed);
    if (GetAsyncKeyState(' ') & 0x8000) m_Camera.Jump(camSpeed);
    if (GetAsyncKeyState(0xA2) & 0x8000) m_Camera.Jump(-camSpeed);
    m_Camera.UpdateViewMatrix();
}

void Application::Draw()
{
    ThrowIfFailed(m_directCmdListAlloc->Reset());
    ThrowIfFailed(m_commandList->Reset(m_directCmdListAlloc.Get(), m_pso.Get()));

    m_commandList->RSSetViewports(1, &m_screenViewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    UINT currentBackBuffer = m_swapChain->GetCurrentBackBufferIndex();
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_swapChainBuffer[currentBackBuffer].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_commandList->ResourceBarrier(1, &barrier);

    auto rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), currentBackBuffer, m_rtvDescriptorSize);
    m_commandList->ClearRenderTargetView(rtvHandle, DirectX::Colors::CornflowerBlue, 0, nullptr);
    m_commandList->OMSetRenderTargets(1, &rtvHandle, true, nullptr);

    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    m_commandList->IASetVertexBuffers(0, 1, &m_boxVbv);
    m_commandList->IASetIndexBuffer(&m_boxIbv);
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&m_world);
    DirectX::XMMATRIX view = m_Camera.GetView();
    DirectX::XMMATRIX proj = m_Camera.GetProjection();
    DirectX::XMMATRIX worldViewProj = world * view * proj;
    worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
    m_commandList->SetGraphicsRoot32BitConstants(0, 16, &worldViewProj, 0);

    m_commandList->DrawIndexedInstanced(m_boxIndexCount, 1, 0, 0, 0);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_swapChainBuffer[currentBackBuffer].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_commandList->ResourceBarrier(1, &barrier);

    ThrowIfFailed(m_commandList->Close());
    ID3D12CommandList* cmdsLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    ThrowIfFailed(m_swapChain->Present(1, 0));
    FlushCommandQueue();
}

LRESULT Application::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
        SetCapture(m_hMainWnd);
        m_LastMousePos.x = LOWORD(lParam);
        m_LastMousePos.y = HIWORD(lParam);
        return 0;
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        ReleaseCapture();
        return 0;
    case WM_MOUSEMOVE:
        if ((wParam & MK_LBUTTON) != 0)
        {
            float dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(LOWORD(lParam) - m_LastMousePos.x));
            float dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(HIWORD(lParam) - m_LastMousePos.y));
            m_Camera.Pitch(dy);
            m_Camera.RotateY(dx);
        }
        m_LastMousePos.x = LOWORD(lParam);
        m_LastMousePos.y = HIWORD(lParam);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_SIZE:
        m_ClientWidth = LOWORD(lParam);
        m_ClientHeight = HIWORD(lParam);
        if (m_d3dDevice) OnResize();
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void Application::FlushCommandQueue()
{
    m_currentFence++;
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_currentFence));
    if (m_fence->GetCompletedValue() < m_currentFence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        if (eventHandle)
        {
            ThrowIfFailed(m_fence->SetEventOnCompletion(m_currentFence, eventHandle));
            WaitForSingleObject(eventHandle, INFINITE);
            CloseHandle(eventHandle);
        }
    }
}

void Application::OnResize()
{
    FlushCommandQueue();
    ThrowIfFailed(m_commandList->Reset(m_directCmdListAlloc.Get(), nullptr));
    for (int i = 0; i < SwapChainBufferCount; ++i)
        m_swapChainBuffer[i].Reset();

    ThrowIfFailed(m_swapChain->ResizeBuffers(SwapChainBufferCount, m_ClientWidth, m_ClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < SwapChainBufferCount; i++)
    {
        ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_swapChainBuffer[i])));
        m_d3dDevice->CreateRenderTargetView(m_swapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
        rtvHeapHandle.Offset(1, m_rtvDescriptorSize);
    }
    ThrowIfFailed(m_commandList->Close());
    ID3D12CommandList* cmdsLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
    FlushCommandQueue();

    m_screenViewport.TopLeftX = 0;
    m_screenViewport.TopLeftY = 0;
    m_screenViewport.Width = static_cast<float>(m_ClientWidth);
    m_screenViewport.Height = static_cast<float>(m_ClientHeight);
    m_screenViewport.MinDepth = 0.0f;
    m_screenViewport.MaxDepth = 1.0f;
    m_scissorRect = { 0, 0, m_ClientWidth, m_ClientHeight };
    m_Camera.SetLens(0.25f * DirectX::XM_PI, (float)m_ClientWidth / m_ClientHeight, 1.0f, 1000.0f);
}

bool Application::InitWindow()
{
    WNDCLASS wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = m_hAppInst;
    wc.hIcon = LoadIcon(0, IDI_APPLICATION);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.lpszClassName = L"MainWnd";
    if (!RegisterClass(&wc)) return false;
    m_hMainWnd = CreateWindow(L"MainWnd", L"Cubo DirectX 12", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, m_ClientWidth, m_ClientHeight, 0, 0, m_hAppInst, this);
    if (!m_hMainWnd) return false;
    ShowWindow(m_hMainWnd, SW_SHOW);
    UpdateWindow(m_hMainWnd);
    return true;
}

bool Application::InitDirect3D()
{
#if defined(DEBUG) || defined(_DEBUG)
    {
        Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
            Microsoft::WRL::ComPtr<ID3D12Debug1> debug1;
            if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debug1))))
            {
                debug1->SetEnableGPUBasedValidation(TRUE);
            }
        }
    }
#endif
ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgiFactory)));
HRESULT hardwareResult = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_d3dDevice));
if (FAILED(hardwareResult))
{
    Microsoft::WRL::ComPtr<IDXGIAdapter> pWarpAdapter;
    ThrowIfFailed(m_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));
    ThrowIfFailed(D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_d3dDevice)));
}
ThrowIfFailed(m_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
m_rtvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
CreateCommandObjects();
CreateSwapChain();
CreateRtvDescriptorHeap();
return true;
}

void Application::CreateCommandObjects()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(m_d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
    ThrowIfFailed(m_d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_directCmdListAlloc)));
    ThrowIfFailed(m_d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_directCmdListAlloc.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
    m_commandList->Close();
}

void Application::CreateSwapChain()
{
    m_swapChain.Reset();
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferDesc.Width = m_ClientWidth;
    sd.BufferDesc.Height = m_ClientHeight;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.SampleDesc.Count = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = SwapChainBufferCount;
    sd.OutputWindow = m_hMainWnd;
    sd.Windowed = true;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    Microsoft::WRL::ComPtr<IDXGISwapChain> tempSwapChain;
    ThrowIfFailed(m_dxgiFactory->CreateSwapChain(m_commandQueue.Get(), &sd, &tempSwapChain));
    ThrowIfFailed(tempSwapChain.As(&m_swapChain));
}

void Application::CreateRtvDescriptorHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(m_rtvHeap.GetAddressOf())));
}

void Application::BuildShadersAndPso()
{
    Microsoft::WRL::ComPtr<ID3DBlob> vsByteCode = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> psByteCode = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;

    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_1", compileFlags, 0, &vsByteCode, &errorBlob));
    ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_1", compileFlags, 0, &psByteCode, &errorBlob));

    m_inputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = { reinterpret_cast<BYTE*>(vsByteCode->GetBufferPointer()), vsByteCode->GetBufferSize() };
    psoDesc.PS = { reinterpret_cast<BYTE*>(psByteCode->GetBufferPointer()), psByteCode->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pso)));
}

void Application::BuildRootSignature()
{
    CD3DX12_ROOT_PARAMETER slotRootParameter[1] = {};
    slotRootParameter[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_ALL);
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, &errorBlob));
    ThrowIfFailed(m_d3dDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(m_rootSignature.GetAddressOf())));
}

void Application::BuildBoxGeometry()
{
    std::array<Vertex, 8> vertices =
    {
        Vertex({ DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::White) }),
        Vertex({ DirectX::XMFLOAT3(-1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Black) }),
        Vertex({ DirectX::XMFLOAT3(+1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Red) }),
        Vertex({ DirectX::XMFLOAT3(+1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Green) }),
        Vertex({ DirectX::XMFLOAT3(-1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Blue) }),
        Vertex({ DirectX::XMFLOAT3(-1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Yellow) }),
        Vertex({ DirectX::XMFLOAT3(+1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Cyan) }),
        Vertex({ DirectX::XMFLOAT3(+1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Magenta) })
    };
    std::array<std::uint16_t, 36> indices =
    {
        0, 1, 2,
        0, 2, 3,

        4, 6, 5,
        4, 7, 6,

        4, 5, 1,
        4, 1, 0,

        3, 2, 6,
        3, 6, 7,

        1, 5, 6,
        1, 6, 2,

        4, 0, 3,
        4, 3, 7
    };
    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    m_boxVertexBufferGPU = CreateDefaultBuffer(vertices.data(), vbByteSize, m_boxVertexBufferUploader);
    m_boxIndexBufferGPU = CreateDefaultBuffer(indices.data(), ibByteSize, m_boxIndexBufferUploader);

    m_boxVbv.BufferLocation = m_boxVertexBufferGPU->GetGPUVirtualAddress();
    m_boxVbv.StrideInBytes = sizeof(Vertex);
    m_boxVbv.SizeInBytes = vbByteSize;
    m_boxIbv.BufferLocation = m_boxIndexBufferGPU->GetGPUVirtualAddress();
    m_boxIbv.Format = DXGI_FORMAT_R16_UINT;
    m_boxIbv.SizeInBytes = ibByteSize;
    m_boxIndexCount = (UINT)indices.size();
}

Microsoft::WRL::ComPtr<ID3D12Resource> Application::CreateDefaultBuffer(const void* initData, UINT64 byteSize, Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
    Microsoft::WRL::ComPtr<ID3D12Resource> defaultBuffer;
    auto heapPropsDefault = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
    ThrowIfFailed(m_d3dDevice->CreateCommittedResource(&heapPropsDefault, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&defaultBuffer)));
    auto heapPropsUpload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    ThrowIfFailed(m_d3dDevice->CreateCommittedResource(&heapPropsUpload, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer)));
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    m_commandList->ResourceBarrier(1, &barrier);
    UpdateSubresources<1>(m_commandList.Get(), defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    m_commandList->ResourceBarrier(1, &barrier);
    return defaultBuffer;
}