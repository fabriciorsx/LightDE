#include "pch.h"
#include "Application.h"

// Estrutura de vértice para PBR
struct Vertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Normal;    // Normal para iluminação
    DirectX::XMFLOAT3 Albedo;    // Cor base/difusa
    float Metallic;              // Metalicidade (0.0 = dielétrico, 1.0 = metálico)
    float Roughness;             // Rugosidade (0.0 = espelho, 1.0 = difuso total)
    float AO;                    // Ambient Occlusion
};
struct SimpleVertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Color;
};
// Estrutura para conter os dados do modelo
struct Model {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

void Application::BuildLightCircle()
{
    const int segments = 32;
    const float radius = 2.0f;
    std::vector<SimpleVertex> vertices;

    // Centro do círculo
    vertices.push_back({ {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f} }); // Amarelo

    // Vértices do perímetro
    for (int i = 0; i <= segments; ++i)
    {
        float angle = (float)i / segments * 2.0f * DirectX::XM_PI;
        float x = radius * cosf(angle);
        float z = radius * sinf(angle);
        vertices.push_back({ {x, 0.0f, z}, {1.0f, 1.0f, 0.0f} }); // Amarelo
    }

    m_lightCircleVertexCount = (UINT)vertices.size();

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(SimpleVertex);

    m_lightCircleVertexBufferGPU = CreateDefaultBuffer(vertices.data(), vbByteSize, m_lightCircleVertexBufferUploader);

    m_lightCircleVbv.BufferLocation = m_lightCircleVertexBufferGPU->GetGPUVirtualAddress();
    m_lightCircleVbv.StrideInBytes = sizeof(SimpleVertex);
    m_lightCircleVbv.SizeInBytes = vbByteSize;
}
void Application::BuildShadersAndPso()
{
    Microsoft::WRL::ComPtr<ID3DBlob> vsByteCode = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> psByteCode = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;

    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

    // COMPILAR SHADERS PRINCIPAIS (modelo PBR)
    HRESULT hr = D3DCompileFromFile(L"pbr_shaders.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_1", compileFlags, 0, &vsByteCode, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        ThrowIfFailed(hr);
    }

    hr = D3DCompileFromFile(L"pbr_shaders.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_1", compileFlags, 0, &psByteCode, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        ThrowIfFailed(hr);
    }

    // Input Layout principal (modelo PBR)
    m_inputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "ALBEDO", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "METALLIC", 0, DXGI_FORMAT_R32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "ROUGHNESS", 0, DXGI_FORMAT_R32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "AO", 0, DXGI_FORMAT_R32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // PSO principal (modelo PBR)
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = { reinterpret_cast<BYTE*>(vsByteCode->GetBufferPointer()), vsByteCode->GetBufferSize() };
    psoDesc.PS = { reinterpret_cast<BYTE*>(psByteCode->GetBufferPointer()), psByteCode->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;

    hr = m_d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pso));
    if (FAILED(hr))
    {
        OutputDebugStringA("Falha ao criar Pipeline State Object principal\n");
        ThrowIfFailed(hr);
    }
}
// Função para carregar um modelo .obj
Model load_model_from_obj(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Nao foi possivel abrir o arquivo do modelo: " + path);
    }

    Model model;
    std::vector<DirectX::XMFLOAT3> temp_positions;
    std::vector<DirectX::XMFLOAT3> temp_normals;
    std::vector<DirectX::XMFLOAT2> temp_texcoords;
    std::map<std::tuple<int, int, int>, unsigned int> index_map;

    std::string line;
    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string prefix;
        ss >> prefix;

        if (prefix == "v") {
            DirectX::XMFLOAT3 position;
            // CORREÇÃO: Troca Y/Z e inverte Z para casar com o sistema de coordenadas do DirectX
            ss >> position.x >> position.z >> position.y;
            position.z = -position.z;
            temp_positions.push_back(position);
        }
        else if (prefix == "vn") {
            DirectX::XMFLOAT3 normal;
            // CORREÇÃO: Aplica a MESMA transformação para as normais
            ss >> normal.x >> normal.z >> normal.y;
            normal.z = -normal.z;
            temp_normals.push_back(normal);
        }
        else if (prefix == "vt") {
            DirectX::XMFLOAT2 uv;
            ss >> uv.x >> uv.y;
            temp_texcoords.push_back(uv);
        }
        else if (prefix == "f") {
            std::string vertex_data;
            std::vector<unsigned int> temp_face_indices;
            while (ss >> vertex_data) {
                std::stringstream vertex_ss(vertex_data);
                int v_idx = 0, vt_idx = 0, vn_idx = 0;
                char slash;
                vertex_ss >> v_idx >> slash >> vt_idx >> slash >> vn_idx;
                if (v_idx == 0) {
                    vertex_ss.clear();
                    vertex_ss.seekg(0);
                    vertex_ss >> v_idx;
                }
                v_idx--; vt_idx--; vn_idx--;
                std::tuple<int, int, int> key = { v_idx, vt_idx, vn_idx };
                if (index_map.count(key)) {
                    temp_face_indices.push_back(index_map[key]);
                }
                else {
                    if (v_idx < 0 || v_idx >= temp_positions.size()) continue;
                    Vertex new_vertex;
                    new_vertex.Pos = temp_positions[v_idx];
                    if (vn_idx >= 0 && vn_idx < temp_normals.size()) {
                        new_vertex.Normal = temp_normals[vn_idx];
                    }
                    else {
                        new_vertex.Normal = { 0.0f, 1.0f, 0.0f };
                    }
                    float height_factor = (new_vertex.Pos.y + 1.0f) * 0.5f;
                    new_vertex.Albedo = { 1.0f, 1.0f, 1.0f };
                    new_vertex.Metallic = 0.1f + height_factor * 0.8f;
                    new_vertex.Roughness = 0.2f + (1.0f - height_factor) * 0.6f;
                    new_vertex.AO = 0.8f + height_factor * 0.2f;
                    unsigned int new_index = static_cast<unsigned int>(model.vertices.size());
                    model.vertices.push_back(new_vertex);
                    index_map[key] = new_index;
                    temp_face_indices.push_back(new_index);
                }
            }
            for (size_t i = 0; i < temp_face_indices.size() - 2; ++i) {
                model.indices.push_back(temp_face_indices[0]);
                model.indices.push_back(temp_face_indices[i + 1]);
                model.indices.push_back(temp_face_indices[i + 2]);
            }
        }
    }
    file.close();
    return model;
}

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
    m_lightPosition = { 2000.0f, 3000.0f, 1500.0f };
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
    BuildGeometry();
    //BuildLightCircle();
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
    if (GetAsyncKeyState(VK_SPACE) & 0x8000) m_Camera.Jump(camSpeed);
    if (GetAsyncKeyState(VK_LCONTROL) & 0x8000) m_Camera.Jump(-camSpeed);
    m_Camera.UpdateViewMatrix();

    static float lightAngle = 0.0f;
    lightAngle += dt * 0.5f;
    m_lightPosition.x = 10.0f * cosf(lightAngle);
    m_lightPosition.y = 80.0f;
    m_lightPosition.z = 10.0f * sinf(lightAngle);
}

void Application::Draw()
{
    ThrowIfFailed(m_directCmdListAlloc->Reset());
    ThrowIfFailed(m_commandList->Reset(m_directCmdListAlloc.Get(), m_pso.Get()));

    m_commandList->RSSetViewports(1, &m_screenViewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    UINT currentBackBuffer = m_swapChain->GetCurrentBackBufferIndex();
    auto rtvBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_swapChainBuffer[currentBackBuffer].Get(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_commandList->ResourceBarrier(1, &rtvBarrier);

    auto rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), currentBackBuffer, m_rtvDescriptorSize);
    auto dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

    m_commandList->ClearRenderTargetView(rtvHandle, DirectX::Colors::CornflowerBlue, 0, nullptr);
    m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    m_commandList->OMSetRenderTargets(1, &rtvHandle, true, &dsvHandle);

    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    m_commandList->IASetVertexBuffers(0, 1, &m_modelVbv);
    m_commandList->IASetIndexBuffer(&m_modelIbv);
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&m_world);
    DirectX::XMMATRIX view = m_Camera.GetView();
    DirectX::XMMATRIX proj = m_Camera.GetProjection();
    DirectX::XMMATRIX worldViewProj = world * view * proj;
    worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);

    DirectX::XMFLOAT3 cameraPos = m_Camera.GetPosition3f();
    DirectX::XMFLOAT3 lightColor = { 300.0f, 300.0f, 300.0f };

    m_commandList->SetGraphicsRoot32BitConstants(0, 16, &worldViewProj, 0);
    m_commandList->SetGraphicsRoot32BitConstants(1, 4, &cameraPos, 0);
    m_commandList->SetGraphicsRoot32BitConstants(2, 4, &m_lightPosition, 0);
    m_commandList->SetGraphicsRoot32BitConstants(3, 4, &lightColor, 0);

    m_commandList->SetGraphicsRoot32BitConstants(4, 16, &world, 0);

    m_commandList->DrawIndexedInstanced(m_modelIndexCount, 1, 0, 0, 0);

    auto presentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_swapChainBuffer[currentBackBuffer].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_commandList->ResourceBarrier(1, &presentBarrier);

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
    m_depthStencilBuffer.Reset();

    ThrowIfFailed(m_swapChain->ResizeBuffers(SwapChainBufferCount, m_ClientWidth, m_ClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < SwapChainBufferCount; i++)
    {
        ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_swapChainBuffer[i])));
        m_d3dDevice->CreateRenderTargetView(m_swapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
        rtvHeapHandle.Offset(1, m_rtvDescriptorSize);
    }

    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = m_ClientWidth;
    depthStencilDesc.Height = m_ClientHeight;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

    auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(m_depthStencilBuffer.GetAddressOf())));

    m_d3dDevice->CreateDepthStencilView(m_depthStencilBuffer.Get(), nullptr, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_depthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    m_commandList->ResourceBarrier(1, &barrier);

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
    m_hMainWnd = CreateWindow(L"MainWnd", L"Modelo OBJ DirectX 12", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, m_ClientWidth, m_ClientHeight, 0, 0, m_hAppInst, this);
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
CreateDsvDescriptorHeap();
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
void Application::CreateDsvDescriptorHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(m_dsvHeap.GetAddressOf())));
}

void Application::BuildRootSignature()
{
    CD3DX12_ROOT_PARAMETER slotRootParameter[5] = {};
    slotRootParameter[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    slotRootParameter[1].InitAsConstants(4, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    slotRootParameter[2].InitAsConstants(4, 2, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    slotRootParameter[3].InitAsConstants(4, 3, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    slotRootParameter[4].InitAsConstants(16, 4, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(5, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, &errorBlob));
    ThrowIfFailed(m_d3dDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(m_rootSignature.GetAddressOf())));
}

void Application::BuildGeometry()
{
    Model model = load_model_from_obj("Models/mustang.obj");

    if (model.vertices.empty() || model.indices.empty())
    {
        throw std::runtime_error("O modelo carregado esta vazio ou em um formato nao suportado. Verifique o arquivo .obj e o parser.");
    }

    const UINT vbByteSize = (UINT)model.vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)model.indices.size() * sizeof(unsigned int);

    m_modelVertexBufferGPU = CreateDefaultBuffer(model.vertices.data(), vbByteSize, m_modelVertexBufferUploader);
    m_modelIndexBufferGPU = CreateDefaultBuffer(model.indices.data(), ibByteSize, m_modelIndexBufferUploader);

    m_modelVbv.BufferLocation = m_modelVertexBufferGPU->GetGPUVirtualAddress();
    m_modelVbv.StrideInBytes = sizeof(Vertex);
    m_modelVbv.SizeInBytes = vbByteSize;

    m_modelIbv.BufferLocation = m_modelIndexBufferGPU->GetGPUVirtualAddress();
    m_modelIbv.Format = DXGI_FORMAT_R32_UINT;
    m_modelIbv.SizeInBytes = ibByteSize;

    m_modelIndexCount = (UINT)model.indices.size();
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