#pragma once

#include <algorithm>

#include <Windows.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <DirectXColors.h>

#ifdef GPU_DEBUG
#include <dxgidebug.h>
#endif

#include "Game.h"
using Microsoft::WRL::ComPtr;

#define AssertDx12(result) Dx12Game::_assertDx12(result, __FILE__, __LINE__)

class Dx12Game final
{
public:
    void Initialize(HWND windowHandle, UINT width, UINT height, Game* game);
    void Resize(UINT width, UINT height);
    void RenderAndWaitForVSync();
    void ProcessTicks(uint64_t numberOfTicks);

private:
    static constexpr UINT SWAP_BUFFER_COUNT = 2;
    // shader model defined in shaders/compile.bat
    static constexpr D3D_SHADER_MODEL SHADER_MODEL = D3D_SHADER_MODEL_6_0;
    static constexpr D3D_FEATURE_LEVEL FEATURE_LEVEL = D3D_FEATURE_LEVEL_11_0;

    HWND                              m_outputWindowHandle;
    UINT                              m_outputWindowWidth;
    UINT                              m_outputWindowHeight;

    ComPtr<IDXGIFactory4>             m_factory;
    ComPtr<ID3D12Device2>             m_device;

    ComPtr<ID3D12GraphicsCommandList> m_directCommandList;
    ComPtr<ID3D12CommandQueue>        m_directCommandQueue;
    ComPtr<ID3D12CommandAllocator>    m_directCommandAllocators[SWAP_BUFFER_COUNT];
    ComPtr<ID3D12Fence>               m_directFence;
    UINT64                            m_directFenceValues[SWAP_BUFFER_COUNT];
    Microsoft::WRL::Wrappers::Event   m_directFenceEvent;
    ComPtr<IDXGISwapChain3>           m_swapChain;
    ComPtr<ID3D12DescriptorHeap>      m_rtvDescriptorHeap;
    UINT                              m_rtvDescriptorSize;
    ComPtr<ID3D12Resource>            m_renderTargets[SWAP_BUFFER_COUNT];
    ComPtr<ID3D12DescriptorHeap>      m_dsvDescriptorHeap;
    ComPtr<ID3D12Resource>            m_depthStencil;
    UINT                              m_backBufferIndex;

    ComPtr<ID3D12CommandQueue>        m_copyCommandQueue;
    ComPtr<ID3D12GraphicsCommandList> m_copyCommandList;
    ComPtr<ID3D12CommandAllocator>    m_copyCommandAllocator;
    ComPtr<ID3D12Fence>               m_copyFence;
    UINT64                            m_copyFenceValue;
    Microsoft::WRL::Wrappers::Event   m_copyFenceEvent;

    ComPtr<ID3D12Resource>            m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW          m_vertexBufferView;
    ComPtr<ID3D12Resource>            m_indexBuffer;
    D3D12_INDEX_BUFFER_VIEW           m_indexBufferView;
    ComPtr<ID3D12RootSignature>       m_rootSignature;
    ComPtr<ID3D12PipelineState>       m_pipelineState;

    D3D12_VIEWPORT                    m_viewport;
    D3D12_RECT                        m_scissorRect;
    float                             m_FoV;
    DirectX::XMMATRIX                 m_modelMatrix;
    DirectX::XMMATRIX                 m_viewMatrix;
    DirectX::XMMATRIX                 m_projectionMatrix;

    Game*                             m_game;

    void createDeviceAndResolutionIndependentResources();
    void createOrResizeResolutionDependentResources();
    
    ComPtr<ID3D12Resource>  copyToGPU(
        ID3D12Resource** pDestinationResource,
        size_t size,
        const void* data
    );
    void onDeviceLost();

    void moveToNextFrame();
    void waitForAllGPUOperations();
    

    static void _printHRESULT(HRESULT result, const char *file, int line);
    static void _assertDx12(HRESULT result, const char *file, int line);
};

#pragma region FakeData

struct VertexShaderInput
{
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Color;
};

static VertexShaderInput g_cubeVertices[8] = {
    { DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
    { DirectX::XMFLOAT3(-1.0f,  1.0f, -1.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
    { DirectX::XMFLOAT3(1.0f,  1.0f, -1.0f),  DirectX::XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
    { DirectX::XMFLOAT3(1.0f, -1.0f, -1.0f),  DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
    { DirectX::XMFLOAT3(-1.0f, -1.0f,  1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
    { DirectX::XMFLOAT3(-1.0f,  1.0f,  1.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
    { DirectX::XMFLOAT3(1.0f,  1.0f,  1.0f),  DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
    { DirectX::XMFLOAT3(1.0f, -1.0f,  1.0f),  DirectX::XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
};

static UINT16 g_cubeIndicies[36] =
{
    0, 1, 2, 0, 2, 3,
    4, 6, 5, 4, 7, 6,
    4, 5, 1, 4, 1, 0,
    3, 2, 6, 3, 6, 7,
    1, 5, 6, 1, 6, 2,
    4, 0, 3, 4, 3, 7
};

#pragma endregion

#pragma region Public members

void Dx12Game::Initialize(HWND windowHandle, UINT width, UINT height, Game* game)
{
    m_outputWindowHandle = windowHandle;
    m_outputWindowWidth = std::max(width, 1u);
    m_outputWindowHeight = std::max(height, 1u);
    m_game = game;

    createDeviceAndResolutionIndependentResources();
    createOrResizeResolutionDependentResources();
}

void Dx12Game::Resize(UINT width, UINT height)
{
    waitForAllGPUOperations();
    m_outputWindowWidth = std::max(width, 1u);
    m_outputWindowHeight = std::max(height, 1u);

    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;
    m_viewport.Width = static_cast<float>(m_outputWindowWidth);
    m_viewport.Height = static_cast<float>(m_outputWindowHeight);
    m_viewport.MinDepth = D3D12_MIN_DEPTH;
    m_viewport.MaxDepth = D3D12_MAX_DEPTH;

    m_scissorRect.left = 0;
    m_scissorRect.top = 0;
    m_scissorRect.right = static_cast<LONG>(width);
    m_scissorRect.bottom = static_cast<LONG>(height);

    createOrResizeResolutionDependentResources();
}

void Dx12Game::RenderAndWaitForVSync()
{
    const UINT bufferIndex = this->m_backBufferIndex;
    { // CLEAR
        AssertDx12(m_directCommandAllocators[bufferIndex]->Reset());
        AssertDx12(m_directCommandList->Reset(m_directCommandAllocators[bufferIndex].Get(), nullptr));
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = m_renderTargets[bufferIndex].Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        m_directCommandList->ResourceBarrier(1, &barrier);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += bufferIndex * m_rtvDescriptorSize;
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        m_directCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
        m_directCommandList->ClearRenderTargetView(rtvHandle, DirectX::Colors::MediumSeaGreen, 0, nullptr);
        m_directCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        m_directCommandList->RSSetViewports(1, &m_viewport);
        m_directCommandList->RSSetScissorRects(1, &m_scissorRect);
    }
    // TODO: LOGIC HERE!

    { // PRESENT
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = m_renderTargets[bufferIndex].Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        m_directCommandList->ResourceBarrier(1, &barrier);

        AssertDx12(m_directCommandList->Close());
        m_directCommandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const *>(m_directCommandList.GetAddressOf()));
        HRESULT result = m_swapChain->Present(1, 0);
        if (result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET)
        {
            onDeviceLost();
        } else {
            AssertDx12(result);
            moveToNextFrame();
        }
    }
}

void Dx12Game::ProcessTicks(uint64_t numberOfTicks)
{
    m_game->ProcessTicks(numberOfTicks);
}

#pragma endregion

#pragma region Resource creation

void Dx12Game::createDeviceAndResolutionIndependentResources()
{
    { // Factory
        DWORD createFactoryFlags = 0;
        #ifdef GPU_DEBUG
        {
            createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
            ComPtr<ID3D12Debug> debugController;
            AssertDx12(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf())));
            debugController->EnableDebugLayer();
            ComPtr<IDXGIInfoQueue> infoQueue;
            AssertDx12(DXGIGetDebugInterface1(0, IID_PPV_ARGS(infoQueue.GetAddressOf())));
            infoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, true);
            infoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
            infoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
        }
        #endif
        AssertDx12(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(m_factory.ReleaseAndGetAddressOf())));
    }
    { // Device
        ComPtr<ID3D12Device> device;
        ComPtr<IDXGIAdapter1> adapter;
        DXGI_ADAPTER_DESC1 adapterDesc;
        for (UINT adapterIndex = 0; 
            DXGI_ERROR_NOT_FOUND != m_factory->EnumAdapters1(adapterIndex, adapter.ReleaseAndGetAddressOf());
            ++adapterIndex)
        {
            AssertDx12(adapter->GetDesc1(&adapterDesc));
            if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                continue;
            }
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), FEATURE_LEVEL, IID_PPV_ARGS(device.ReleaseAndGetAddressOf()))))
            {
                LOG("Picked %ws\n", adapterDesc.Description);
                break;
            }
        }
        if (!device)
        {
            LOG("No Direct3D 12 device found!\n");
            exit(1);
        }
        #ifdef GPU_DEBUG
        {
            ComPtr<ID3D12InfoQueue> infoQueue;
            if (SUCCEEDED(device.As(&infoQueue)))
            {
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
                // D3D12_MESSAGE_ID hide[] =
                // {
                //     D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                //     D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
                //     // Workarounds for debug layer issues on hybrid-graphics systems
                //     D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
                //     D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
                // };
                // D3D12_INFO_QUEUE_FILTER filter = {};
                // filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
                // filter.DenyList.pIDList = hide;
                // infoQueue->AddStorageFilterEntries(&filter);
            }
        }
        #endif
        AssertDx12(device.As(&m_device));
    }
    { // Direct Command Queue, Allocators, List & Fences
        D3D12_COMMAND_QUEUE_DESC directCommandQueueDesc = {};
        directCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        directCommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        directCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        
        AssertDx12(m_device->CreateCommandQueue(
            &directCommandQueueDesc, 
            IID_PPV_ARGS(m_directCommandQueue.ReleaseAndGetAddressOf())
        ));

        for (UINT bufferIdx = 0; bufferIdx < SWAP_BUFFER_COUNT; ++bufferIdx)
        {
            AssertDx12(m_device->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT, 
                IID_PPV_ARGS(m_directCommandAllocators[bufferIdx].ReleaseAndGetAddressOf())
            ));
        }

        m_backBufferIndex = 0;

        AssertDx12(m_device->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            m_directCommandAllocators[m_backBufferIndex].Get(),
            nullptr,
            IID_PPV_ARGS(m_directCommandList.ReleaseAndGetAddressOf())
        ));
        // TODO: Czy potrzebuje wywołać close?
        AssertDx12(m_directCommandList->Close());

        for (UINT bufferIdx = 0; bufferIdx < SWAP_BUFFER_COUNT; ++bufferIdx)
        {
            m_directFenceValues[bufferIdx] = 0;
        }
        AssertDx12(m_device->CreateFence(
            m_directFenceValues[m_backBufferIndex],
            D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(m_directFence.ReleaseAndGetAddressOf())
        ));

        m_directFenceEvent.Attach(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
        if (!m_directFenceEvent.IsValid())
        {
            LOG("Unable to create direct fence event\n");
            exit(1);
        }
    }
    { // RTV Descriptor Heap & Allocators
        D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
        rtvDescriptorHeapDesc.NumDescriptors = SWAP_BUFFER_COUNT;
        rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        AssertDx12(m_device->CreateDescriptorHeap(
            &rtvDescriptorHeapDesc, 
            IID_PPV_ARGS(m_rtvDescriptorHeap.ReleaseAndGetAddressOf())
        ));
        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }
    { // DSV Descriptor heap
        D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
        dsvDescriptorHeapDesc.NumDescriptors = 1;
        dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        AssertDx12(m_device->CreateDescriptorHeap(
            &dsvDescriptorHeapDesc,
            IID_PPV_ARGS(m_dsvDescriptorHeap.ReleaseAndGetAddressOf())
        ));
    }
    { // Copy Command Queue, Allocator, List & Fence
        D3D12_COMMAND_QUEUE_DESC copyCommandQueueDesc = {};
        copyCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
        copyCommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        copyCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

        AssertDx12(m_device->CreateCommandQueue(
            &copyCommandQueueDesc,
            IID_PPV_ARGS(m_copyCommandQueue.ReleaseAndGetAddressOf())
        ));

        AssertDx12(m_device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_COPY,
            IID_PPV_ARGS(m_copyCommandAllocator.ReleaseAndGetAddressOf())
        ));

        AssertDx12(m_device->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_COPY,
            m_copyCommandAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(m_copyCommandList.ReleaseAndGetAddressOf())
        ));

        m_copyFenceValue = 0;
        AssertDx12(m_device->CreateFence(
            m_copyFenceValue,
            D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(m_copyFence.ReleaseAndGetAddressOf())
        ));
        m_copyFenceEvent.Attach(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
        if (!m_copyFenceEvent.IsValid())
        {
            LOG("Unable to create copy fence event\n");
            exit(1);
        }
    }
    { // Ensure shader model & DxMath support
        D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { SHADER_MODEL };
        if (
            FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel))) ||
            (shaderModel.HighestShaderModel < SHADER_MODEL)
        )
        {
            LOG("Missing support for required shader model!\n");
            exit(1);
        }
        if (!DirectX::XMVerifyCPUSupport())
        {
            LOG("No support for directX math!\n");
            exit(1);
        }
    }
    // Load static content
    { // Load cube vertices
        constexpr size_t cubeVerticesBufferSize = sizeof(g_cubeVertices);
        ComPtr<ID3D12Resource> cubeVerticesIntermediateBuffer = copyToGPU(
            m_vertexBuffer.ReleaseAndGetAddressOf(),
            cubeVerticesBufferSize,
            g_cubeVertices
        );
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.SizeInBytes = cubeVerticesBufferSize;
        m_vertexBufferView.StrideInBytes = sizeof(VertexShaderInput);
        
        constexpr size_t cubeIndiciesBufferSize = sizeof(g_cubeIndicies);
        ComPtr<ID3D12Resource> cubeIndiciesIntermediateBuffer = copyToGPU(
            m_indexBuffer.ReleaseAndGetAddressOf(),
            cubeIndiciesBufferSize,
            g_cubeIndicies
        );
        m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        m_indexBufferView.SizeInBytes = cubeIndiciesBufferSize;
        m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;

        ComPtr<ID3DBlob> vertexShaderBlob;
        AssertDx12(D3DReadFileToBlob(L"shaders\\VertexShader.cso", vertexShaderBlob.ReleaseAndGetAddressOf()));

        ComPtr<ID3DBlob> pixelShaderBlob;
        AssertDx12(D3DReadFileToBlob(L"shaders\\PixelShader.cso", pixelShaderBlob.ReleaseAndGetAddressOf()));

        D3D12_INPUT_ELEMENT_DESC inputLayout[2];
        inputLayout[0].SemanticName = "POSITION";
        inputLayout[0].SemanticIndex = 0;
        inputLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
        inputLayout[0].InputSlot = 0;
        inputLayout[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
        inputLayout[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        inputLayout[0].InstanceDataStepRate = 0;

        inputLayout[1].SemanticName = "COLOR";
        inputLayout[1].SemanticIndex = 0;
        inputLayout[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
        inputLayout[1].InputSlot = 0;
        inputLayout[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
        inputLayout[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        inputLayout[1].InstanceDataStepRate = 0;

        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
        
        ComPtr<ID3DBlob> rootSignatureBlob;
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
        if (SUCCEEDED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            D3D12_ROOT_PARAMETER1 rootParameters[1];
            rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
            rootParameters[0].Constants.Num32BitValues = sizeof(DirectX::XMMATRIX) / 4;
            rootParameters[0].Constants.ShaderRegister = 0;
            rootParameters[0].Constants.RegisterSpace = 0;

            D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
            rootSignatureDescription.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
            rootSignatureDescription.Desc_1_1.NumParameters = _countof(rootParameters);
            rootSignatureDescription.Desc_1_1.pParameters = rootParameters;
            rootSignatureDescription.Desc_1_1.NumStaticSamplers = 0;
            rootSignatureDescription.Desc_1_1.pStaticSamplers = nullptr;
            rootSignatureDescription.Desc_1_1.Flags = rootSignatureFlags;

            AssertDx12(D3D12SerializeVersionedRootSignature(
                &rootSignatureDescription,
                rootSignatureBlob.ReleaseAndGetAddressOf(),
                NULL
            ));
        } else {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
            D3D12_ROOT_PARAMETER rootParameters[1];
            rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
            rootParameters[0].Constants.Num32BitValues = sizeof(DirectX::XMMATRIX) / 4;
            rootParameters[0].Constants.ShaderRegister = 0;
            rootParameters[0].Constants.RegisterSpace = 0;

            D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
            rootSignatureDescription.Version = D3D_ROOT_SIGNATURE_VERSION_1_0;
            rootSignatureDescription.Desc_1_0.NumParameters = _countof(rootParameters);
            rootSignatureDescription.Desc_1_0.pParameters = rootParameters;
            rootSignatureDescription.Desc_1_0.NumStaticSamplers = 0;
            rootSignatureDescription.Desc_1_0.pStaticSamplers = nullptr;
            rootSignatureDescription.Desc_1_0.Flags = rootSignatureFlags;

            AssertDx12(D3D12SerializeRootSignature(
                &rootSignatureDescription.Desc_1_0, 
                D3D_ROOT_SIGNATURE_VERSION_1_0, 
                rootSignatureBlob.ReleaseAndGetAddressOf(),
                NULL
            ));
        }

        AssertDx12(m_device->CreateRootSignature(
            0,
            rootSignatureBlob->GetBufferPointer(),
            rootSignatureBlob->GetBufferSize(),
            IID_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())
        ));
        
        struct alignas(void*) PipelineStateStream
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE pRootSignatureType; // D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE
            ID3D12RootSignature* pRootSignature;

            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE InputLayoutType; // D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT
            D3D12_INPUT_LAYOUT_DESC InputLayout;

            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE PrimitiveTopologyTypeType; // D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY
            D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;

            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE VS_Type; // D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS
            D3D12_SHADER_BYTECODE VS;

            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE PS_Type; // D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS
            D3D12_SHADER_BYTECODE PS;

            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE DSVFormat_Type; // D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT
            DXGI_FORMAT DSVFormat;

            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE RTVFormats_Type; // D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS
            D3D12_RT_FORMAT_ARRAY RTVFormats;
        } pipelineStateStream;

        pipelineStateStream.pRootSignatureType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
        pipelineStateStream.pRootSignature = m_rootSignature.Get();

        pipelineStateStream.InputLayoutType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT;
        pipelineStateStream.InputLayout.pInputElementDescs = inputLayout;
        pipelineStateStream.InputLayout.NumElements = _countof(inputLayout);

        pipelineStateStream.PrimitiveTopologyTypeType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY;
        pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        pipelineStateStream.VS_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS;
        pipelineStateStream.VS.pShaderBytecode = vertexShaderBlob->GetBufferPointer();
        pipelineStateStream.VS.BytecodeLength = vertexShaderBlob->GetBufferSize();

        pipelineStateStream.PS_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS;
        pipelineStateStream.PS.pShaderBytecode = pixelShaderBlob->GetBufferPointer();
        pipelineStateStream.PS.BytecodeLength = pixelShaderBlob->GetBufferSize();

        pipelineStateStream.DSVFormat_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT;
        pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;

        pipelineStateStream.RTVFormats_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS;
        D3D12_RT_FORMAT_ARRAY rtvFormats = {};
        rtvFormats.NumRenderTargets = 1;
        rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        pipelineStateStream.RTVFormats = rtvFormats;

        D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {};
        pipelineStateStreamDesc.SizeInBytes = sizeof(PipelineStateStream);
        pipelineStateStreamDesc.pPipelineStateSubobjectStream = &pipelineStateStream;

        AssertDx12(m_device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf())));
        m_copyFenceValue++;
        m_copyCommandQueue->Signal(m_copyFence.Get(), m_copyFenceValue);

        if (m_copyFence->GetCompletedValue() < m_copyFenceValue) 
        {
            m_copyFence->SetEventOnCompletion(m_copyFenceValue, m_copyFenceEvent.Get());
            ::WaitForSingleObject(m_copyFenceEvent.Get(), INFINITE);
        }
    }
}

ComPtr<ID3D12Resource>  Dx12Game::copyToGPU(
    ID3D12Resource** pDestinationBuffer,
    size_t bufferSize,
    const void* data)
{
    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = bufferSize;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    AssertDx12(m_device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(pDestinationBuffer)
    ));


    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

    ComPtr<ID3D12Resource> intermidiateBuffer;
    AssertDx12(m_device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(intermidiateBuffer.ReleaseAndGetAddressOf())
    ));
    D3D12_SUBRESOURCE_DATA subresourceData = {};
    subresourceData.pData = data;
    subresourceData.RowPitch = bufferSize;
    subresourceData.SlicePitch = subresourceData.RowPitch;

    UINT64 requiredSize = 0;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT resourceLayout;
    UINT64 rowSizeInBytes;
    UINT numRows;

    m_device->GetCopyableFootprints(
        &resourceDesc, 0, 1, 0, &resourceLayout, &numRows, &rowSizeInBytes, &requiredSize
    );

    BYTE* pData;
    AssertDx12(intermidiateBuffer->Map(0, NULL, reinterpret_cast<void **>(&pData)));
    D3D12_MEMCPY_DEST destData;
    destData.pData = pData + resourceLayout.Offset;
    destData.RowPitch = resourceLayout.Footprint.RowPitch;
    destData.SlicePitch = resourceLayout.Footprint.RowPitch * numRows;

    for (UINT z = 0; z < resourceLayout.Footprint.Depth; ++z)
    {
        BYTE* pDestSlice = reinterpret_cast<BYTE*>(destData.pData) + destData.SlicePitch * z;
        const BYTE* pSrcSlice = reinterpret_cast<const BYTE*>(subresourceData.pData) + subresourceData.SlicePitch * z;
        for (UINT y = 0; y < numRows; ++y)
        {
            memcpy(
                pDestSlice + destData.RowPitch * y,
                pSrcSlice + subresourceData.RowPitch * y,
                rowSizeInBytes
            );
        }
    }
    intermidiateBuffer->Unmap(0, NULL);
    m_copyCommandList->CopyBufferRegion(
        *pDestinationBuffer, 0, 
        intermidiateBuffer.Get(), resourceLayout.Offset, 
        resourceLayout.Footprint.Width
    );
    return intermidiateBuffer;
}

void Dx12Game::createOrResizeResolutionDependentResources()
{
    for (UINT i = 0; i < SWAP_BUFFER_COUNT; ++i)
    {
        m_renderTargets[i].Reset();
        m_directFenceValues[i] = m_directFenceValues[m_backBufferIndex];
    }

    // TODO: make sure that rtv has format DXGI_FORMAT_R8G8B8A8_UNORM
    constexpr DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    constexpr DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

    if (m_swapChain) // Swap Chain
    {
        HRESULT result = m_swapChain->ResizeBuffers(SWAP_BUFFER_COUNT, m_outputWindowWidth, m_outputWindowHeight, backBufferFormat, 0);

        if (result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_REMOVED)
        {
            onDeviceLost();
            return;
        } else {
            AssertDx12(result);
        }
    } else {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = m_outputWindowWidth;
        swapChainDesc.Height = m_outputWindowHeight;
        swapChainDesc.Format = backBufferFormat;
        swapChainDesc.Stereo = 0;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = SWAP_BUFFER_COUNT;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapChainDesc.Flags = 0;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = TRUE;

        ComPtr<IDXGISwapChain1> swapChain;
        AssertDx12(m_factory->CreateSwapChainForHwnd(
            m_directCommandQueue.Get(),
            m_outputWindowHandle,
            &swapChainDesc,
            &fsSwapChainDesc,
            nullptr,
            swapChain.GetAddressOf()
        ));
        AssertDx12(swapChain.As(&m_swapChain));
        AssertDx12(m_factory->MakeWindowAssociation(m_outputWindowHandle, DXGI_MWA_NO_ALT_ENTER));
    }
    
    { // RTV
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < SWAP_BUFFER_COUNT; ++i)
        {
            AssertDx12(m_swapChain->GetBuffer(i, IID_PPV_ARGS(m_renderTargets[i].GetAddressOf())));
            #ifdef GPU_DEBUG
            wchar_t name[25] = {};
            swprintf_s(name, L"Render target %u", i);
            m_renderTargets[i]->SetName(name);
            #endif
            m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, cpuHandle);
            cpuHandle.ptr += m_rtvDescriptorSize;
        }
        m_backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
    }

    { // DSV
        D3D12_HEAP_PROPERTIES depthHeapProperties = {};
        depthHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        depthHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        depthHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        depthHeapProperties.CreationNodeMask = 1;
        depthHeapProperties.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC depthStencilDesc = {};
        depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthStencilDesc.Alignment = 0;
        depthStencilDesc.Width = m_outputWindowWidth;
        depthStencilDesc.Height = m_outputWindowHeight;
        depthStencilDesc.DepthOrArraySize = 1;
        depthStencilDesc.MipLevels = 1;
        depthStencilDesc.Format = depthBufferFormat;
        depthStencilDesc.SampleDesc.Count = 1;
        depthStencilDesc.SampleDesc.Quality = 0;
        depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
        depthOptimizedClearValue.Format = depthBufferFormat;
        depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
        depthOptimizedClearValue.DepthStencil.Stencil = 0u;

        AssertDx12(m_device->CreateCommittedResource(
            &depthHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &depthStencilDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthOptimizedClearValue,
            IID_PPV_ARGS(m_depthStencil.ReleaseAndGetAddressOf())
        ));
        #ifdef GPU_DEBUG
        m_depthStencil->SetName(L"Depth stencil");
        #endif
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = depthBufferFormat;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        m_device->CreateDepthStencilView(m_depthStencil.Get(), &dsvDesc, cpuHandle);
    }
}

void Dx12Game::onDeviceLost()
{
    // nawet nie wiem
    LOG("TODO Dx12Game::onDeviceLost\n");
}

#pragma endregion

#pragma region Synchronization

void Dx12Game::moveToNextFrame()
{
    const UINT64 currentFenceValue = ++m_directFenceValues[m_backBufferIndex];
    AssertDx12(m_directCommandQueue->Signal(m_directFence.Get(), currentFenceValue));
    m_backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
    if (m_directFence->GetCompletedValue() < m_directFenceValues[m_backBufferIndex])
    {
        AssertDx12(m_directFence->SetEventOnCompletion(m_directFenceValues[m_backBufferIndex], m_directFenceEvent.Get()));
        ::WaitForSingleObject(m_directFenceEvent.Get(), INFINITE);
    }
    m_directFenceValues[m_backBufferIndex] = currentFenceValue;
}

void Dx12Game::waitForAllGPUOperations()
{
    if (m_copyFence->GetCompletedValue() < m_copyFenceValue)
    {
        m_copyFenceValue++;
        m_copyCommandQueue->Signal(m_copyFence.Get(), m_copyFenceValue);
        m_copyFence->SetEventOnCompletion(m_copyFenceValue, m_copyFenceEvent.Get());
        ::WaitForSingleObject(m_copyFenceEvent.Get(), INFINITE);
    }

    if (m_directFence->GetCompletedValue() < m_directFenceValues[m_backBufferIndex])
    {
        m_directFenceValues[m_backBufferIndex]++;
        m_directCommandQueue->Signal(m_directFence.Get(), m_directFenceValues[m_backBufferIndex]);
        m_directFence->SetEventOnCompletion(m_directFenceValues[m_backBufferIndex], m_directFenceEvent.Get());
        ::WaitForSingleObject(m_directFenceEvent.Get(), INFINITE);
    }
}

#pragma endregion

#pragma region Logging

void Dx12Game::_printHRESULT(HRESULT result, const char *file, int line)
{
    LPSTR messageBuffer = nullptr;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        result,
        0,
        (LPSTR)&messageBuffer,
        0,
        nullptr
    );
    

    if (messageBuffer)
    {
        LOG_PATH(file, line, "HRESULT 0x%08X %s\n", result, messageBuffer);
        LocalFree(messageBuffer);
    } else {
        LOG_PATH(file, line, "HRESULT 0x%08X\n", result);
    }
}

void Dx12Game::_assertDx12(HRESULT result, const char *file, int line)
{
    if (!SUCCEEDED(result))
    {
        _printHRESULT(result, file, line);
        exit(1);
    }
}

#pragma endregion