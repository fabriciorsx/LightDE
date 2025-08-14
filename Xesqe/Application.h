#pragma once
#include "Camera.h"

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
    void Draw();

    bool InitWindow();
    bool InitDirect3D();

    void CreateCommandObjects();
    void CreateSwapChain();
    void CreateRtvDescriptorHeap();
    void FlushCommandQueue();

    void BuildBoxGeometry();
    void BuildRootSignature();
    void BuildShadersAndPso();

    Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(const void* initData, UINT64 byteSize, Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

    HINSTANCE m_hAppInst = nullptr;
    HWND      m_hMainWnd = nullptr;
    int m_ClientWidth = 1280;
    int m_ClientHeight = 720;

    static const int SwapChainBufferCount = 2;

    Microsoft::WRL::ComPtr<IDXGIFactory4> m_dxgiFactory;
    Microsoft::WRL::ComPtr<ID3D12Device> m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
    UINT64 m_currentFence = 0;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_directCmdListAlloc;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;

    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_swapChainBuffer[SwapChainBufferCount];

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    UINT m_rtvDescriptorSize = 0;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso;

    D3D12_VIEWPORT m_screenViewport{};
    D3D12_RECT m_scissorRect{};

    std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_boxVertexBufferGPU;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_boxIndexBufferGPU;
    D3D12_VERTEX_BUFFER_VIEW m_boxVbv{};
    D3D12_INDEX_BUFFER_VIEW m_boxIbv{};
    UINT m_boxIndexCount = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_boxVertexBufferUploader;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_boxIndexBufferUploader;

    Camera m_Camera;
    DirectX::XMFLOAT4X4 m_world{};
    POINT m_LastMousePos{};
};