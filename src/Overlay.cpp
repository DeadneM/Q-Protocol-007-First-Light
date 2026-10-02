#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <d3d12.h>
#include <dxgi1_4.h>

#include "Overlay.h"

#include "imgui.h"
#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_win32.h"
#include "kiero.h"

#include <algorithm>
#include <array>
#include <atomic>
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

constexpr std::uint16_t kKieroExecuteCommandLists = 54;
constexpr std::uint16_t kKieroPresent = 140;
constexpr std::uint16_t kKieroResizeBuffers = 145;

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

using ExecuteCommandListsFn = void(__stdcall*)(
    ID3D12CommandQueue*,
    UINT,
    ID3D12CommandList* const*);

struct FrameContext {
    ID3D12CommandAllocator* allocator = nullptr;
    ID3D12Resource* renderTarget = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv{};
    UINT64 fenceValue = 0;
};

struct WeaponEntry {
    std::string alias;
    std::string displayName;
    std::string role;
    std::string validation;
    std::uint64_t rid = 0;
};

struct ProfileUi {
    std::string qPistol;
    std::string oneHanded;
    std::string twoHanded;
    std::array<int, 6> ammo{10, 30, 30, 8, 5, 8};
};

std::wstring g_iniPath;

enum class OverlayStage : int {
    WaitingHooks = 0,
    HooksInstalled,
    QueueCaptured,
    Ready,
    InitFailed,
    FenceTimeout
};

std::atomic<bool> g_visible{false};
std::atomic<bool> g_overlayReady{false};
std::atomic<int> g_stage{static_cast<int>(OverlayStage::WaitingHooks)};
std::atomic<bool> g_reloadRequested{false};
std::atomic<bool> g_playerReady{false};
std::atomic<bool> g_autoDone{false};
std::atomic<std::size_t> g_queueCount{0};
std::atomic<bool> g_qpistolNextB{true};

bool g_insertWasDown = false;
bool g_hooksInstalled = false;
bool g_executeHookBound = false;
bool g_executeHookRetireRequested = false;
bool g_hookInitAttempted = false;
ULONGLONG g_nextHookRetryAt = 0;

PresentFn g_originalPresent = nullptr;
ResizeBuffersFn g_originalResizeBuffers = nullptr;
ExecuteCommandListsFn g_originalExecuteCommandLists = nullptr;

ID3D12CommandQueue* g_commandQueue = nullptr;
ID3D12Device* g_device = nullptr;
IDXGISwapChain3* g_swapChain3 = nullptr;
ID3D12DescriptorHeap* g_rtvHeap = nullptr;
ID3D12DescriptorHeap* g_srvHeap = nullptr;
ID3D12GraphicsCommandList* g_commandList = nullptr;
ID3D12Fence* g_fence = nullptr;
HANDLE g_fenceEvent = nullptr;
UINT64 g_nextFenceValue = 0;
std::vector<FrameContext> g_frames;
UINT g_rtvDescriptorSize = 0;
DXGI_FORMAT g_backBufferFormat = DXGI_FORMAT_UNKNOWN;
HWND g_gameWindow = nullptr;
WNDPROC g_originalWndProc = nullptr;

bool g_imguiContextCreated = false;
bool g_imguiWin32Initialized = false;
bool g_imguiDx12Initialized = false;

bool g_uiLoaded = false;
bool g_autoEnabledUi = true;
bool g_showExperimental = false;
ProfileUi g_manualUi{};
ProfileUi g_autoUi{};
std::vector<WeaponEntry> g_weapons;

const char* kAmmoKeys[6] = {
    "QPistol",
    "SMG",
    "AssaultRifle",
    "Shotgun",
    "Sniper",
    "HeavyPistol"
};

const wchar_t* kAmmoKeysW[6] = {
    L"QPistol",
    L"SMG",
    L"AssaultRifle",
    L"Shotgun",
    L"Sniper",
    L"HeavyPistol"
};

const char* kAmmoLabels[6] = {
    "Q-Pistol",
    "SMG",
    "Assault rifle",
    "Shotgun",
    "Sniper / marksman",
    "Heavy pistol"
};

std::wstring ReadIniW(
    const wchar_t* section,
    const wchar_t* key,
    const wchar_t* fallback = L"") {

    wchar_t buffer[512]{};
    GetPrivateProfileStringW(
        section,
        key,
        fallback,
        buffer,
        static_cast<DWORD>(std::size(buffer)),
        g_iniPath.c_str());
    return buffer;
}

std::string WideToUtf8(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }

    const int count = WideCharToMultiByte(
        CP_UTF8,
        0,
        value.c_str(),
        static_cast<int>(value.size()),
        nullptr,
        0,
        nullptr,
        nullptr);

    if (count <= 0) {
        return {};
    }

    std::string out(static_cast<std::size_t>(count), '\0');
    WideCharToMultiByte(
        CP_UTF8,
        0,
        value.c_str(),
        static_cast<int>(value.size()),
        out.data(),
        count,
        nullptr,
        nullptr);
    return out;
}

std::wstring Utf8ToWide(const std::string& value) {
    if (value.empty()) {
        return {};
    }

    const int count = MultiByteToWideChar(
        CP_UTF8,
        0,
        value.c_str(),
        static_cast<int>(value.size()),
        nullptr,
        0);

    if (count <= 0) {
        return {};
    }

    std::wstring out(static_cast<std::size_t>(count), L'\0');
    MultiByteToWideChar(
        CP_UTF8,
        0,
        value.c_str(),
        static_cast<int>(value.size()),
        out.data(),
        count);
    return out;
}

void WriteIniW(
    const wchar_t* section,
    const wchar_t* key,
    const std::wstring& value) {

    WritePrivateProfileStringW(
        section,
        key,
        value.c_str(),
        g_iniPath.c_str());
}

std::string HumanizeAlias(const std::string& alias) {
    std::string out;
    out.reserve(alias.size() + 8);

    for (std::size_t i = 0; i < alias.size(); ++i) {
        const char c = alias[i];
        const bool currentUpper = c >= 'A' && c <= 'Z';
        const bool currentDigit = c >= '0' && c <= '9';

        if (i > 0) {
            const char prev = alias[i - 1];
            const bool prevLower = prev >= 'a' && prev <= 'z';
            const bool prevDigit = prev >= '0' && prev <= '9';
            const bool nextLower =
                i + 1 < alias.size() &&
                alias[i + 1] >= 'a' &&
                alias[i + 1] <= 'z';

            if ((currentUpper && (prevLower || nextLower)) ||
                (currentDigit && !prevDigit) ||
                (!currentDigit && prevDigit)) {
                out.push_back(' ');
            }
        }

        out.push_back(c);
    }

    const auto replaceAll = [&](const std::string& from, const std::string& to) {
        std::size_t pos = 0;
        while ((pos = out.find(from, pos)) != std::string::npos) {
            out.replace(pos, from.size(), to);
            pos += to.size();
        }
    };

    replaceAll("QPistol", "Q-Pistol");
    replaceAll("Q Pistol", "Q-Pistol");
    replaceAll("SMG", "SMG");
    replaceAll("AR ", "AR ");

    return out;
}

std::uint64_t ParseDisplayRid(const std::wstring& value) {
    if (value.empty()) {
        return 0;
    }

    wchar_t* end = nullptr;
    const unsigned long long parsed =
        wcstoull(value.c_str(), &end, 16);

    if (!end || end == value.c_str() || *end != L'\0') {
        return 0;
    }

    return static_cast<std::uint64_t>(parsed);
}

void LoadWeaponCatalog() {
    g_weapons.clear();

    std::vector<wchar_t> buffer(65536);
    GetPrivateProfileSectionW(
        L"WeaponCatalog",
        buffer.data(),
        static_cast<DWORD>(buffer.size()),
        g_iniPath.c_str());

    const wchar_t* p = buffer.data();
    while (*p) {
        std::wstring line = p;
        const std::size_t eq = line.find(L'=');
        if (eq != std::wstring::npos && eq > 0) {
            const std::wstring aliasW = line.substr(0, eq);
            const std::wstring ridW = line.substr(eq + 1);

            WeaponEntry entry{};
            entry.alias = WideToUtf8(aliasW);
            entry.displayName = HumanizeAlias(entry.alias);
            entry.rid = ParseDisplayRid(ridW);

            entry.role = WideToUtf8(
                ReadIniW(
                    L"WeaponRole",
                    aliasW.c_str(),
                    L"Unknown"));

            entry.validation = WideToUtf8(
                ReadIniW(
                    L"WeaponValidation",
                    aliasW.c_str(),
                    L"Experimental"));

            g_weapons.push_back(std::move(entry));
        }

        p += line.size() + 1;
    }

    std::sort(
        g_weapons.begin(),
        g_weapons.end(),
        [](const WeaponEntry& a, const WeaponEntry& b) {
            if (a.role != b.role) {
                return a.role < b.role;
            }
            return a.displayName < b.displayName;
        });
}

void LoadProfile(
    const wchar_t* loadoutSection,
    const wchar_t* ammoSection,
    ProfileUi& profile) {

    profile.qPistol = WideToUtf8(
        ReadIniW(loadoutSection, L"QPistol", L"QPistolSilenced"));
    profile.oneHanded = WideToUtf8(
        ReadIniW(loadoutSection, L"OneHanded", L"MachinePistolHighRecoil"));
    profile.twoHanded = WideToUtf8(
        ReadIniW(loadoutSection, L"TwoHanded", L"ShotgunSemiAuto"));

    for (int i = 0; i < 6; ++i) {
        profile.ammo[static_cast<std::size_t>(i)] =
            std::clamp(
                static_cast<int>(
                    GetPrivateProfileIntW(
                        ammoSection,
                        kAmmoKeysW[i],
                        i == 0 ? 10 :
                        i == 1 ? 30 :
                        i == 2 ? 30 :
                        i == 3 ? 8 :
                        i == 4 ? 5 : 8,
                        g_iniPath.c_str())),
                0,
                100000);
    }
}

void LoadUiFromIni() {
    LoadWeaponCatalog();

    g_autoEnabledUi =
        GetPrivateProfileIntW(
            L"Auto",
            L"Enabled",
            1,
            g_iniPath.c_str()) != 0;

    LoadProfile(L"ManualLoadout", L"ManualAmmo", g_manualUi);
    LoadProfile(L"AutoLoadout", L"AutoAmmo", g_autoUi);

    g_uiLoaded = true;
}

void SaveProfile(
    const wchar_t* loadoutSection,
    const wchar_t* ammoSection,
    const ProfileUi& profile) {

    WriteIniW(
        loadoutSection,
        L"QPistol",
        Utf8ToWide(profile.qPistol));
    WriteIniW(
        loadoutSection,
        L"OneHanded",
        Utf8ToWide(profile.oneHanded));
    WriteIniW(
        loadoutSection,
        L"TwoHanded",
        Utf8ToWide(profile.twoHanded));

    for (int i = 0; i < 6; ++i) {
        WriteIniW(
            ammoSection,
            kAmmoKeysW[i],
            std::to_wstring(
                std::clamp(
                    profile.ammo[static_cast<std::size_t>(i)],
                    0,
                    100000)));
    }
}

void SaveUiToIni() {
    WriteIniW(
        L"Auto",
        L"Enabled",
        g_autoEnabledUi ? L"1" : L"0");

    SaveProfile(
        L"ManualLoadout",
        L"ManualAmmo",
        g_manualUi);

    SaveProfile(
        L"AutoLoadout",
        L"AutoAmmo",
        g_autoUi);

    WritePrivateProfileStringW(
        nullptr,
        nullptr,
        nullptr,
        g_iniPath.c_str());

    g_reloadRequested.store(true, std::memory_order_release);
}

void ResetDefaults() {
    g_autoEnabledUi = true;

    g_manualUi.qPistol = "QPistolSilenced";
    g_manualUi.oneHanded = "MachinePistolHighRecoil";
    g_manualUi.twoHanded = "ShotgunSemiAuto";
    g_manualUi.ammo = {10, 30, 30, 8, 5, 8};

    g_autoUi.qPistol = "QPistolSilenced";
    g_autoUi.oneHanded = "HeavyPistol50Cal";
    g_autoUi.twoHanded = "ARMilitary";
    g_autoUi.ammo = {10, 30, 30, 8, 5, 8};

    SaveUiToIni();
}

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
            g_visible.load(std::memory_order_acquire) &&
            IsInputMessage(msg)) {
            return 1;
        }
    }

    return CallWindowProcW(
        g_originalWndProc,
        hwnd,
        msg,
        wParam,
        lParam);
}

bool WaitForFenceValue(UINT64 value, DWORD timeoutMs) {
    if (!value || !g_fence || !g_fenceEvent) {
        return true;
    }

    if (g_fence->GetCompletedValue() >= value) {
        return true;
    }

    if (FAILED(g_fence->SetEventOnCompletion(value, g_fenceEvent))) {
        return false;
    }

    return WaitForSingleObject(g_fenceEvent, timeoutMs) == WAIT_OBJECT_0;
}

bool WaitForAllOverlayFrames(DWORD timeoutMs) {
    UINT64 maxValue = 0;
    for (const auto& frame : g_frames) {
        maxValue = (std::max)(maxValue, frame.fenceValue);
    }
    return WaitForFenceValue(maxValue, timeoutMs);
}

void ReleaseFrameResources() {
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
    g_overlayReady.store(false, std::memory_order_release);

    if (g_imguiDx12Initialized) {
        ImGui_ImplDX12_Shutdown();
        g_imguiDx12Initialized = false;
    }

    WaitForAllOverlayFrames(2000);
    ReleaseFrameResources();
}

void ShutdownImGui() {
    ShutdownDx12Backend();

    if (g_imguiWin32Initialized) {
        ImGui_ImplWin32_Shutdown();
        g_imguiWin32Initialized = false;
    }

    if (g_gameWindow && g_originalWndProc) {
        SetWindowLongPtrW(
            g_gameWindow,
            GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(g_originalWndProc));
    }

    g_originalWndProc = nullptr;
    g_gameWindow = nullptr;

    if (g_imguiContextCreated) {
        ImGui::DestroyContext();
        g_imguiContextCreated = false;
    }
}

void ApplyStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowPadding = ImVec2(18.0f, 16.0f);
    style.FramePadding = ImVec2(10.0f, 7.0f);
    style.ItemSpacing = ImVec2(10.0f, 9.0f);
    style.ItemInnerSpacing = ImVec2(7.0f, 6.0f);

    style.WindowRounding = 7.0f;
    style.ChildRounding = 5.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;

    ImVec4* c = style.Colors;
    c[ImGuiCol_Text] = ImVec4(0.92f, 0.94f, 0.97f, 1.00f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.48f, 0.52f, 0.58f, 1.00f);
    c[ImGuiCol_WindowBg] = ImVec4(0.055f, 0.063f, 0.078f, 0.97f);
    c[ImGuiCol_ChildBg] = ImVec4(0.075f, 0.086f, 0.105f, 0.96f);
    c[ImGuiCol_PopupBg] = ImVec4(0.070f, 0.080f, 0.098f, 0.99f);
    c[ImGuiCol_Border] = ImVec4(0.18f, 0.22f, 0.28f, 0.90f);
    c[ImGuiCol_FrameBg] = ImVec4(0.105f, 0.120f, 0.145f, 1.00f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.145f, 0.180f, 0.235f, 1.00f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.160f, 0.205f, 0.280f, 1.00f);
    c[ImGuiCol_TitleBg] = ImVec4(0.055f, 0.063f, 0.078f, 1.00f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.055f, 0.063f, 0.078f, 1.00f);
    c[ImGuiCol_CheckMark] = ImVec4(0.22f, 0.58f, 0.98f, 1.00f);
    c[ImGuiCol_SliderGrab] = ImVec4(0.22f, 0.58f, 0.98f, 1.00f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(0.32f, 0.68f, 1.00f, 1.00f);
    c[ImGuiCol_Button] = ImVec4(0.12f, 0.27f, 0.48f, 1.00f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.15f, 0.38f, 0.68f, 1.00f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.12f, 0.31f, 0.56f, 1.00f);
    c[ImGuiCol_Header] = ImVec4(0.12f, 0.27f, 0.48f, 0.65f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.15f, 0.38f, 0.68f, 0.85f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.12f, 0.31f, 0.56f, 1.00f);
    c[ImGuiCol_Tab] = ImVec4(0.085f, 0.100f, 0.125f, 1.00f);
    c[ImGuiCol_TabHovered] = ImVec4(0.15f, 0.38f, 0.68f, 1.00f);
    c[ImGuiCol_TabSelected] = ImVec4(0.12f, 0.27f, 0.48f, 1.00f);
    c[ImGuiCol_Separator] = ImVec4(0.18f, 0.22f, 0.28f, 1.00f);
}

bool InitializeImGuiForSwapChain(IDXGISwapChain* swapChain) {
    if (!swapChain) {
        return false;
    }

    if (FAILED(
            swapChain->GetDevice(
                __uuidof(ID3D12Device),
                reinterpret_cast<void**>(&g_device))) ||
        !g_device) {
        return false;
    }

    if (FAILED(
            swapChain->QueryInterface(
                __uuidof(IDXGISwapChain3),
                reinterpret_cast<void**>(&g_swapChain3))) ||
        !g_swapChain3) {
        ReleaseFrameResources();
        return false;
    }

    DXGI_SWAP_CHAIN_DESC desc{};
    if (FAILED(swapChain->GetDesc(&desc)) ||
        !desc.BufferCount ||
        !desc.OutputWindow) {
        ReleaseFrameResources();
        return false;
    }

    g_gameWindow = desc.OutputWindow;
    g_backBufferFormat = desc.BufferDesc.Format;

    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDesc.NumDescriptors = desc.BufferCount;
    rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    if (FAILED(
            g_device->CreateDescriptorHeap(
                &rtvDesc,
                IID_PPV_ARGS(&g_rtvHeap)))) {
        ReleaseFrameResources();
        return false;
    }

    D3D12_DESCRIPTOR_HEAP_DESC srvDesc{};
    srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvDesc.NumDescriptors = 1;
    srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    if (FAILED(
            g_device->CreateDescriptorHeap(
                &srvDesc,
                IID_PPV_ARGS(&g_srvHeap)))) {
        ReleaseFrameResources();
        return false;
    }

    g_rtvDescriptorSize =
        g_device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    g_frames.resize(desc.BufferCount);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv =
        g_rtvHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < desc.BufferCount; ++i) {
        FrameContext& frame = g_frames[i];

        if (FAILED(
                g_device->CreateCommandAllocator(
                    D3D12_COMMAND_LIST_TYPE_DIRECT,
                    IID_PPV_ARGS(&frame.allocator)))) {
            ReleaseFrameResources();
            return false;
        }

        if (FAILED(
                swapChain->GetBuffer(
                    i,
                    IID_PPV_ARGS(&frame.renderTarget)))) {
            ReleaseFrameResources();
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
                IID_PPV_ARGS(&g_commandList)))) {
        ReleaseFrameResources();
        return false;
    }

    g_commandList->Close();

    if (FAILED(
            g_device->CreateFence(
                0,
                D3D12_FENCE_FLAG_NONE,
                IID_PPV_ARGS(&g_fence)))) {
        ReleaseFrameResources();
        return false;
    }

    g_fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!g_fenceEvent) {
        ReleaseFrameResources();
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
        if (!ImGui_ImplWin32_Init(g_gameWindow)) {
            ShutdownImGui();
            return false;
        }

        g_imguiWin32Initialized = true;
        g_originalWndProc = reinterpret_cast<WNDPROC>(
            SetWindowLongPtrW(
                g_gameWindow,
                GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(&OverlayWndProc)));
    }

    const D3D12_CPU_DESCRIPTOR_HANDLE srvCpu =
        g_srvHeap->GetCPUDescriptorHandleForHeapStart();
    const D3D12_GPU_DESCRIPTOR_HANDLE srvGpu =
        g_srvHeap->GetGPUDescriptorHandleForHeapStart();

    if (!ImGui_ImplDX12_Init(
            g_device,
            desc.BufferCount,
            g_backBufferFormat,
            g_srvHeap,
            srvCpu,
            srvGpu)) {
        ShutdownDx12Backend();
        return false;
    }

    g_imguiDx12Initialized = true;
    g_overlayReady.store(true, std::memory_order_release);
    g_stage.store(static_cast<int>(OverlayStage::Ready), std::memory_order_release);
    g_uiLoaded = false;
    return true;
}

const WeaponEntry* FindWeapon(const std::string& alias) {
    for (const auto& weapon : g_weapons) {
        if (weapon.alias == alias) {
            return &weapon;
        }
    }
    return nullptr;
}

bool RoleMatches(
    const WeaponEntry& weapon,
    const char* role) {

    return weapon.role == role ||
           weapon.role == "Any";
}

bool ValidationVisible(const WeaponEntry& weapon) {
    return weapon.validation == "Validated" ||
           g_showExperimental;
}

void DrawValidationBadge(const WeaponEntry& weapon) {
    if (weapon.validation == "Validated") {
        ImGui::SameLine();
        ImGui::TextColored(
            ImVec4(0.28f, 0.82f, 0.48f, 1.0f),
            "validated");
    } else {
        ImGui::SameLine();
        ImGui::TextColored(
            ImVec4(0.95f, 0.66f, 0.25f, 1.0f),
            "experimental");
    }
}

bool WeaponCombo(
    const char* label,
    std::string& selectedAlias,
    const char* role) {

    const WeaponEntry* current =
        FindWeapon(selectedAlias);

    const char* preview =
        current
            ? current->displayName.c_str()
            : selectedAlias.c_str();

    bool changed = false;

    if (ImGui::BeginCombo(label, preview)) {
        for (const auto& weapon : g_weapons) {
            if (!RoleMatches(weapon, role) ||
                !ValidationVisible(weapon)) {
                continue;
            }

            const bool selected =
                weapon.alias == selectedAlias;

            std::string itemLabel =
                weapon.displayName +
                "##" +
                label +
                weapon.alias;

            if (ImGui::Selectable(
                    itemLabel.c_str(),
                    selected)) {
                selectedAlias = weapon.alias;
                changed = true;
            }

            if (selected) {
                ImGui::SetItemDefaultFocus();
            }

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("%s", weapon.alias.c_str());
                ImGui::Text(
                    "RID %016llX",
                    static_cast<unsigned long long>(
                        weapon.rid));

                if (weapon.validation == "Validated") {
                    ImGui::TextColored(
                        ImVec4(0.28f, 0.82f, 0.48f, 1.0f),
                        "Spawn validated in Q Protocol");
                } else {
                    ImGui::TextColored(
                        ImVec4(0.95f, 0.66f, 0.25f, 1.0f),
                        "Experimental / may require a resident source graph");
                }

                ImGui::EndTooltip();
            }
        }

        ImGui::EndCombo();
    }

    return changed;
}

void DrawAmmoGrid(ProfileUi& profile, const char* suffix) {
    if (ImGui::BeginTable(
            suffix,
            2,
            ImGuiTableFlags_SizingStretchSame)) {

        for (int i = 0; i < 6; ++i) {
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(kAmmoLabels[i]);

            ImGui::SameLine();
            ImGui::SetNextItemWidth(72.0f);

            std::string id =
                "##ammo_" +
                std::to_string(i) +
                suffix;

            ImGui::InputInt(
                id.c_str(),
                &profile.ammo[static_cast<std::size_t>(i)],
                1,
                10);

            profile.ammo[static_cast<std::size_t>(i)] =
                std::clamp(
                    profile.ammo[static_cast<std::size_t>(i)],
                    0,
                    100000);
        }

        ImGui::EndTable();
    }
}

void DrawProfilePanel(
    const char* title,
    ProfileUi& profile,
    bool automatic) {

    ImGui::PushID(title);

    ImGui::TextUnformatted(title);
    ImGui::Separator();
    ImGui::Spacing();

    if (automatic) {
        ImGui::Checkbox(
            "Apply automatically when player becomes ready",
            &g_autoEnabledUi);
        ImGui::Spacing();
    } else {
        ImGui::TextDisabled(
            "F2 = reserve ammo   |   F3 = manual loadout");
        ImGui::Spacing();
    }

    WeaponCombo(
        "Q-Pistol",
        profile.qPistol,
        "QPistol");

    WeaponCombo(
        "One-handed",
        profile.oneHanded,
        "OneHanded");

    WeaponCombo(
        "Two-handed",
        profile.twoHanded,
        "TwoHanded");

    ImGui::Spacing();
    ImGui::TextUnformatted("Reserve ammo");
    ImGui::Separator();
    DrawAmmoGrid(profile, automatic ? "auto" : "manual");

    ImGui::PopID();
}

void DrawLoadoutTab() {
    if (ImGui::BeginTable(
            "profiles",
            2,
            ImGuiTableFlags_SizingStretchSame |
            ImGuiTableFlags_BordersInnerV)) {

        ImGui::TableNextColumn();
        ImGui::BeginChild(
            "manual_profile",
            ImVec2(0.0f, 385.0f),
            ImGuiChildFlags_Borders);
        DrawProfilePanel(
            "MANUAL",
            g_manualUi,
            false);
        ImGui::EndChild();

        ImGui::TableNextColumn();
        ImGui::BeginChild(
            "auto_profile",
            ImVec2(0.0f, 385.0f),
            ImGuiChildFlags_Borders);
        DrawProfilePanel(
            "AUTOMATIC",
            g_autoUi,
            true);
        ImGui::EndChild();

        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Checkbox(
        "Show experimental weapons in loadout selectors",
        &g_showExperimental);

    ImGui::SameLine();
    ImGui::TextDisabled(
        "(validated only is safer)");
}

void DrawWeaponsTab() {
    ImGui::TextWrapped(
        "Full recovered firearm catalogue. 'Validated' means the spawn path "
        "has been confirmed in Q Protocol. Experimental entries are genuine "
        "internal/debug weapons but may not have a resident source graph in "
        "every mission.");

    ImGui::Spacing();

    if (ImGui::BeginTable(
            "weapon_catalog",
            4,
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_Borders |
            ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_Resizable,
            ImVec2(0.0f, 410.0f))) {

        ImGui::TableSetupColumn(
            "Weapon",
            ImGuiTableColumnFlags_WidthStretch,
            1.6f);
        ImGui::TableSetupColumn(
            "Role",
            ImGuiTableColumnFlags_WidthFixed,
            110.0f);
        ImGui::TableSetupColumn(
            "Status",
            ImGuiTableColumnFlags_WidthFixed,
            105.0f);
        ImGui::TableSetupColumn(
            "RID",
            ImGuiTableColumnFlags_WidthFixed,
            165.0f);
        ImGui::TableHeadersRow();

        for (const auto& weapon : g_weapons) {
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(
                weapon.displayName.c_str());

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(
                weapon.role.c_str());

            ImGui::TableNextColumn();
            if (weapon.validation == "Validated") {
                ImGui::TextColored(
                    ImVec4(0.28f, 0.82f, 0.48f, 1.0f),
                    "Validated");
            } else {
                ImGui::TextColored(
                    ImVec4(0.95f, 0.66f, 0.25f, 1.0f),
                    "Experimental");
            }

            ImGui::TableNextColumn();
            ImGui::Text(
                "%016llX",
                static_cast<unsigned long long>(
                    weapon.rid));
        }

        ImGui::EndTable();
    }
}

void DrawHotkeysTab() {
    const char* keys[] = {
        "F1", "F2", "F3", "F4",
        "F5", "F6", "F7", "F8",
        "F9", "F10", "F11", "F12"
    };

    const char* actions[] = {
        "License To Kill toggle",
        "Add Manual reserve ammo",
        "Apply Manual loadout",
        "Swap Q-Pistol",
        "Weapon slot 1",
        "Weapon slot 2",
        "Weapon slot 3",
        "Weapon slot 4",
        "Weapon slot 5",
        "Weapon slot 6",
        "Weapon slot 7",
        "Weapon slot 8"
    };

    if (ImGui::BeginTable(
            "hotkeys",
            2,
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_Borders |
            ImGuiTableFlags_SizingStretchProp)) {

        ImGui::TableSetupColumn(
            "Key",
            ImGuiTableColumnFlags_WidthFixed,
            80.0f);
        ImGui::TableSetupColumn(
            "Action",
            ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (int i = 0; i < 12; ++i) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(keys[i]);
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(actions[i]);
        }

        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::TextDisabled(
        "Editable F1-F12 bindings come after this overlay/render pass is validated.");
}

void DrawFooter() {
    ImGui::Separator();

    if (ImGui::Button(
            "Save",
            ImVec2(105.0f, 34.0f))) {
        SaveUiToIni();
    }

    ImGui::SameLine();

    if (ImGui::Button(
            "Reload",
            ImVec2(105.0f, 34.0f))) {
        LoadUiFromIni();
        g_reloadRequested.store(
            true,
            std::memory_order_release);
    }

    ImGui::SameLine();

    if (ImGui::Button(
            "Defaults",
            ImVec2(105.0f, 34.0f))) {
        ResetDefaults();
    }

    ImGui::SameLine();

    ImGui::TextDisabled(
        "Insert closes Q Protocol");
}

void DrawOverlayWindow() {
    if (!g_uiLoaded) {
        LoadUiFromIni();
    }

    ImGuiIO& io = ImGui::GetIO();

    const ImVec2 size(
        std::min(900.0f, io.DisplaySize.x - 40.0f),
        std::min(650.0f, io.DisplaySize.y - 40.0f));

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
            "Q Protocol",
            nullptr,
            flags)) {
        ImGui::End();
        return;
    }

    ImGui::TextColored(
        ImVec4(0.42f, 0.72f, 1.0f, 1.0f),
        "Q PROTOCOL");

    ImGui::SameLine();
    ImGui::TextDisabled(
        "007 FIRST LIGHT");

    ImGui::SameLine(
        ImGui::GetWindowWidth() - 260.0f);

    if (g_playerReady.load(std::memory_order_acquire)) {
        ImGui::TextColored(
            ImVec4(0.28f, 0.82f, 0.48f, 1.0f),
            "PLAYER READY");
    } else {
        ImGui::TextColored(
            ImVec4(0.95f, 0.55f, 0.30f, 1.0f),
            "PLAYER NOT READY");
    }

    ImGui::Separator();

    if (ImGui::BeginTabBar("main_tabs")) {
        if (ImGui::BeginTabItem("Loadout")) {
            DrawLoadoutTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Weapons")) {
            DrawWeaponsTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Hotkeys")) {
            DrawHotkeysTab();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    DrawFooter();
    ImGui::End();
}

HRESULT __stdcall HookPresent(
    IDXGISwapChain* swapChain,
    UINT syncInterval,
    UINT flags) {

    if (!g_originalPresent) {
        return E_FAIL;
    }

    if (!g_imguiDx12Initialized) {
        if (!g_commandQueue) {
            return g_originalPresent(
                swapChain,
                syncInterval,
                flags);
        }

        if (!InitializeImGuiForSwapChain(swapChain)) {
            g_overlayReady.store(false, std::memory_order_release);
            g_stage.store(static_cast<int>(OverlayStage::InitFailed), std::memory_order_release);
            return g_originalPresent(
                swapChain,
                syncInterval,
                flags);
        }
    }

    if (!g_overlayReady.load(std::memory_order_acquire) ||
        !g_visible.load(std::memory_order_acquire) ||
        !g_commandQueue ||
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

    if (!WaitForFenceValue(frame.fenceValue, 1000)) {
        g_visible.store(false, std::memory_order_release);
        g_overlayReady.store(false, std::memory_order_release);
        g_stage.store(static_cast<int>(OverlayStage::FenceTimeout), std::memory_order_release);
        return g_originalPresent(
            swapChain,
            syncInterval,
            flags);
    }
    frame.fenceValue = 0;

    if (FAILED(frame.allocator->Reset()) ||
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
    barrier.Flags =
        D3D12_RESOURCE_BARRIER_FLAG_NONE;
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

    const UINT64 fenceValue = ++g_nextFenceValue;
    if (SUCCEEDED(g_commandQueue->Signal(g_fence, fenceValue))) {
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

    WaitForAllOverlayFrames(2000);
    ShutdownDx12Backend();
    g_stage.store(
        g_commandQueue
            ? static_cast<int>(OverlayStage::QueueCaptured)
            : static_cast<int>(OverlayStage::HooksInstalled),
        std::memory_order_release);

    return g_originalResizeBuffers(
        swapChain,
        bufferCount,
        width,
        height,
        newFormat,
        swapChainFlags);
}

void __stdcall HookExecuteCommandLists(
    ID3D12CommandQueue* queue,
    UINT numCommandLists,
    ID3D12CommandList* const* lists) {

    if (!g_commandQueue && queue) {
        const D3D12_COMMAND_QUEUE_DESC desc =
            queue->GetDesc();

        if (desc.Type ==
            D3D12_COMMAND_LIST_TYPE_DIRECT) {

            queue->AddRef();
            g_commandQueue = queue;
            g_executeHookRetireRequested = true;
            g_stage.store(
                static_cast<int>(OverlayStage::QueueCaptured),
                std::memory_order_release);
        }
    }

    g_originalExecuteCommandLists(
        queue,
        numCommandLists,
        lists);
}

bool InstallDx12Hooks() {
    if (g_hooksInstalled) {
        return true;
    }

    const auto status =
        kiero::init(kiero::RenderType::D3D12);

    if (status != kiero::Status::Success &&
        status != kiero::Status::AlreadyInitializedError) {
        return false;
    }

    if (kiero::bind(
            kKieroExecuteCommandLists,
            reinterpret_cast<void**>(
                &g_originalExecuteCommandLists),
            reinterpret_cast<void*>(
                &HookExecuteCommandLists)) !=
        kiero::Status::Success) {
        kiero::shutdown();
        return false;
    }
    g_executeHookBound = true;

    if (kiero::bind(
            kKieroPresent,
            reinterpret_cast<void**>(
                &g_originalPresent),
            reinterpret_cast<void*>(
                &HookPresent)) !=
        kiero::Status::Success) {
        kiero::shutdown();
        return false;
    }

    if (kiero::bind(
            kKieroResizeBuffers,
            reinterpret_cast<void**>(
                &g_originalResizeBuffers),
            reinterpret_cast<void*>(
                &HookResizeBuffers)) !=
        kiero::Status::Success) {
        kiero::shutdown();
        return false;
    }

    g_hooksInstalled = true;
    g_stage.store(static_cast<int>(OverlayStage::HooksInstalled), std::memory_order_release);
    return true;
}

void TryInstallHooks() {
    if (g_hooksInstalled) {
        return;
    }

    const ULONGLONG now =
        GetTickCount64();

    if (now < g_nextHookRetryAt) {
        return;
    }

    g_nextHookRetryAt = now + 2000;

    g_hookInitAttempted = true;
    InstallDx12Hooks();
}

} // namespace

bool OverlayInitialize(
    const std::wstring& iniPath) {

    g_iniPath = iniPath;
    g_uiLoaded = false;
    TryInstallHooks();
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

    TryInstallHooks();

    if (g_executeHookRetireRequested && g_executeHookBound) {
        kiero::unbind(kKieroExecuteCommandLists);
        g_executeHookBound = false;
        g_executeHookRetireRequested = false;
    }

    const bool insertDown =
        (GetAsyncKeyState(VK_INSERT) &
         0x8000) != 0;

    if (insertDown &&
        !g_insertWasDown) {

        const bool next =
            !g_visible.load(
                std::memory_order_acquire);

        g_visible.store(
            next,
            std::memory_order_release);

        if (next) {
            ClipCursor(nullptr);
            g_uiLoaded = false;
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
    return g_overlayReady.load(std::memory_order_acquire) &&
           g_visible.load(std::memory_order_acquire);
}

const char* OverlayStatus() {
    switch (static_cast<OverlayStage>(
        g_stage.load(std::memory_order_acquire))) {
    case OverlayStage::WaitingHooks:
        return "DX12 waiting for hooks";
    case OverlayStage::HooksInstalled:
        return "DX12 hooks installed; waiting for DIRECT queue";
    case OverlayStage::QueueCaptured:
        return "DX12 queue captured; waiting for swapchain/ImGui";
    case OverlayStage::Ready:
        return "DX12 ImGui ready";
    case OverlayStage::InitFailed:
        return "DX12 ImGui initialization failed (gameplay fail-open)";
    case OverlayStage::FenceTimeout:
        return "DX12 overlay fence timeout (overlay disabled; gameplay fail-open)";
    default:
        return "DX12 overlay unknown state";
    }
}

void OverlayShutdown() {
    g_visible.store(
        false,
        std::memory_order_release);

    if (g_hooksInstalled) {
        kiero::shutdown();
        g_hooksInstalled = false;
        g_executeHookBound = false;
    }

    ShutdownImGui();

    if (g_commandQueue) {
        g_commandQueue->Release();
        g_commandQueue = nullptr;
    }
}

} // namespace qp
