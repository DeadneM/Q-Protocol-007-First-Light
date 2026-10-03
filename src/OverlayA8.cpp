#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <d3d12.h>
#include <dxgi1_4.h>

#include "Overlay.h"

#include "MinHook.h"
#include "imgui.h"
#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_win32.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam);

namespace qp {
namespace {

using PresentFn = HRESULT(__stdcall*)(
    IDXGISwapChain*,
    UINT,
    UINT);

using ResizeBuffersFn = HRESULT(__stdcall*)(
    IDXGISwapChain*,
    UINT,
    UINT,
    UINT,
    DXGI_FORMAT,
    UINT);

using FactoryCreateSwapChainFn = HRESULT(__stdcall*)(
    IDXGIFactory*,
    IUnknown*,
    DXGI_SWAP_CHAIN_DESC*,
    IDXGISwapChain**);

using FactoryCreateSwapChainForHwndFn = HRESULT(__stdcall*)(
    IDXGIFactory2*,
    IUnknown*,
    HWND,
    const DXGI_SWAP_CHAIN_DESC1*,
    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*,
    IDXGIOutput*,
    IDXGISwapChain1**);

using FactoryCreateSwapChainForCoreWindowFn = HRESULT(__stdcall*)(
    IDXGIFactory2*,
    IUnknown*,
    IUnknown*,
    const DXGI_SWAP_CHAIN_DESC1*,
    IDXGIOutput*,
    IDXGISwapChain1**);

using FactoryCreateSwapChainForCompositionFn = HRESULT(__stdcall*)(
    IDXGIFactory2*,
    IUnknown*,
    const DXGI_SWAP_CHAIN_DESC1*,
    IDXGIOutput*,
    IDXGISwapChain1**);

struct FrameContext {
    ID3D12CommandAllocator* allocator = nullptr;
    ID3D12Resource* renderTarget = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv{};
    UINT64 fenceValue = 0;
};

enum class OverlayStage : int {
    Bootstrap = 0,
    FactoryHooksReady,
    SwapChainCaptured,
    ImGuiReady,
    InitFailed,
    FenceTimeout
};

std::wstring g_iniPath;

std::atomic<bool> g_requestedVisible{false};
std::atomic<bool> g_overlayReady{false};
std::atomic<bool> g_reloadRequested{false};
std::atomic<bool> g_playerReady{false};
std::atomic<bool> g_autoDone{false};
std::atomic<std::size_t> g_queueCount{0};
std::atomic<bool> g_qpistolNextB{true};
std::atomic<int> g_stage{static_cast<int>(OverlayStage::Bootstrap)};

bool g_insertWasDown = false;
bool g_minHookInitialized = false;
bool g_factoryMethodsHooked = false;
bool g_swapChainMethodsHooked = false;

FactoryCreateSwapChainFn g_originalCreateSwapChain = nullptr;
FactoryCreateSwapChainForHwndFn g_originalCreateSwapChainForHwnd = nullptr;
FactoryCreateSwapChainForCoreWindowFn g_originalCreateSwapChainForCoreWindow = nullptr;
FactoryCreateSwapChainForCompositionFn g_originalCreateSwapChainForComposition = nullptr;
PresentFn g_originalPresent = nullptr;
ResizeBuffersFn g_originalResizeBuffers = nullptr;

ID3D12CommandQueue* g_commandQueue = nullptr;
ID3D12Device* g_device = nullptr;
IDXGISwapChain3* g_swapChain3 = nullptr;
ID3D12DescriptorHeap* g_rtvHeap = nullptr;
ID3D12DescriptorHeap* g_srvHeap = nullptr;
ID3D12GraphicsCommandList* g_commandList = nullptr;
ID3D12Fence* g_fence = nullptr;
HANDLE g_fenceEvent = nullptr;
UINT64 g_nextFenceValue = 0;
UINT g_rtvDescriptorSize = 0;
DXGI_FORMAT g_backBufferFormat = DXGI_FORMAT_UNKNOWN;
std::vector<FrameContext> g_frames;

HWND g_gameWindow = nullptr;
WNDPROC g_originalWndProc = nullptr;

bool g_imguiContextCreated = false;
bool g_imguiWin32Initialized = false;
bool g_imguiDx12Initialized = false;

bool IsInputMessage(UINT msg) {
    switch (msg) {
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_CHAR:
        return true;
    default:
        return false;
    }
}

LRESULT CALLBACK OverlayWndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam) {

    if (g_imguiWin32Initialized) {
        ImGui_ImplWin32_WndProcHandler(
            hwnd,
            msg,
            wParam,
            lParam);

        if (g_overlayReady.load(std::memory_order_acquire) &&
            g_requestedVisible.load(std::memory_order_acquire) &&
            IsInputMessage(msg)) {
            return 1;
        }
    }

    if (g_originalWndProc) {
        return CallWindowProcW(
            g_originalWndProc,
            hwnd,
            msg,
            wParam,
            lParam);
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool WaitForFenceValue(
    UINT64 value,
    DWORD timeoutMs) {

    if (!value ||
        !g_fence ||
        !g_fenceEvent) {
        return true;
    }

    if (g_fence->GetCompletedValue() >= value) {
        return true;
    }

    if (FAILED(
            g_fence->SetEventOnCompletion(
                value,
                g_fenceEvent))) {
        return false;
    }

    return WaitForSingleObject(
               g_fenceEvent,
               timeoutMs) == WAIT_OBJECT_0;
}

bool WaitForAllFrames(DWORD timeoutMs) {
    UINT64 last = 0;
    for (const auto& frame : g_frames) {
        last = (std::max)(last, frame.fenceValue);
    }
    return WaitForFenceValue(last, timeoutMs);
}

void ReleaseDx12Resources() {
    for (auto& frame : g_frames) {
        if (frame.renderTarget) {
            frame.renderTarget->Release();
            frame.renderTarget = nullptr;
        }

        if (frame.allocator) {
            frame.allocator->Release();
            frame.allocator = nullptr;
        }

        frame.fenceValue = 0;
    }

    g_frames.clear();

    if (g_commandList) {
        g_commandList->Release();
        g_commandList = nullptr;
    }

    if (g_fence) {
        g_fence->Release();
        g_fence = nullptr;
    }

    if (g_fenceEvent) {
        CloseHandle(g_fenceEvent);
        g_fenceEvent = nullptr;
    }

    if (g_rtvHeap) {
        g_rtvHeap->Release();
        g_rtvHeap = nullptr;
    }

    if (g_srvHeap) {
        g_srvHeap->Release();
        g_srvHeap = nullptr;
    }

    if (g_swapChain3) {
        g_swapChain3->Release();
        g_swapChain3 = nullptr;
    }

    if (g_device) {
        g_device->Release();
        g_device = nullptr;
    }

    g_nextFenceValue = 0;
    g_rtvDescriptorSize = 0;
    g_backBufferFormat = DXGI_FORMAT_UNKNOWN;
}

void ShutdownDx12Backend() {
    g_overlayReady.store(
        false,
        std::memory_order_release);

    WaitForAllFrames(2000);

    if (g_imguiDx12Initialized) {
        ImGui_ImplDX12_Shutdown();
        g_imguiDx12Initialized = false;
    }

    ReleaseDx12Resources();
}

void ShutdownImGui() {
    ShutdownDx12Backend();

    if (g_imguiWin32Initialized) {
        ImGui_ImplWin32_Shutdown();
        g_imguiWin32Initialized = false;
    }

    if (g_gameWindow &&
        g_originalWndProc) {
        SetWindowLongPtrW(
            g_gameWindow,
            GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(
                g_originalWndProc));
    }

    g_originalWndProc = nullptr;
    g_gameWindow = nullptr;

    if (g_imguiContextCreated) {
        ImGui::DestroyContext();
        g_imguiContextCreated = false;
    }
}

void ApplyStyle() {
    ImGuiStyle& s = ImGui::GetStyle();

    s.WindowPadding = ImVec2(22.0f, 20.0f);
    s.FramePadding = ImVec2(11.0f, 8.0f);
    s.ItemSpacing = ImVec2(10.0f, 10.0f);
    s.WindowRounding = 9.0f;
    s.FrameRounding = 5.0f;
    s.ChildRounding = 7.0f;
    s.TabRounding = 5.0f;
    s.WindowBorderSize = 1.0f;

    ImVec4* c = s.Colors;
    c[ImGuiCol_Text] = ImVec4(0.94f, 0.96f, 0.99f, 1.00f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.53f, 0.58f, 0.66f, 1.00f);
    c[ImGuiCol_WindowBg] = ImVec4(0.035f, 0.043f, 0.058f, 0.97f);
    c[ImGuiCol_ChildBg] = ImVec4(0.055f, 0.067f, 0.090f, 0.96f);
    c[ImGuiCol_Border] = ImVec4(0.16f, 0.26f, 0.39f, 0.95f);
    c[ImGuiCol_FrameBg] = ImVec4(0.075f, 0.095f, 0.130f, 1.00f);
    c[ImGuiCol_Button] = ImVec4(0.08f, 0.30f, 0.56f, 1.00f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.10f, 0.42f, 0.76f, 1.00f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.08f, 0.34f, 0.64f, 1.00f);
    c[ImGuiCol_Header] = ImVec4(0.08f, 0.30f, 0.56f, 0.72f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.10f, 0.42f, 0.76f, 0.92f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.08f, 0.34f, 0.64f, 1.00f);
    c[ImGuiCol_Tab] = ImVec4(0.060f, 0.075f, 0.105f, 1.00f);
    c[ImGuiCol_TabSelected] = ImVec4(0.08f, 0.30f, 0.56f, 1.00f);
    c[ImGuiCol_TabHovered] = ImVec4(0.10f, 0.42f, 0.76f, 1.00f);
}

void DrawStatusPill(
    const char* text,
    bool good) {

    const ImVec4 color =
        good
            ? ImVec4(0.25f, 0.85f, 0.48f, 1.0f)
            : ImVec4(0.95f, 0.64f, 0.27f, 1.0f);

    ImGui::TextColored(
        color,
        "%s",
        text);
}

void DrawOverlayWindow() {
    ImGuiIO& io = ImGui::GetIO();

    const ImVec2 size(
        (std::min)(720.0f, io.DisplaySize.x - 48.0f),
        (std::min)(430.0f, io.DisplaySize.y - 48.0f));

    const ImVec2 pos(
        (io.DisplaySize.x - size.x) * 0.5f,
        (io.DisplaySize.y - size.y) * 0.5f);

    ImGui::SetNextWindowPos(
        pos,
        ImGuiCond_Always);

    ImGui::SetNextWindowSize(
        size,
        ImGuiCond_Always);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin(
            "Q Protocol A8 renderer test",
            nullptr,
            flags)) {
        ImGui::End();
        return;
    }

    ImGui::TextColored(
        ImVec4(0.30f, 0.70f, 1.0f, 1.0f),
        "Q PROTOCOL");

    ImGui::SameLine();
    ImGui::TextDisabled(
        "007 FIRST LIGHT");

    ImGui::SameLine(
        ImGui::GetWindowWidth() - 155.0f);

    DrawStatusPill(
        g_playerReady.load(std::memory_order_acquire)
            ? "PLAYER READY"
            : "NOT READY",
        g_playerReady.load(std::memory_order_acquire));

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextUnformatted(
        "Direct DXGI / D3D12 overlay test");

    ImGui::TextDisabled(
        "This build validates the renderer only. The A5 gameplay core remains authoritative.");

    ImGui::Spacing();

    if (ImGui::BeginTable(
            "status",
            2,
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_BordersInnerH |
            ImGuiTableFlags_SizingStretchProp)) {

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextDisabled("Renderer");
        ImGui::TableNextColumn();
        DrawStatusPill("DX12 ImGui READY", true);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextDisabled("Player");
        ImGui::TableNextColumn();
        ImGui::Text(
            "%s",
            g_playerReady.load()
                ? "Ready"
                : "Not ready");

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextDisabled("AUTO");
        ImGui::TableNextColumn();
        ImGui::Text(
            "%s",
            g_autoDone.load()
                ? "Applied"
                : "Waiting");

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextDisabled("Weapon queue");
        ImGui::TableNextColumn();
        ImGui::Text(
            "%zu",
            g_queueCount.load());

        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped(
        "If you can see this panel, the new overlay path works. "
        "The next build will place Manual / Automatic / Weapons / Hotkeys here.");

    ImGui::Spacing();

    if (ImGui::Button(
            "Close  [Insert]",
            ImVec2(155.0f, 36.0f))) {
        g_requestedVisible.store(
            false,
            std::memory_order_release);
    }

    ImGui::End();
}

bool InitializeImGuiForSwapChain(
    IDXGISwapChain* swapChain) {

    if (!swapChain ||
        !g_commandQueue) {
        return false;
    }

    if (FAILED(
            swapChain->GetDevice(
                __uuidof(ID3D12Device),
                reinterpret_cast<void**>(
                    &g_device))) ||
        !g_device) {
        return false;
    }

    if (FAILED(
            swapChain->QueryInterface(
                __uuidof(IDXGISwapChain3),
                reinterpret_cast<void**>(
                    &g_swapChain3))) ||
        !g_swapChain3) {
        ReleaseDx12Resources();
        return false;
    }

    DXGI_SWAP_CHAIN_DESC desc{};
    if (FAILED(
            swapChain->GetDesc(&desc)) ||
        !desc.BufferCount ||
        !desc.OutputWindow) {
        ReleaseDx12Resources();
        return false;
    }

    g_gameWindow = desc.OutputWindow;
    g_backBufferFormat = desc.BufferDesc.Format;

    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
    rtvDesc.Type =
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDesc.NumDescriptors =
        desc.BufferCount;

    if (FAILED(
            g_device->CreateDescriptorHeap(
                &rtvDesc,
                IID_PPV_ARGS(&g_rtvHeap)))) {
        ReleaseDx12Resources();
        return false;
    }

    D3D12_DESCRIPTOR_HEAP_DESC srvDesc{};
    srvDesc.Type =
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvDesc.NumDescriptors = 1;
    srvDesc.Flags =
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    if (FAILED(
            g_device->CreateDescriptorHeap(
                &srvDesc,
                IID_PPV_ARGS(&g_srvHeap)))) {
        ReleaseDx12Resources();
        return false;
    }

    g_rtvDescriptorSize =
        g_device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    g_frames.resize(desc.BufferCount);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv =
        g_rtvHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0;
         i < desc.BufferCount;
         ++i) {

        FrameContext& frame = g_frames[i];

        if (FAILED(
                g_device->CreateCommandAllocator(
                    D3D12_COMMAND_LIST_TYPE_DIRECT,
                    IID_PPV_ARGS(
                        &frame.allocator)))) {
            ReleaseDx12Resources();
            return false;
        }

        if (FAILED(
                swapChain->GetBuffer(
                    i,
                    IID_PPV_ARGS(
                        &frame.renderTarget)))) {
            ReleaseDx12Resources();
            return false;
        }

        frame.rtv = rtv;

        g_device->CreateRenderTargetView(
            frame.renderTarget,
            nullptr,
            frame.rtv);

        rtv.ptr += g_rtvDescriptorSize;
    }

    if (FAILED(
            g_device->CreateCommandList(
                0,
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                g_frames[0].allocator,
                nullptr,
                IID_PPV_ARGS(
                    &g_commandList)))) {
        ReleaseDx12Resources();
        return false;
    }

    g_commandList->Close();

    if (FAILED(
            g_device->CreateFence(
                0,
                D3D12_FENCE_FLAG_NONE,
                IID_PPV_ARGS(&g_fence)))) {
        ReleaseDx12Resources();
        return false;
    }

    g_fenceEvent =
        CreateEventW(
            nullptr,
            FALSE,
            FALSE,
            nullptr);

    if (!g_fenceEvent) {
        ReleaseDx12Resources();
        return false;
    }

    if (!g_imguiContextCreated) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        g_imguiContextCreated = true;

        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.LogFilename = nullptr;
        io.MouseDrawCursor = true;

        ApplyStyle();
    }

    if (!g_imguiWin32Initialized) {
        if (!ImGui_ImplWin32_Init(
                g_gameWindow)) {
            ShutdownImGui();
            return false;
        }

        g_imguiWin32Initialized = true;

        g_originalWndProc =
            reinterpret_cast<WNDPROC>(
                SetWindowLongPtrW(
                    g_gameWindow,
                    GWLP_WNDPROC,
                    reinterpret_cast<LONG_PTR>(
                        &OverlayWndProc)));
    }

    if (!ImGui_ImplDX12_Init(
            g_device,
            desc.BufferCount,
            g_backBufferFormat,
            g_srvHeap,
            g_srvHeap->GetCPUDescriptorHandleForHeapStart(),
            g_srvHeap->GetGPUDescriptorHandleForHeapStart())) {
        ShutdownDx12Backend();
        return false;
    }

    g_imguiDx12Initialized = true;

    g_overlayReady.store(
        true,
        std::memory_order_release);

    g_stage.store(
        static_cast<int>(
            OverlayStage::ImGuiReady),
        std::memory_order_release);

    return true;
}

HRESULT __stdcall HookPresent(
    IDXGISwapChain* swapChain,
    UINT syncInterval,
    UINT flags) {

    if (!g_originalPresent) {
        return E_FAIL;
    }

    if (!g_imguiDx12Initialized) {
        if (!InitializeImGuiForSwapChain(
                swapChain)) {
            g_overlayReady.store(
                false,
                std::memory_order_release);

            g_stage.store(
                static_cast<int>(
                    OverlayStage::InitFailed),
                std::memory_order_release);

            return g_originalPresent(
                swapChain,
                syncInterval,
                flags);
        }
    }

    if (!g_overlayReady.load(
            std::memory_order_acquire) ||
        !g_requestedVisible.load(
            std::memory_order_acquire) ||
        !g_swapChain3 ||
        g_frames.empty()) {

        return g_originalPresent(
            swapChain,
            syncInterval,
            flags);
    }

    const UINT index =
        g_swapChain3->GetCurrentBackBufferIndex();

    if (index >= g_frames.size()) {
        return g_originalPresent(
            swapChain,
            syncInterval,
            flags);
    }

    FrameContext& frame = g_frames[index];

    if (!WaitForFenceValue(
            frame.fenceValue,
            1000)) {

        g_requestedVisible.store(
            false,
            std::memory_order_release);

        g_overlayReady.store(
            false,
            std::memory_order_release);

        g_stage.store(
            static_cast<int>(
                OverlayStage::FenceTimeout),
            std::memory_order_release);

        return g_originalPresent(
            swapChain,
            syncInterval,
            flags);
    }

    frame.fenceValue = 0;

    if (FAILED(
            frame.allocator->Reset()) ||
        FAILED(
            g_commandList->Reset(
                frame.allocator,
                nullptr))) {

        return g_originalPresent(
            swapChain,
            syncInterval,
            flags);
    }

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    DrawOverlayWindow();

    ImGui::Render();

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type =
        D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

    barrier.Transition.pResource =
        frame.renderTarget;

    barrier.Transition.Subresource =
        D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    barrier.Transition.StateBefore =
        D3D12_RESOURCE_STATE_PRESENT;

    barrier.Transition.StateAfter =
        D3D12_RESOURCE_STATE_RENDER_TARGET;

    g_commandList->ResourceBarrier(
        1,
        &barrier);

    g_commandList->OMSetRenderTargets(
        1,
        &frame.rtv,
        FALSE,
        nullptr);

    ID3D12DescriptorHeap* heaps[] = {
        g_srvHeap
    };

    g_commandList->SetDescriptorHeaps(
        1,
        heaps);

    ImGui_ImplDX12_RenderDrawData(
        ImGui::GetDrawData(),
        g_commandList);

    std::swap(
        barrier.Transition.StateBefore,
        barrier.Transition.StateAfter);

    g_commandList->ResourceBarrier(
        1,
        &barrier);

    if (FAILED(g_commandList->Close())) {
        return g_originalPresent(
            swapChain,
            syncInterval,
            flags);
    }

    ID3D12CommandList* lists[] = {
        g_commandList
    };

    g_commandQueue->ExecuteCommandLists(
        1,
        lists);

    const UINT64 fenceValue =
        ++g_nextFenceValue;

    if (SUCCEEDED(
            g_commandQueue->Signal(
                g_fence,
                fenceValue))) {
        frame.fenceValue = fenceValue;
    }

    return g_originalPresent(
        swapChain,
        syncInterval,
        flags);
}

HRESULT __stdcall HookResizeBuffers(
    IDXGISwapChain* swapChain,
    UINT bufferCount,
    UINT width,
    UINT height,
    DXGI_FORMAT newFormat,
    UINT swapChainFlags) {

    ShutdownDx12Backend();

    g_stage.store(
        static_cast<int>(
            OverlayStage::SwapChainCaptured),
        std::memory_order_release);

    return g_originalResizeBuffers(
        swapChain,
        bufferCount,
        width,
        height,
        newFormat,
        swapChainFlags);
}

bool HookSwapChainMethods(
    IUnknown* swapChainUnknown) {

    if (g_swapChainMethodsHooked) {
        return true;
    }

    if (!swapChainUnknown) {
        return false;
    }

    IDXGISwapChain* swapChain = nullptr;

    if (FAILED(
            swapChainUnknown->QueryInterface(
                __uuidof(IDXGISwapChain),
                reinterpret_cast<void**>(
                    &swapChain))) ||
        !swapChain) {
        return false;
    }

    void** vtable =
        *reinterpret_cast<void***>(
            swapChain);

    const MH_STATUS presentStatus =
        MH_CreateHook(
            vtable[8],
            reinterpret_cast<void*>(
                &HookPresent),
            reinterpret_cast<void**>(
                &g_originalPresent));

    const MH_STATUS resizeStatus =
        MH_CreateHook(
            vtable[13],
            reinterpret_cast<void*>(
                &HookResizeBuffers),
            reinterpret_cast<void**>(
                &g_originalResizeBuffers));

    bool ok =
        presentStatus == MH_OK &&
        resizeStatus == MH_OK;

    if (ok) {
        ok =
            MH_EnableHook(vtable[8]) == MH_OK &&
            MH_EnableHook(vtable[13]) == MH_OK;
    }

    swapChain->Release();

    if (!ok) {
        return false;
    }

    g_swapChainMethodsHooked = true;

    g_stage.store(
        static_cast<int>(
            OverlayStage::SwapChainCaptured),
        std::memory_order_release);

    return true;
}

void CaptureDirectQueue(
    IUnknown* deviceOrQueue) {

    if (!deviceOrQueue ||
        g_commandQueue) {
        return;
    }

    ID3D12CommandQueue* queue = nullptr;

    if (FAILED(
            deviceOrQueue->QueryInterface(
                __uuidof(ID3D12CommandQueue),
                reinterpret_cast<void**>(
                    &queue))) ||
        !queue) {
        return;
    }

    const D3D12_COMMAND_QUEUE_DESC desc =
        queue->GetDesc();

    if (desc.Type !=
        D3D12_COMMAND_LIST_TYPE_DIRECT) {
        queue->Release();
        return;
    }

    g_commandQueue = queue;
}

HRESULT __stdcall HookCreateSwapChain(
    IDXGIFactory* factory,
    IUnknown* device,
    DXGI_SWAP_CHAIN_DESC* desc,
    IDXGISwapChain** outSwapChain) {

    const HRESULT hr =
        g_originalCreateSwapChain(
            factory,
            device,
            desc,
            outSwapChain);

    if (SUCCEEDED(hr) &&
        outSwapChain &&
        *outSwapChain) {

        CaptureDirectQueue(device);
        HookSwapChainMethods(*outSwapChain);
    }

    return hr;
}

HRESULT __stdcall HookCreateSwapChainForHwnd(
    IDXGIFactory2* factory,
    IUnknown* device,
    HWND hwnd,
    const DXGI_SWAP_CHAIN_DESC1* desc,
    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreenDesc,
    IDXGIOutput* restrictOutput,
    IDXGISwapChain1** outSwapChain) {

    const HRESULT hr =
        g_originalCreateSwapChainForHwnd(
            factory,
            device,
            hwnd,
            desc,
            fullscreenDesc,
            restrictOutput,
            outSwapChain);

    if (SUCCEEDED(hr) &&
        outSwapChain &&
        *outSwapChain) {

        CaptureDirectQueue(device);
        HookSwapChainMethods(*outSwapChain);
    }

    return hr;
}

HRESULT __stdcall HookCreateSwapChainForCoreWindow(
    IDXGIFactory2* factory,
    IUnknown* device,
    IUnknown* window,
    const DXGI_SWAP_CHAIN_DESC1* desc,
    IDXGIOutput* restrictOutput,
    IDXGISwapChain1** outSwapChain) {

    const HRESULT hr =
        g_originalCreateSwapChainForCoreWindow(
            factory,
            device,
            window,
            desc,
            restrictOutput,
            outSwapChain);

    if (SUCCEEDED(hr) &&
        outSwapChain &&
        *outSwapChain) {

        CaptureDirectQueue(device);
        HookSwapChainMethods(*outSwapChain);
    }

    return hr;
}

HRESULT __stdcall HookCreateSwapChainForComposition(
    IDXGIFactory2* factory,
    IUnknown* device,
    const DXGI_SWAP_CHAIN_DESC1* desc,
    IDXGIOutput* restrictOutput,
    IDXGISwapChain1** outSwapChain) {

    const HRESULT hr =
        g_originalCreateSwapChainForComposition(
            factory,
            device,
            desc,
            restrictOutput,
            outSwapChain);

    if (SUCCEEDED(hr) &&
        outSwapChain &&
        *outSwapChain) {

        CaptureDirectQueue(device);
        HookSwapChainMethods(*outSwapChain);
    }

    return hr;
}

bool HookFactoryMethods() {
    IDXGIFactory2* factory2 = nullptr;

    if (FAILED(
            CreateDXGIFactory1(
                __uuidof(IDXGIFactory2),
                reinterpret_cast<void**>(
                    &factory2))) ||
        !factory2) {
        return false;
    }

    void** vtable2 =
        *reinterpret_cast<void***>(
            factory2);

    IDXGIFactory* factory0 = nullptr;
    factory2->QueryInterface(
        __uuidof(IDXGIFactory),
        reinterpret_cast<void**>(
            &factory0));

    bool ok = factory0 != nullptr;

    if (factory0) {
        void** vtable0 =
            *reinterpret_cast<void***>(
                factory0);

        ok =
            MH_CreateHook(
                vtable0[10],
                reinterpret_cast<void*>(
                    &HookCreateSwapChain),
                reinterpret_cast<void**>(
                    &g_originalCreateSwapChain)) == MH_OK &&
            MH_EnableHook(
                vtable0[10]) == MH_OK;

        factory0->Release();
    }

    if (ok) {
        ok =
            MH_CreateHook(
                vtable2[15],
                reinterpret_cast<void*>(
                    &HookCreateSwapChainForHwnd),
                reinterpret_cast<void**>(
                    &g_originalCreateSwapChainForHwnd)) == MH_OK &&
            MH_EnableHook(
                vtable2[15]) == MH_OK;
    }

    if (ok) {
        ok =
            MH_CreateHook(
                vtable2[16],
                reinterpret_cast<void*>(
                    &HookCreateSwapChainForCoreWindow),
                reinterpret_cast<void**>(
                    &g_originalCreateSwapChainForCoreWindow)) == MH_OK &&
            MH_EnableHook(
                vtable2[16]) == MH_OK;
    }

    if (ok) {
        ok =
            MH_CreateHook(
                vtable2[24],
                reinterpret_cast<void*>(
                    &HookCreateSwapChainForComposition),
                reinterpret_cast<void**>(
                    &g_originalCreateSwapChainForComposition)) == MH_OK &&
            MH_EnableHook(
                vtable2[24]) == MH_OK;
    }

    factory2->Release();

    if (!ok) {
        return false;
    }

    g_factoryMethodsHooked = true;

    g_stage.store(
        static_cast<int>(
            OverlayStage::FactoryHooksReady),
        std::memory_order_release);

    return true;
}

} // namespace

bool OverlayInitialize(
    const std::wstring& iniPath) {

    g_iniPath = iniPath;

    if (MH_Initialize() != MH_OK) {
        g_stage.store(
            static_cast<int>(
                OverlayStage::InitFailed),
            std::memory_order_release);
        return false;
    }

    g_minHookInitialized = true;

    if (!HookFactoryMethods()) {
        g_stage.store(
            static_cast<int>(
                OverlayStage::InitFailed),
            std::memory_order_release);
        return false;
    }

    return true;
}

void OverlayPump(
    bool playerReady,
    bool autoDone,
    std::size_t weaponQueueCount,
    bool qpistolNextB) {

    g_playerReady.store(
        playerReady,
        std::memory_order_release);

    g_autoDone.store(
        autoDone,
        std::memory_order_release);

    g_queueCount.store(
        weaponQueueCount,
        std::memory_order_release);

    g_qpistolNextB.store(
        qpistolNextB,
        std::memory_order_release);

    const bool insertDown =
        (GetAsyncKeyState(
             VK_INSERT) &
         0x8000) != 0;

    if (insertDown &&
        !g_insertWasDown) {

        const bool next =
            !g_requestedVisible.load(
                std::memory_order_acquire);

        g_requestedVisible.store(
            next,
            std::memory_order_release);

        if (next) {
            ClipCursor(nullptr);
        }
    }

    g_insertWasDown = insertDown;
}

bool OverlayConsumeReloadRequest() {
    return g_reloadRequested.exchange(
        false,
        std::memory_order_acq_rel);
}

bool OverlayIsVisible() {
    return g_overlayReady.load(
               std::memory_order_acquire) &&
           g_requestedVisible.load(
               std::memory_order_acquire);
}

const char* OverlayStatus() {
    switch (
        static_cast<OverlayStage>(
            g_stage.load(
                std::memory_order_acquire))) {

    case OverlayStage::Bootstrap:
        return "A8 DXGI bootstrap";

    case OverlayStage::FactoryHooksReady:
        return "A8 factory hooks ready; waiting for game swapchain";

    case OverlayStage::SwapChainCaptured:
        return "A8 game swapchain captured; waiting for first Present";

    case OverlayStage::ImGuiReady:
        return "A8 DX12 ImGui ready";

    case OverlayStage::InitFailed:
        return "A8 overlay initialization failed (A5 gameplay unaffected)";

    case OverlayStage::FenceTimeout:
        return "A8 overlay fence timeout (overlay disabled; A5 gameplay unaffected)";

    default:
        return "A8 overlay unknown state";
    }
}

void OverlayShutdown() {
    g_requestedVisible.store(
        false,
        std::memory_order_release);

    ShutdownImGui();

    if (g_commandQueue) {
        g_commandQueue->Release();
        g_commandQueue = nullptr;
    }

    if (g_minHookInitialized) {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        g_minHookInitialized = false;
    }
}

} // namespace qp
