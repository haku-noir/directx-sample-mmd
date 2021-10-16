#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <DirectXTex.h>

#include <tchar.h>
#include <vector>
#ifdef _DEBUG
#include <iostream>
#endif // _DEBUG

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "DirectXTex.lib")

using namespace DirectX;

#define WINDOW_CLASS _T("DX12SampleMMD")
#define WINDOW_TITLE _T("DX12ƒeƒXƒg")
#define	WINDOW_STYLE WS_OVERLAPPEDWINDOW
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define ASSERT_RES(res, s) \
    do{ \
        if (res != S_OK) { \
            std::cout << "Erorr of res at " << s << std::endl; \
            return -1; \
        } \
    }while(0)
#define ASSERT_PTR(ptr, s) \
    do{ \
        if (ptr == nullptr) { \
            std::cout << "Erorr of ptr at " << s << std::endl; \
            return -1; \
        } \
    }while(0)

size_t AlignmentedSize(size_t size, size_t alignment) {
    return size + alignment - size % alignment;
}

void EnableDebugLayer() {
    ID3D12Debug* debugLayer = nullptr;
    auto res = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
    debugLayer->EnableDebugLayer();
    debugLayer->Release();
}

LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

#ifdef _DEBUG
int main() {
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#endif // _DEBUG
    auto res = CoInitializeEx(0, COINIT_MULTITHREADED);
    ASSERT_RES(res, "CoInitializeEx");

    WNDCLASSEX wndClass = {};
    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.lpfnWndProc = (WNDPROC)WindowProcedure;
    wndClass.lpszClassName = WINDOW_CLASS;
    wndClass.hInstance = GetModuleHandle(nullptr);

    RegisterClassEx(&wndClass);

    RECT wrc = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    AdjustWindowRect(&wrc, WINDOW_STYLE, false);

    HWND hwnd = CreateWindow(
        wndClass.lpszClassName,
        WINDOW_TITLE,
        WINDOW_STYLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        wrc.right - wrc.left,
        wrc.bottom - wrc.top,
        nullptr,
        nullptr,
        wndClass.hInstance,
        nullptr
    );

    IDXGIFactory6* dxgiFactory = nullptr;
#ifdef _DEBUG
    EnableDebugLayer();
    res = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgiFactory));
#else
    res = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
#endif // _DEBUG

    ID3D12Device* dev = nullptr;
    {
        std::vector<IDXGIAdapter*> adapters;
        IDXGIAdapter* tmpAdapter = nullptr;
        for (int i = 0; dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
            adapters.push_back(tmpAdapter);
        }

        for (auto adpt : adapters) {
            DXGI_ADAPTER_DESC adesc = {};
            adpt->GetDesc(&adesc);
            std::wstring strDesc = adesc.Description;

            if (strDesc.find(L"NVIDIA") != std::string::npos) {
                tmpAdapter = adpt;
                break;
            }
        }

        D3D_FEATURE_LEVEL levels[] = {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
        };

        D3D_FEATURE_LEVEL featureLevel;
        for (auto lv : levels) {
            if (D3D12CreateDevice(tmpAdapter, lv, IID_PPV_ARGS(&dev)) == S_OK) {
                featureLevel = lv;
                break;
            }
        }
        ASSERT_PTR(dev, "D3D12CreateDevice");
    }

    ID3D12CommandAllocator* cmdAllocator = nullptr;
    {
        res = dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator));
        ASSERT_RES(res, "CreateCommandAllocator");
    }

    ID3D12GraphicsCommandList* cmdList = nullptr;
    {
        res = dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator, nullptr, IID_PPV_ARGS(&cmdList));
        ASSERT_RES(res, "CreateCommandList");
    }

    ID3D12CommandQueue* cmdQueue = nullptr;
    {
        D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
        cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        cmdQueueDesc.NodeMask = 0;
        cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        res = dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&cmdQueue));
        ASSERT_RES(res, "CreateCommandQueue");
    }

    IDXGISwapChain4* swapChain = nullptr;
    {
        DXGI_SWAP_CHAIN_DESC1 swcDesc1 = {};
        swcDesc1.Width = WINDOW_WIDTH;
        swcDesc1.Height = WINDOW_HEIGHT;
        swcDesc1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swcDesc1.Stereo = false;
        swcDesc1.SampleDesc.Count = 1;
        swcDesc1.SampleDesc.Quality = 0;
        swcDesc1.BufferUsage = DXGI_USAGE_BACK_BUFFER;
        swcDesc1.BufferCount = 2;
        swcDesc1.Scaling = DXGI_SCALING_STRETCH;
        swcDesc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swcDesc1.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swcDesc1.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        res = dxgiFactory->CreateSwapChainForHwnd(cmdQueue, hwnd, &swcDesc1, nullptr, nullptr, (IDXGISwapChain1**)&swapChain);
        ASSERT_RES(res, "CreateSwapChainForHwnd");
    }

    ID3D12DescriptorHeap* rtvHeap = nullptr;
    std::vector<ID3D12Resource*> backBuffers;
    const UINT rtvHeapSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        heapDesc.NodeMask = 0;
        heapDesc.NumDescriptors = 2;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        res = dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeap));
        ASSERT_RES(res, "CreateDescriptorHeap");

        DXGI_SWAP_CHAIN_DESC swcDesc = {};
        res = swapChain->GetDesc(&swcDesc);
        ASSERT_RES(res, "GetDesc");
        backBuffers.resize(swcDesc.BufferCount);

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < swcDesc.BufferCount; ++i) {
            res = swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i]));
            ASSERT_RES(res, "GetBuffer");
            dev->CreateRenderTargetView(backBuffers[i], &rtvDesc, handle);
            handle.ptr += rtvHeapSize;
        }
    }

    ShowWindow(hwnd, SW_SHOW);

    ID3D12Fence* fence = nullptr;
    UINT fenceVal = 0;
    {
        res = dev->CreateFence(fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        ASSERT_RES(res, "CreateFence");
    }

    D3D12_RESOURCE_BARRIER barrierDesc = {};
    barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrierDesc.Transition.Subresource = 0;

    MSG msg = {};

    while (true) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (msg.message == WM_QUIT) {
            break;
        }

        auto bbIdx = swapChain->GetCurrentBackBufferIndex();

        barrierDesc.Transition.pResource = backBuffers[bbIdx];
        barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        cmdList->ResourceBarrier(1, &barrierDesc);

        auto rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += bbIdx * rtvHeapSize;
        cmdList->OMSetRenderTargets(1, &rtvHandle, true, nullptr);

        float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

        barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        cmdList->ResourceBarrier(1, &barrierDesc);

        cmdList->Close();

        ID3D12CommandList* cmdlists[] = { cmdList };
        cmdQueue->ExecuteCommandLists(1, cmdlists);

        cmdQueue->Signal(fence, ++fenceVal);
        if (fence->GetCompletedValue() != fenceVal) {
            auto event = CreateEvent(nullptr, false, false, nullptr);
            fence->SetEventOnCompletion(fenceVal, event);
            WaitForSingleObject(event, INFINITE);
            CloseHandle(event);
        }

        cmdAllocator->Reset();
        cmdList->Reset(cmdAllocator, nullptr);
        swapChain->Present(1, 0);
    }

    UnregisterClass(wndClass.lpszClassName, wndClass.hInstance);

    return 0;
}