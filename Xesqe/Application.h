#pragma once

#include "pch.h"
#include "Camera.h"
#include <vector>
#include <string>

struct Vertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT3 Albedo;
    float Metallic;
    float Roughness;
    float AO;
};

struct SimpleVertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Color;
};

struct Model {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

struct Physics {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 velocity;
    DirectX::XMFLOAT3 acceleration;
    DirectX::XMFLOAT4 orientation;
    DirectX::XMFLOAT3 angularVelocity;
    float angularDamping;
    float mass;
    float bounciness;
    float friction;
    bool onGround;

    Physics();
};

class Terrain {
public:
    float width, depth;
    int verticesPerRow, verticesPerCol;

    Terrain(float w = 200.0f, float d = 200.0f, int rows = 50, int cols = 50);

    void GenerateHeightMap();
    float GetHeightAt(float x, float z);
    Model GenerateTerrainMesh();

private:
    std::vector<std::vector<float>> heightMap;
    DirectX::XMFLOAT3 CalculateNormal(int i, int j);
};

class Application
{
public:
    Application(HINSTANCE hInstance);
    Application(const Application& rhs) = delete;
    Application& operator=(const Application& rhs) = delete;
    ~Application();

    bool Initialize();
    int Run();

    LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    void OnResize();
    void Update(float dt);
    void UpdatePhysics(float dt);
    void Draw();

    bool InitWindow();
    bool InitDirect3D();
    void CreateCommandObjects();
    void CreateSwapChain();
    void CreateRtvDescriptorHeap();
    void CreateDsvDescriptorHeap();

    void FlushCommandQueue();

    void BuildRootSignature();
    void BuildShadersAndPso();
    void BuildGeometry();
    void BuildTerrainGeometry();
    void BuildLightCircle();

    Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
        const void* initData,
        UINT64 byteSize,
        Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

protected:
    static const int SwapChainBufferCount = 2;

    HINSTANCE m_hAppInst = nullptr;
    HWND m_hMainWnd = nullptr;
    bool m_appPaused = false;
    bool m_minimized = false;
    bool m_maximized = false;
    bool m_resizing = false;
    bool m_fullscreenState = false;

    int m_ClientWidth = 1280;
    int m_ClientHeight = 720;

    Microsoft::WRL::ComPtr<IDXGIFactory4> m_dxgiFactory;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
    Microsoft::WRL::ComPtr<ID3D12Device> m_d3dDevice;

    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
    UINT64 m_currentFence = 0;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_directCmdListAlloc;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;

    UINT m_rtvDescriptorSize = 0;
    UINT m_dsvDescriptorSize = 0;
    UINT m_cbvSrvUavDescriptorSize = 0;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_swapChainBuffer[SwapChainBufferCount];
    Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencilBuffer;

    D3D12_VIEWPORT m_screenViewport;
    D3D12_RECT m_scissorRect;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature = nullptr;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso = nullptr;
    std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_modelVertexBufferGPU = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_modelVertexBufferUploader = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_modelIndexBufferGPU = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_modelIndexBufferUploader = nullptr;

    D3D12_VERTEX_BUFFER_VIEW m_modelVbv = {};
    D3D12_INDEX_BUFFER_VIEW m_modelIbv = {};
    UINT m_modelIndexCount = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_terrainVertexBufferGPU = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_terrainVertexBufferUploader = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_terrainIndexBufferGPU = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_terrainIndexBufferUploader = nullptr;

    D3D12_VERTEX_BUFFER_VIEW m_terrainVbv = {};
    D3D12_INDEX_BUFFER_VIEW m_terrainIbv = {};
    UINT m_terrainIndexCount = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_lightCircleVertexBufferGPU = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_lightCircleVertexBufferUploader = nullptr;
    D3D12_VERTEX_BUFFER_VIEW m_lightCircleVbv = {};
    UINT m_lightCircleVertexCount = 0;

    DirectX::XMFLOAT4X4 m_world;
    DirectX::XMFLOAT3 m_lightPosition = { 0.0f, 0.0f, 0.0f };

    Camera m_Camera;
    POINT m_LastMousePos;


    std::unique_ptr<Terrain> m_terrain;
    std::unique_ptr<Physics> m_physics;
};