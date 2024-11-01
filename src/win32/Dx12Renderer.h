#pragma once

// Use the C++ standard templated min/max


#include "Windows.h"

#include <stdint.h>
#include <algorithm>
#include <tuple>
#include "diagnostics.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include <DirectXColors.h>
using Microsoft::WRL::ComPtr;
#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#define AssertDx12(result) Dx12Renderer_internal::_assertDx12(result, __FILE__, __LINE__)

namespace Dx12Renderer_internal
{
    constexpr D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    constexpr UINT swapBufferCount = 3;
}

struct Dx12Renderer
{
    HWND windowHandle;
    int outputWidth;
    int outputHeight;

    ComPtr<IDXGIFactory4> factory;
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> commandQueue;
    ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
    ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;
    ComPtr<ID3D12CommandAllocator> commandAllocators[Dx12Renderer_internal::swapBufferCount];
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ComPtr<ID3D12Fence> fence;
    UINT64 fenceValues[Dx12Renderer_internal::swapBufferCount];
    Microsoft::WRL::Wrappers::Event fenceEvent;

    ComPtr<IDXGISwapChain3> swapChain;
    ComPtr<ID3D12Resource> renderTargets[Dx12Renderer_internal::swapBufferCount];
    ComPtr<ID3D12Resource> depthStencil;

    UINT backBufferIndex;
    UINT rtvDescriptorSize;

    void Initialize(HWND windowHandle, int width, int height);
    void Tick(uint64_t numberOfTicks);
    void Render();
    void Resize(int width, int height);
};

namespace Dx12Renderer_internal
{
    void _printHRESULT(HRESULT result, const char *file, int line);

    void _assertDx12(HRESULT result, const char *file, int line);

    void moveToNextFrame(Dx12Renderer* instance);

    void createDevice(Dx12Renderer* instance);

    void createResources(Dx12Renderer* instance);

    void ondDeviceLost(Dx12Renderer* instance);

    void waitForGpu(Dx12Renderer* instance);
}

void Dx12Renderer::Initialize(HWND _windowHandle, int width, int height)
{
    this->windowHandle = _windowHandle;
    this->outputWidth = std::max(width, 1);
    this->outputHeight = std::max(height, 1);
    this->backBufferIndex = 0;
    for (UINT i = 0; i < Dx12Renderer_internal::swapBufferCount; ++i)
    {
        this->fenceValues[i] = 0;
    }
    Dx12Renderer_internal::createDevice(this);
    Dx12Renderer_internal::createResources(this);
    LOG("Did initialize\n");
}

void Dx12Renderer::Resize(int width, int height)
{
    Dx12Renderer_internal::waitForGpu(this);
    this->outputWidth = std::max(width, 1);
    this->outputHeight = std::max(height, 1);
    Dx12Renderer_internal::createResources(this);    
}

void Dx12Renderer::Tick(uint64_t numberOfTicks)
{
    if (!numberOfTicks)
    {
        return;
    }
    LOG("TODO Dx12Renderer::Tick %llu\n", numberOfTicks);
}

void Dx12Renderer::Render()
{
    const UINT bufferIndex = this->backBufferIndex;
    { // CLEAR
        AssertDx12(this->commandAllocators[bufferIndex]->Reset());
        AssertDx12(this->commandList->Reset(this->commandAllocators[bufferIndex].Get(), nullptr));
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = this->renderTargets[bufferIndex].Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        this->commandList->ResourceBarrier(1, &barrier);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = this->rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += bufferIndex * this->rtvDescriptorSize;
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = this->dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        this->commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
        this->commandList->ClearRenderTargetView(rtvHandle, DirectX::Colors::MediumSeaGreen, 0, nullptr);
        this->commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        D3D12_VIEWPORT viewport = {};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = static_cast<float>(this->outputWidth);
        viewport.Height = static_cast<float>(this->outputHeight);
        viewport.MinDepth = D3D12_MIN_DEPTH;
        viewport.MaxDepth = D3D12_MAX_DEPTH;
        this->commandList->RSSetViewports(1, &viewport);

        D3D12_RECT scissorsRect = {};
        scissorsRect.left = 0;
        scissorsRect.top = 0;
        scissorsRect.right = static_cast<LONG>(this->outputWidth);
        scissorsRect.bottom = static_cast<LONG>(this->outputHeight);
        this->commandList->RSSetScissorRects(1, &scissorsRect);
    }

    { // PRESENT
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = this->renderTargets[bufferIndex].Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        this->commandList->ResourceBarrier(1, &barrier);

        AssertDx12(this->commandList->Close());
        this->commandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const *>(this->commandList.GetAddressOf()));
        HRESULT result = swapChain->Present(1, 0);
        if (result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET)
        {
            Dx12Renderer_internal::ondDeviceLost(this);
            return;
        } else  {
            AssertDx12(result);
            Dx12Renderer_internal::moveToNextFrame(this);
        }
    }
}




void Dx12Renderer_internal::moveToNextFrame(Dx12Renderer* instance)
{
    const UINT64 currentFenceValue = instance->fenceValues[instance->backBufferIndex];
    AssertDx12(instance->commandQueue->Signal(instance->fence.Get(), currentFenceValue));
    instance->backBufferIndex = instance->swapChain->GetCurrentBackBufferIndex();
    if (instance->fence->GetCompletedValue() < instance->fenceValues[instance->backBufferIndex])
    {
        AssertDx12(instance->fence->SetEventOnCompletion(instance->fenceValues[instance->backBufferIndex], instance->fenceEvent.Get()));
        std::ignore = WaitForSingleObjectEx(instance->fenceEvent.Get(), INFINITE, FALSE);
    }
    instance->fenceValues[instance->backBufferIndex] = currentFenceValue + 1;
}

void Dx12Renderer_internal::waitForGpu(Dx12Renderer* instance)
{
    if (instance->commandQueue && instance->fence && instance->fenceEvent.IsValid())
    {
        UINT64 fenceValue = instance->fenceValues[instance->backBufferIndex];
        if (SUCCEEDED(instance->commandQueue->Signal(instance->fence.Get(), fenceValue)))
        {
            if (SUCCEEDED(instance->fence->SetEventOnCompletion(fenceValue, instance->fenceEvent.Get())))
            {
                std::ignore = WaitForSingleObjectEx(instance->fenceEvent.Get(), INFINITE, FALSE);
                fenceValue += 1;
                for (UINT i = 0; i < swapBufferCount; ++i)
                {
                    instance->fenceValues[i] = fenceValue;
                }
            }
        }
    }
}


void Dx12Renderer_internal::createDevice(Dx12Renderer* instance)
{
    DWORD dxgiFactoryFlags = 0;
    #ifdef _DEBUG
    {
        ComPtr<ID3D12Debug> debugController;
        AssertDx12(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf())));
        debugController->EnableDebugLayer();
        ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
        AssertDx12(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf())));
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
        dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
    }
    #endif
    AssertDx12(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(instance->factory.ReleaseAndGetAddressOf())));
    ComPtr<IDXGIAdapter1> adapter;
    for (
        UINT adapterIndex = 0; 
        DXGI_ERROR_NOT_FOUND != instance->factory->EnumAdapters1(adapterIndex, adapter.ReleaseAndGetAddressOf()); 
        ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 adapterDesc;
        AssertDx12(adapter->GetDesc1(&adapterDesc));
        if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            continue;
        }
        
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), featureLevel, IID_PPV_ARGS(instance->device.ReleaseAndGetAddressOf()))))
        {
            LOG("Picked %ws\n", adapterDesc.Description);
            break;
        }
    }
    if (!adapter)
    {
        LOG("No Direct3D 12 device found!\n");
        exit(1);
    }

    #ifdef _DEBUG
    ComPtr<ID3D12InfoQueue> d3dInfoQueue;
    if (SUCCEEDED(instance->device.As(&d3dInfoQueue)))
    {
        d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);

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
        // d3dInfoQueue->AddStorageFilterEntries(&filter);
    }
    #endif

    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    AssertDx12(instance->device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(instance->commandQueue.ReleaseAndGetAddressOf())));

    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
    rtvDescriptorHeapDesc.NumDescriptors = swapBufferCount;
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
    dsvDescriptorHeapDesc.NumDescriptors = 1;
    dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

    AssertDx12(instance->device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(instance->rtvDescriptorHeap.ReleaseAndGetAddressOf())));
    AssertDx12(instance->device->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(instance->dsvDescriptorHeap.ReleaseAndGetAddressOf())));

    instance->rtvDescriptorSize = instance->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    for (UINT i = 0; i < swapBufferCount; ++i)
    {
        AssertDx12(instance->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(instance->commandAllocators[i].ReleaseAndGetAddressOf())));
    }
    AssertDx12(instance->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, instance->commandAllocators[0].Get(), nullptr, IID_PPV_ARGS(instance->commandList.ReleaseAndGetAddressOf())));
    AssertDx12(instance->commandList->Close());
    
    AssertDx12(instance->device->CreateFence(instance->fenceValues[instance->backBufferIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(instance->fence.ReleaseAndGetAddressOf())));

    instance->fenceEvent.Attach(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
    if (!instance->fenceEvent.IsValid())
    {
        LOG("Unable to create fence event\n");
        exit(1);
    }

    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_0 };
    if (FAILED(instance->device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel))) || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0))
    {
        LOG("Shader model 6.0 not supported!\n");
        exit(1);
    }
}

void Dx12Renderer_internal::createResources(Dx12Renderer* instance)
{
    for (UINT i = 0; i < swapBufferCount; ++i)
    {
        instance->renderTargets[i].Reset();
        instance->fenceValues[i] = instance->fenceValues[instance->backBufferIndex];
    }
    constexpr DXGI_FORMAT backBufferFormat =  DXGI_FORMAT_B8G8R8A8_UNORM;
    constexpr DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;
    const UINT backBufferWidth = static_cast<UINT>(instance->outputWidth);
    const UINT backBufferHeight = static_cast<UINT>(instance->outputHeight);

    if (instance->swapChain)
    {
        HRESULT result = instance->swapChain->ResizeBuffers(swapBufferCount, backBufferWidth, backBufferHeight, backBufferFormat, 0);
        if (result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET)
        {
            ondDeviceLost(instance);
            return;
        } else {
            AssertDx12(result);
        }
    } else {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = backBufferWidth;
        swapChainDesc.Height = backBufferHeight;
        swapChainDesc.Format = backBufferFormat;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = swapBufferCount;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = TRUE;

        ComPtr<IDXGISwapChain1> swapChain;
        AssertDx12(instance->factory->CreateSwapChainForHwnd(
            instance->commandQueue.Get(), 
            instance->windowHandle, 
            &swapChainDesc, 
            &fsSwapChainDesc, 
            nullptr, 
            swapChain.GetAddressOf()
        ));
        AssertDx12(swapChain.As(&instance->swapChain));
        AssertDx12(instance->factory->MakeWindowAssociation(instance->windowHandle, DXGI_MWA_NO_ALT_ENTER));
    }

    {
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = instance->rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < swapBufferCount; ++i)
        {
            AssertDx12(instance->swapChain->GetBuffer(i, IID_PPV_ARGS(instance->renderTargets[i].GetAddressOf())));
            #ifdef _DEBUG
            wchar_t name[25] = {};
            swprintf_s(name, L"Render target %u", i);
            instance->renderTargets[i]->SetName(name);
            #endif
            instance->device->CreateRenderTargetView(instance->renderTargets[i].Get(), nullptr, cpuHandle);
            cpuHandle.ptr += instance->rtvDescriptorSize;
        }
        instance->backBufferIndex = instance->swapChain->GetCurrentBackBufferIndex();
    }
    
    {
        D3D12_HEAP_PROPERTIES depthHeapProperties = {};
        depthHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        depthHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        depthHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        depthHeapProperties.CreationNodeMask = 1;
        depthHeapProperties.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC depthStencilDesc = {};
        depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthStencilDesc.Alignment = 0;
        depthStencilDesc.Width = backBufferWidth;
        depthStencilDesc.Height = backBufferHeight;
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
        
        AssertDx12(instance->device->CreateCommittedResource(
            &depthHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &depthStencilDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthOptimizedClearValue,
            IID_PPV_ARGS(instance->depthStencil.ReleaseAndGetAddressOf())
        ));
        #ifdef _DEBUG
        instance->depthStencil->SetName(L"Depth stencil");
        #endif
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = depthBufferFormat;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = instance->dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        instance->device->CreateDepthStencilView(instance->depthStencil.Get(), &dsvDesc, cpuHandle);
    }

}

void Dx12Renderer_internal::ondDeviceLost(Dx12Renderer* instance)
{
    waitForGpu(instance);
    for (UINT i = 0; i < swapBufferCount; ++i)
    {
        instance->commandAllocators[i].Reset();
        instance->renderTargets[i].Reset();
        instance->fenceValues[i] = 0;
    }
    instance->depthStencil.Reset();
    instance->fence.Reset();
    instance->commandList.Reset();
    instance->swapChain.Reset();
    instance->rtvDescriptorHeap.Reset();
    instance->dsvDescriptorHeap.Reset();
    instance->commandQueue.Reset();
    instance->device.Reset();
    instance->factory.Reset();
    createDevice(instance);
    createResources(instance);
}


void Dx12Renderer_internal::_printHRESULT(HRESULT result, const char *file, int line)
{
    LPSTR messageBuffer = nullptr;
    std::ignore = FormatMessageA(
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

void Dx12Renderer_internal::_assertDx12(HRESULT result, const char *file, int line)
{
    if (!SUCCEEDED(result))
    {
        _printHRESULT(result, file, line);
        exit(1);
    }
}