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
#include <array>
#include <atomic>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
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
    std::string status;
    std::uint64_t rid = 0;
};

struct RuntimeEntry {
    std::string alias;
    std::string displayName;
    std::string category;
    std::uint64_t rid = 0;
};

struct ProfileUi {
    std::string qPistol;
    std::string oneHanded;
    std::string twoHanded;
    std::array<int, 6> ammo{10, 30, 30, 8, 5, 8};
};

enum class OverlayStage : int {
    Bootstrap = 0,
    RuntimeHooksReady,
    QueueCaptured,
    ImGuiReady,
    InitFailed,
    FenceTimeout
};

std::wstring g_iniPath;

std::atomic<bool> g_requestedVisible{false};
std::atomic<bool> g_overlayReady{false};
std::atomic<bool> g_reloadRequested{false};
std::atomic<std::uint64_t> g_spawnRequest{0};

std::atomic<bool> g_playerReady{false};
std::atomic<bool> g_autoDone{false};
std::atomic<std::size_t> g_queueCount{0};
std::atomic<bool> g_qpistolNextB{true};
std::atomic<int> g_stage{static_cast<int>(OverlayStage::Bootstrap)};

std::atomic<ID3D12CommandQueue*> g_commandQueue{nullptr};

std::atomic<int> g_overlayToggleVk{VK_INSERT};
std::atomic<bool> g_overlayToggleWasDown{false};
std::atomic<bool> g_captureOverlayToggleKey{false};
std::string g_overlayToggleKeyName = "Insert";

bool g_minHookInitialized = false;
bool g_hooksInstalled = false;

PresentFn g_originalPresent = nullptr;
ResizeBuffersFn g_originalResizeBuffers = nullptr;
ExecuteCommandListsFn g_originalExecuteCommandLists = nullptr;

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

bool g_uiLoaded = false;
bool g_autoEnabledUi = false;
bool g_showExperimentalLoadout = false;
ProfileUi g_manualUi{};
ProfileUi g_autoUi{};
std::vector<WeaponEntry> g_weapons;
std::vector<RuntimeEntry> g_runtimeCatalog;
int g_selectedWeapon = -1;
char g_weaponSearch[96]{};
std::string g_spawnMessage;

std::mutex g_runtimeRidMutex;
std::vector<std::uint64_t> g_currentRuntimeRids;
std::vector<std::uint64_t> g_sessionRuntimeRids;
std::uint64_t g_selectedDiscoveryRid = 0;

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
    "SMG / Machine Pistol",
    "Assault Rifle",
    "Shotgun",
    "Sniper / Marksman",
    "Heavy Pistol"
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

std::string WideToUtf8(
    const std::wstring& value) {

    if (value.empty()) {
        return {};
    }

    const int count =
        WideCharToMultiByte(
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

    std::string out(
        static_cast<std::size_t>(count),
        '\0');

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

std::wstring Utf8ToWide(
    const std::string& value) {

    if (value.empty()) {
        return {};
    }

    const int count =
        MultiByteToWideChar(
            CP_UTF8,
            0,
            value.c_str(),
            static_cast<int>(value.size()),
            nullptr,
            0);

    if (count <= 0) {
        return {};
    }

    std::wstring out(
        static_cast<std::size_t>(count),
        L'\0');

    MultiByteToWideChar(
        CP_UTF8,
        0,
        value.c_str(),
        static_cast<int>(value.size()),
        out.data(),
        count);

    return out;
}


std::string NormalizeKeyToken(
    std::string value) {

    std::string out;
    out.reserve(value.size());

    for (char c : value) {
        if (c == ' ' ||
            c == '_' ||
            c == '-') {
            continue;
        }

        out.push_back(
            static_cast<char>(
                std::toupper(
                    static_cast<unsigned char>(c))));
    }

    return out;
}

struct OverlayKeyName {
    int vk;
    const char* name;
    const char* token;
};

const OverlayKeyName kOverlayKeyNames[] = {
    {VK_INSERT, "Insert", "INSERT"},
    {VK_DELETE, "Delete", "DELETE"},
    {VK_HOME, "Home", "HOME"},
    {VK_END, "End", "END"},
    {VK_PRIOR, "PageUp", "PAGEUP"},
    {VK_NEXT, "PageDown", "PAGEDOWN"},
    {VK_TAB, "Tab", "TAB"},
    {VK_SPACE, "Space", "SPACE"},
    {VK_BACK, "Backspace", "BACKSPACE"},
    {VK_RETURN, "Enter", "ENTER"},
    {VK_PAUSE, "Pause", "PAUSE"},
    {VK_SCROLL, "ScrollLock", "SCROLLLOCK"},
    {VK_CAPITAL, "CapsLock", "CAPSLOCK"},
    {VK_LEFT, "Left", "LEFT"},
    {VK_RIGHT, "Right", "RIGHT"},
    {VK_UP, "Up", "UP"},
    {VK_DOWN, "Down", "DOWN"},
    {VK_NUMPAD0, "Numpad0", "NUMPAD0"},
    {VK_NUMPAD1, "Numpad1", "NUMPAD1"},
    {VK_NUMPAD2, "Numpad2", "NUMPAD2"},
    {VK_NUMPAD3, "Numpad3", "NUMPAD3"},
    {VK_NUMPAD4, "Numpad4", "NUMPAD4"},
    {VK_NUMPAD5, "Numpad5", "NUMPAD5"},
    {VK_NUMPAD6, "Numpad6", "NUMPAD6"},
    {VK_NUMPAD7, "Numpad7", "NUMPAD7"},
    {VK_NUMPAD8, "Numpad8", "NUMPAD8"},
    {VK_NUMPAD9, "Numpad9", "NUMPAD9"},
    {VK_MULTIPLY, "NumpadMultiply", "NUMPADMULTIPLY"},
    {VK_ADD, "NumpadAdd", "NUMPADADD"},
    {VK_SUBTRACT, "NumpadSubtract", "NUMPADSUBTRACT"},
    {VK_DECIMAL, "NumpadDecimal", "NUMPADDECIMAL"},
    {VK_DIVIDE, "NumpadDivide", "NUMPADDIVIDE"}
};

std::string OverlayKeyNameFromVk(
    int vk) {

    for (const auto& entry :
         kOverlayKeyNames) {

        if (entry.vk == vk) {
            return entry.name;
        }
    }

    if (vk >= 'A' && vk <= 'Z') {
        return std::string(
            1,
            static_cast<char>(vk));
    }

    if (vk >= '0' && vk <= '9') {
        return std::string(
            1,
            static_cast<char>(vk));
    }

    if (vk >= VK_F1 && vk <= VK_F24) {
        return "F" +
               std::to_string(
                   vk - VK_F1 + 1);
    }

    char buffer[16]{};
    sprintf_s(
        buffer,
        "VK_%02X",
        vk & 0xFF);

    return buffer;
}

int ParseOverlayToggleKey(
    const std::wstring& value) {

    const std::string token =
        NormalizeKeyToken(
            WideToUtf8(value));

    if (token.empty()) {
        return VK_INSERT;
    }

    for (const auto& entry :
         kOverlayKeyNames) {

        if (token == entry.token) {
            return entry.vk;
        }
    }

    if (token == "PGUP") {
        return VK_PRIOR;
    }

    if (token == "PGDN") {
        return VK_NEXT;
    }

    if (token == "ESC" ||
        token == "ESCAPE") {
        return VK_ESCAPE;
    }

    if (token.size() == 1) {
        const unsigned char c =
            static_cast<unsigned char>(
                token[0]);

        if ((c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9')) {
            return static_cast<int>(c);
        }
    }

    if (token[0] == 'F' &&
        token.size() <= 3) {

        const int n =
            std::atoi(
                token.c_str() + 1);

        if (n >= 1 && n <= 24) {
            return VK_F1 + n - 1;
        }
    }

    if (token.rfind("VK", 0) == 0 &&
        token.size() >= 3) {

        const char* p =
            token.c_str() + 2;

        char* end = nullptr;

        const unsigned long parsed =
            std::strtoul(
                p,
                &end,
                16);

        if (end &&
            end != p &&
            *end == '\0' &&
            parsed > 0 &&
            parsed <= 0xFF) {
            return static_cast<int>(
                parsed);
        }
    }

    return VK_INSERT;
}

void LoadOverlayToggleKeyFromIni() {

    const std::wstring configured =
        ReadIniW(
            L"Overlay",
            L"ToggleKey",
            L"Insert");

    const int vk =
        ParseOverlayToggleKey(
            configured);

    g_overlayToggleVk.store(
        vk,
        std::memory_order_release);

    g_overlayToggleKeyName =
        OverlayKeyNameFromVk(vk);

    const bool currentlyDown =
        (GetAsyncKeyState(vk) &
         0x8000) != 0;

    g_overlayToggleWasDown.store(
        currentlyDown,
        std::memory_order_release);
}

bool IsCapturableOverlayKey(
    int vk) {

    switch (vk) {
    case VK_LBUTTON:
    case VK_RBUTTON:
    case VK_MBUTTON:
    case VK_XBUTTON1:
    case VK_XBUTTON2:
    case VK_SHIFT:
    case VK_CONTROL:
    case VK_MENU:
    case VK_LSHIFT:
    case VK_RSHIFT:
    case VK_LCONTROL:
    case VK_RCONTROL:
    case VK_LMENU:
    case VK_RMENU:
    case VK_LWIN:
    case VK_RWIN:
        return false;
    default:
        return vk >= 1 &&
               vk <= 0xFF;
    }
}

void CaptureOverlayToggleKey() {

    if (!g_captureOverlayToggleKey.load(
            std::memory_order_acquire)) {
        return;
    }

    if ((GetAsyncKeyState(
             VK_ESCAPE) &
         0x8000) != 0) {

        g_captureOverlayToggleKey.store(
            false,
            std::memory_order_release);

        g_spawnMessage =
            "Overlay key capture cancelled";

        return;
    }

    for (int vk = 1;
         vk <= 0xFF;
         ++vk) {

        if (!IsCapturableOverlayKey(vk) ||
            vk == VK_ESCAPE) {
            continue;
        }

        if ((GetAsyncKeyState(vk) &
             0x8000) == 0) {
            continue;
        }

        g_overlayToggleVk.store(
            vk,
            std::memory_order_release);

        g_overlayToggleKeyName =
            OverlayKeyNameFromVk(vk);

        g_overlayToggleWasDown.store(
            true,
            std::memory_order_release);

        g_captureOverlayToggleKey.store(
            false,
            std::memory_order_release);

        g_spawnMessage =
            "Overlay key: " +
            g_overlayToggleKeyName;

        return;
    }
}

std::uint64_t ParseDisplayRid(
    const std::wstring& value) {

    if (value.empty()) {
        return 0;
    }

    wchar_t* end = nullptr;

    const unsigned long long parsed =
        wcstoull(
            value.c_str(),
            &end,
            16);

    if (!end ||
        end == value.c_str() ||
        *end != L'\0') {
        return 0;
    }

    return static_cast<std::uint64_t>(
        parsed);
}

std::string HumanizeAlias(
    const std::string& alias) {

    std::string out;
    out.reserve(alias.size() + 12);

    for (std::size_t i = 0;
         i < alias.size();
         ++i) {

        const char c = alias[i];

        if (i > 0) {
            const char p = alias[i - 1];

            const bool cUpper =
                c >= 'A' && c <= 'Z';

            const bool pLower =
                p >= 'a' && p <= 'z';

            const bool cDigit =
                c >= '0' && c <= '9';

            const bool pDigit =
                p >= '0' && p <= '9';

            const bool nextLower =
                i + 1 < alias.size() &&
                alias[i + 1] >= 'a' &&
                alias[i + 1] <= 'z';

            if ((cUpper && (pLower || nextLower)) ||
                (cDigit && !pDigit) ||
                (!cDigit && pDigit)) {
                out.push_back(' ');
            }
        }

        out.push_back(c);
    }

    std::size_t pos = 0;
    while ((pos = out.find("QPistol", pos)) != std::string::npos) {
        out.replace(pos, 7, "Q-Pistol");
        pos += 8;
    }

    return out;
}

bool IsKnownStatus(
    const std::string& value) {

    return value == "Validated" ||
           value == "Not Working" ||
           value == "Experimental";
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
        const std::wstring line = p;
        const std::size_t eq =
            line.find(L'=');

        if (eq != std::wstring::npos &&
            eq > 0) {

            const std::wstring aliasW =
                line.substr(0, eq);

            const std::wstring ridW =
                line.substr(eq + 1);

            WeaponEntry entry{};
            entry.alias = WideToUtf8(aliasW);
            entry.displayName =
                HumanizeAlias(entry.alias);
            entry.rid =
                ParseDisplayRid(ridW);

            entry.role = WideToUtf8(
                ReadIniW(
                    L"WeaponRole",
                    aliasW.c_str(),
                    L"Unknown"));

            entry.status = WideToUtf8(
                ReadIniW(
                    L"WeaponValidation",
                    aliasW.c_str(),
                    L"Experimental"));

            if (!IsKnownStatus(
                    entry.status)) {
                entry.status =
                    "Experimental";
            }

            g_weapons.push_back(
                std::move(entry));
        }

        p += line.size() + 1;
    }

    std::sort(
        g_weapons.begin(),
        g_weapons.end(),
        [](const WeaponEntry& a,
           const WeaponEntry& b) {

            if (a.role != b.role) {
                return a.role < b.role;
            }

            return a.displayName <
                   b.displayName;
        });

    if (g_selectedWeapon >=
        static_cast<int>(
            g_weapons.size())) {
        g_selectedWeapon = -1;
    }
}


void LoadRuntimeCatalog() {
    g_runtimeCatalog.clear();

    std::vector<wchar_t> buffer(32768);

    GetPrivateProfileSectionW(
        L"RuntimeCatalog",
        buffer.data(),
        static_cast<DWORD>(buffer.size()),
        g_iniPath.c_str());

    const wchar_t* p = buffer.data();

    while (*p) {
        const std::wstring line = p;
        const std::size_t eq =
            line.find(L'=');

        if (eq != std::wstring::npos &&
            eq > 0) {

            const std::wstring aliasW =
                line.substr(0, eq);

            const std::wstring ridW =
                line.substr(eq + 1);

            RuntimeEntry entry{};
            entry.alias = WideToUtf8(aliasW);
            entry.displayName =
                HumanizeAlias(entry.alias);
            entry.rid =
                ParseDisplayRid(ridW);

            entry.category = WideToUtf8(
                ReadIniW(
                    L"RuntimeCategory",
                    aliasW.c_str(),
                    L"Runtime Item"));

            if (entry.rid) {
                g_runtimeCatalog.push_back(
                    std::move(entry));
            }
        }

        p += line.size() + 1;
    }

    std::sort(
        g_runtimeCatalog.begin(),
        g_runtimeCatalog.end(),
        [](const RuntimeEntry& a,
           const RuntimeEntry& b) {

            if (a.category != b.category) {
                return a.category < b.category;
            }

            return a.displayName <
                   b.displayName;
        });
}

void LoadProfile(
    const wchar_t* loadoutSection,
    const wchar_t* ammoSection,
    ProfileUi& profile) {

    const bool automatic =
        _wcsicmp(
            loadoutSection,
            L"AutoLoadout") == 0;

    profile.qPistol = WideToUtf8(
        ReadIniW(
            loadoutSection,
            L"QPistol",
            L"QPistolSilenced"));

    profile.oneHanded = WideToUtf8(
        ReadIniW(
            loadoutSection,
            L"OneHanded",
            automatic
                ? L"None"
                : L"MachinePistolHighRecoil"));

    profile.twoHanded = WideToUtf8(
        ReadIniW(
            loadoutSection,
            L"TwoHanded",
            automatic
                ? L"None"
                : L"ShotgunSemiAuto"));

    const int defaults[6] = {
        10, 30, 30, 8, 5, 8
    };

    for (int i = 0; i < 6; ++i) {
        profile.ammo[
            static_cast<std::size_t>(i)] =
            std::clamp(
                static_cast<int>(
                    GetPrivateProfileIntW(
                        ammoSection,
                        kAmmoKeysW[i],
                        defaults[i],
                        g_iniPath.c_str())),
                0,
                100000);
    }
}

void LoadUiFromIni() {
    LoadWeaponCatalog();
    LoadRuntimeCatalog();
    LoadOverlayToggleKeyFromIni();

    g_autoEnabledUi =
        GetPrivateProfileIntW(
            L"Auto",
            L"Enabled",
            0,
            g_iniPath.c_str()) != 0;

    LoadProfile(
        L"ManualLoadout",
        L"ManualAmmo",
        g_manualUi);

    LoadProfile(
        L"AutoLoadout",
        L"AutoAmmo",
        g_autoUi);

    g_uiLoaded = true;
}

void SaveProfile(
    const wchar_t* loadoutSection,
    const wchar_t* ammoSection,
    const ProfileUi& profile) {

    WriteIniW(
        loadoutSection,
        L"QPistol",
        Utf8ToWide(
            profile.qPistol));

    WriteIniW(
        loadoutSection,
        L"OneHanded",
        Utf8ToWide(
            profile.oneHanded));

    WriteIniW(
        loadoutSection,
        L"TwoHanded",
        Utf8ToWide(
            profile.twoHanded));

    for (int i = 0; i < 6; ++i) {
        WriteIniW(
            ammoSection,
            kAmmoKeysW[i],
            std::to_wstring(
                std::clamp(
                    profile.ammo[
                        static_cast<std::size_t>(i)],
                    0,
                    100000)));
    }
}

void SaveWeaponStatuses() {
    for (const auto& weapon :
         g_weapons) {

        WriteIniW(
            L"WeaponValidation",
            Utf8ToWide(
                weapon.alias).c_str(),
            Utf8ToWide(
                weapon.status));
    }
}

void SaveUiToIni() {
    WriteIniW(
        L"Overlay",
        L"ToggleKey",
        Utf8ToWide(
            g_overlayToggleKeyName));

    WriteIniW(
        L"Auto",
        L"Enabled",
        g_autoEnabledUi
            ? L"1"
            : L"0");

    SaveProfile(
        L"ManualLoadout",
        L"ManualAmmo",
        g_manualUi);

    SaveProfile(
        L"AutoLoadout",
        L"AutoAmmo",
        g_autoUi);

    SaveWeaponStatuses();

    WritePrivateProfileStringW(
        nullptr,
        nullptr,
        nullptr,
        g_iniPath.c_str());

    g_reloadRequested.store(
        true,
        std::memory_order_release);

    g_spawnMessage = "Saved";
}

void ResetProfileDefaults() {
    g_autoEnabledUi = false;

    g_manualUi.qPistol =
        "QPistolSilenced";

    g_manualUi.oneHanded =
        "MachinePistolHighRecoil";

    g_manualUi.twoHanded =
        "ShotgunSemiAuto";

    g_manualUi.ammo = {
        10, 30, 30, 8, 5, 8
    };

    g_autoUi.qPistol =
        "QPistolSilenced";

    g_autoUi.oneHanded =
        "None";

    g_autoUi.twoHanded =
        "None";

    g_autoUi.ammo = {
        10, 30, 30, 8, 5, 8
    };

    SaveUiToIni();
}

const WeaponEntry* FindWeapon(
    const std::string& alias) {

    for (const auto& weapon :
         g_weapons) {

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

bool LoadoutWeaponVisible(
    const WeaponEntry& weapon) {

    if (weapon.status ==
        "Validated") {
        return true;
    }

    if (weapon.status ==
        "Experimental") {
        return g_showExperimentalLoadout;
    }

    return false;
}

bool WeaponCombo(
    const char* label,
    std::string& selectedAlias,
    const char* role,
    bool allowOff = false) {

    const WeaponEntry* current =
        FindWeapon(selectedAlias);

    const bool isOff =
        selectedAlias == "None" ||
        selectedAlias == "Off";

    const char* preview =
        isOff
            ? "Off"
            : current
                ? current->displayName.c_str()
                : selectedAlias.c_str();

    bool changed = false;

    if (ImGui::BeginCombo(
            label,
            preview)) {

        if (allowOff) {
            const bool selected = isOff;

            if (ImGui::Selectable(
                    "Off##qpistol_off",
                    selected)) {

                selectedAlias = "None";
                changed = true;
            }

            if (selected) {
                ImGui::SetItemDefaultFocus();
            }

            ImGui::Separator();
        }

        for (const auto& weapon :
             g_weapons) {

            if (!RoleMatches(
                    weapon,
                    role) ||
                !LoadoutWeaponVisible(
                    weapon)) {
                continue;
            }

            const bool selected =
                weapon.alias ==
                selectedAlias;

            std::string itemLabel =
                weapon.displayName +
                "##" +
                label +
                weapon.alias;

            if (ImGui::Selectable(
                    itemLabel.c_str(),
                    selected)) {

                selectedAlias =
                    weapon.alias;

                changed = true;
            }

            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    return changed;
}
void DrawAmmoGrid(
    ProfileUi& profile,
    const char* suffix) {

    if (!ImGui::BeginTable(
            suffix,
            2,
            ImGuiTableFlags_SizingStretchProp |
            ImGuiTableFlags_RowBg)) {
        return;
    }

    ImGui::TableSetupColumn(
        "Ammo type",
        ImGuiTableColumnFlags_WidthStretch);

    ImGui::TableSetupColumn(
        "Amount",
        ImGuiTableColumnFlags_WidthFixed,
        112.0f);

    for (int i = 0;
         i < 6;
         ++i) {

        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(
            kAmmoLabels[i]);

        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(
            100.0f);

        std::string id =
            "##ammo_" +
            std::to_string(i) +
            suffix;

        // No tiny +/- steppers: a clean numeric field is easier to read
        // and cannot be clipped in narrow profile columns.
        ImGui::InputInt(
            id.c_str(),
            &profile.ammo[
                static_cast<std::size_t>(i)],
            0,
            0);

        profile.ammo[
            static_cast<std::size_t>(i)] =
            std::clamp(
                profile.ammo[
                    static_cast<std::size_t>(i)],
                0,
                100000);
    }

    ImGui::EndTable();
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
            "Enable automatic loadout",
            &g_autoEnabledUi);

        ImGui::Spacing();
    } else {
        ImGui::TextDisabled(
            "F2 ammo   |   F3 loadout");

        ImGui::Spacing();
    }

    WeaponCombo(
        "Q-Pistol",
        profile.qPistol,
        "QPistol",
        true);

    WeaponCombo(
        "One-handed",
        profile.oneHanded,
        "OneHanded",
        true);

    WeaponCombo(
        "Two-handed",
        profile.twoHanded,
        "TwoHanded",
        true);

    ImGui::Spacing();
    ImGui::TextUnformatted(
        "Reserve ammo");
    ImGui::Separator();

    DrawAmmoGrid(
        profile,
        automatic
            ? "auto"
            : "manual");

    ImGui::PopID();
}

void DrawLoadoutTab() {
    if (ImGui::BeginTable(
            "loadout_profiles",
            2,
            ImGuiTableFlags_SizingStretchSame |
            ImGuiTableFlags_BordersInnerV)) {

        ImGui::TableNextColumn();

        ImGui::BeginChild(
            "manual_profile",
            ImVec2(0.0f, 485.0f),
            ImGuiChildFlags_Borders);

        DrawProfilePanel(
            "MANUAL",
            g_manualUi,
            false);

        ImGui::EndChild();

        ImGui::TableNextColumn();

        ImGui::BeginChild(
            "automatic_profile",
            ImVec2(0.0f, 485.0f),
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
        "Show Experimental weapons in loadout lists",
        &g_showExperimentalLoadout);

    ImGui::SameLine();

    ImGui::TextDisabled(
        "Not Working weapons stay hidden");
}

std::string Lower(
    std::string value) {

    for (char& c : value) {
        c = static_cast<char>(
            std::tolower(
                static_cast<unsigned char>(c)));
    }

    return value;
}

bool SearchMatches(
    const WeaponEntry& weapon) {

    if (!g_weaponSearch[0]) {
        return true;
    }

    const std::string q =
        Lower(g_weaponSearch);

    return
        Lower(weapon.displayName).find(q) !=
            std::string::npos ||
        Lower(weapon.alias).find(q) !=
            std::string::npos ||
        Lower(weapon.role).find(q) !=
            std::string::npos ||
        Lower(weapon.status).find(q) !=
            std::string::npos;
}

void DrawStatusText(
    const std::string& status) {

    if (status ==
        "Validated") {

        ImGui::TextColored(
            ImVec4(
                0.25f,
                0.86f,
                0.48f,
                1.0f),
            "Validated");

        return;
    }

    if (status ==
        "Not Working") {

        ImGui::TextColored(
            ImVec4(
                0.96f,
                0.30f,
                0.30f,
                1.0f),
            "Not Working");

        return;
    }

    ImGui::TextColored(
        ImVec4(
            0.96f,
            0.67f,
            0.24f,
            1.0f),
        "Experimental");
}

void DrawCatalogWeaponsTab() {
    ImGui::SetNextItemWidth(
        280.0f);

    ImGui::InputTextWithHint(
        "##weapon_search",
        "Search weapons...",
        g_weaponSearch,
        sizeof(g_weaponSearch));

    ImGui::SameLine();

    ImGui::TextDisabled(
        "Select a weapon, then Spawn Weapon");

    ImGui::Spacing();

    const float detailHeight =
        120.0f;

    if (ImGui::BeginTable(
            "weapon_catalog",
            4,
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_Borders |
            ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_Resizable,
            ImVec2(
                0.0f,
                320.0f))) {

        ImGui::TableSetupColumn(
            "Weapon",
            ImGuiTableColumnFlags_WidthStretch,
            1.6f);

        ImGui::TableSetupColumn(
            "Role",
            ImGuiTableColumnFlags_WidthFixed,
            120.0f);

        ImGui::TableSetupColumn(
            "Status",
            ImGuiTableColumnFlags_WidthFixed,
            115.0f);

        ImGui::TableSetupColumn(
            "RID",
            ImGuiTableColumnFlags_WidthFixed,
            170.0f);

        ImGui::TableHeadersRow();

        for (int i = 0;
             i < static_cast<int>(
                 g_weapons.size());
             ++i) {

            const WeaponEntry& weapon =
                g_weapons[
                    static_cast<std::size_t>(i)];

            if (!SearchMatches(weapon)) {
                continue;
            }

            ImGui::TableNextRow();

            ImGui::TableNextColumn();

            const bool selected =
                i == g_selectedWeapon;

            std::string selectable =
                weapon.displayName +
                "##weapon_" +
                std::to_string(i);

            if (ImGui::Selectable(
                    selectable.c_str(),
                    selected,
                    ImGuiSelectableFlags_SpanAllColumns)) {

                g_selectedWeapon = i;
                g_spawnMessage.clear();
            }

            ImGui::TableNextColumn();

            ImGui::TextUnformatted(
                weapon.role.c_str());

            ImGui::TableNextColumn();

            DrawStatusText(
                weapon.status);

            ImGui::TableNextColumn();

            ImGui::Text(
                "%016llX",
                static_cast<unsigned long long>(
                    weapon.rid));
        }

        ImGui::EndTable();
    }

    ImGui::Spacing();

    ImGui::BeginChild(
        "weapon_detail",
        ImVec2(
            0.0f,
            detailHeight),
        ImGuiChildFlags_Borders);

    if (g_selectedWeapon >= 0 &&
        g_selectedWeapon <
        static_cast<int>(
            g_weapons.size())) {

        WeaponEntry& weapon =
            g_weapons[
                static_cast<std::size_t>(
                    g_selectedWeapon)];

        ImGui::Text(
            "%s",
            weapon.displayName.c_str());

        ImGui::SameLine();

        ImGui::TextDisabled(
            "(%s)",
            weapon.alias.c_str());

        ImGui::Spacing();

        ImGui::TextUnformatted(
            "Status");

        ImGui::SameLine();

        ImGui::SetNextItemWidth(
            155.0f);

        if (ImGui::BeginCombo(
                "##weapon_status",
                weapon.status.c_str())) {

            const char* statuses[] = {
                "Validated",
                "Not Working",
                "Experimental"
            };

            for (const char* status :
                 statuses) {

                const bool selected =
                    weapon.status ==
                    status;

                if (ImGui::Selectable(
                        status,
                        selected)) {

                    weapon.status =
                        status;
                }

                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }

        ImGui::SameLine();

        if (ImGui::Button(
                "Spawn Weapon",
                ImVec2(
                    155.0f,
                    32.0f))) {

            if (!g_playerReady.load(
                    std::memory_order_acquire)) {

                g_spawnMessage =
                    "Player not ready";

            } else if (!weapon.rid) {

                g_spawnMessage =
                    "Invalid RID";

            } else {

                std::uint64_t expected = 0;

                if (g_spawnRequest
                        .compare_exchange_strong(
                            expected,
                            weapon.rid,
                            std::memory_order_acq_rel)) {

                    g_spawnMessage =
                        "Queued: " +
                        weapon.displayName;

                } else {

                    g_spawnMessage =
                        "Spawn request already pending";
                }
            }
        }

        if (!g_spawnMessage.empty()) {
            ImGui::SameLine();

            ImGui::TextDisabled(
                "%s",
                g_spawnMessage.c_str());
        }

    } else {

        ImGui::TextDisabled(
            "Select a weapon from the list.");
    }

    ImGui::EndChild();
}


bool IsCataloguedRid(std::uint64_t rid) {
    for (const auto& weapon : g_weapons) {
        if (weapon.rid == rid) {
            return true;
        }
    }
    return false;
}

const RuntimeEntry* FindRuntimeEntryByRid(
    std::uint64_t rid) {

    for (const auto& entry :
         g_runtimeCatalog) {

        if (entry.rid == rid) {
            return &entry;
        }
    }

    return nullptr;
}

bool IsCurrentRuntimeRid(
    std::uint64_t rid,
    const std::vector<std::uint64_t>& current) {

    return std::find(
               current.begin(),
               current.end(),
               rid) != current.end();
}

bool QueueOverlaySpawn(
    std::uint64_t displayRid,
    const std::string& label) {

    if (!g_playerReady.load(
            std::memory_order_acquire)) {

        g_spawnMessage =
            "Player not ready";
        return false;
    }

    if (!displayRid) {
        g_spawnMessage =
            "Invalid RID";
        return false;
    }

    std::uint64_t expected = 0;

    if (g_spawnRequest
            .compare_exchange_strong(
                expected,
                displayRid,
                std::memory_order_acq_rel)) {

        g_spawnMessage =
            "Queued: " + label;
        return true;
    }

    g_spawnMessage =
        "Spawn request already pending";
    return false;
}

void DrawRuntimeDiscoveryTab() {
    std::vector<std::uint64_t> current;
    std::vector<std::uint64_t> session;

    {
        std::lock_guard<std::mutex> lock(
            g_runtimeRidMutex);

        current =
            g_currentRuntimeRids;

        session =
            g_sessionRuntimeRids;
    }

    std::vector<std::uint64_t> runtimeItems;

    for (const std::uint64_t rid :
         session) {

        if (!IsCataloguedRid(rid)) {
            runtimeItems.push_back(rid);
        }
    }

    std::sort(
        runtimeItems.begin(),
        runtimeItems.end());

    runtimeItems.erase(
        std::unique(
            runtimeItems.begin(),
            runtimeItems.end()),
        runtimeItems.end());

    std::size_t knownSession = 0;
    std::size_t unknownSession = 0;
    std::size_t unknownCurrent = 0;

    for (const std::uint64_t rid :
         runtimeItems) {

        if (FindRuntimeEntryByRid(rid)) {
            ++knownSession;
        } else {
            ++unknownSession;
        }
    }

    for (const std::uint64_t rid :
         current) {

        if (!IsCataloguedRid(rid) &&
            !FindRuntimeEntryByRid(rid)) {
            ++unknownCurrent;
        }
    }

    ImGui::Text(
        "Current level graphs: %zu",
        current.size());

    ImGui::SameLine();
    ImGui::TextDisabled("|");

    ImGui::SameLine();
    ImGui::Text(
        "Known runtime items: %zu",
        knownSession);

    ImGui::SameLine();
    ImGui::TextDisabled("|");

    ImGui::SameLine();
    ImGui::Text(
        "Unknown current/session: %zu / %zu",
        unknownCurrent,
        unknownSession);

    ImGui::Spacing();

    ImGui::TextWrapped(
        "A19 keeps A11 discovery intact but now identifies known non-firearm "
        "ItemEntry/Spawner graphs. Gadgets and throwables stay outside "
        "WeaponCatalog, while unknown RIDs remain visible for further mapping.");

    ImGui::Spacing();

    if (ImGui::BeginTable(
            "runtime_discovery",
            5,
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_Borders |
            ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_Resizable,
            ImVec2(0.0f, 350.0f))) {

        ImGui::TableSetupColumn(
            "Item",
            ImGuiTableColumnFlags_WidthStretch,
            1.5f);

        ImGui::TableSetupColumn(
            "Type",
            ImGuiTableColumnFlags_WidthFixed,
            105.0f);

        ImGui::TableSetupColumn(
            "Current level",
            ImGuiTableColumnFlags_WidthFixed,
            105.0f);

        ImGui::TableSetupColumn(
            "State",
            ImGuiTableColumnFlags_WidthFixed,
            105.0f);

        ImGui::TableSetupColumn(
            "RID",
            ImGuiTableColumnFlags_WidthFixed,
            170.0f);

        ImGui::TableHeadersRow();

        for (const std::uint64_t rid :
             runtimeItems) {

            const RuntimeEntry* entry =
                FindRuntimeEntryByRid(rid);

            char ridText[32]{};
            sprintf_s(
                ridText,
                "%016llX",
                static_cast<unsigned long long>(rid));

            const std::string displayName =
                entry
                    ? entry->displayName
                    : std::string("Unknown runtime item");

            const std::string category =
                entry
                    ? entry->category
                    : std::string("Unknown");

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            const bool selected =
                g_selectedDiscoveryRid == rid;

            std::string selectable =
                displayName +
                "##runtime_" +
                ridText;

            if (ImGui::Selectable(
                    selectable.c_str(),
                    selected,
                    ImGuiSelectableFlags_SpanAllColumns)) {

                g_selectedDiscoveryRid = rid;
                g_spawnMessage.clear();
            }

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(
                category.c_str());

            ImGui::TableNextColumn();

            if (IsCurrentRuntimeRid(
                    rid,
                    current)) {

                ImGui::TextColored(
                    ImVec4(
                        0.25f,
                        0.86f,
                        0.48f,
                        1.0f),
                    "Present");

            } else {

                ImGui::TextDisabled(
                    "Seen earlier");
            }

            ImGui::TableNextColumn();

            if (entry) {
                ImGui::TextColored(
                    ImVec4(
                        0.25f,
                        0.70f,
                        1.0f,
                        1.0f),
                    "Identified");
            } else {
                ImGui::TextColored(
                    ImVec4(
                        0.96f,
                        0.67f,
                        0.24f,
                        1.0f),
                    "Uncatalogued");
            }

            ImGui::TableNextColumn();
            ImGui::Text(
                "%016llX",
                static_cast<unsigned long long>(rid));
        }

        ImGui::EndTable();
    }

    ImGui::Spacing();

    ImGui::BeginChild(
        "runtime_discovery_detail",
        ImVec2(0.0f, 105.0f),
        ImGuiChildFlags_Borders);

    if (g_selectedDiscoveryRid) {
        const RuntimeEntry* selectedEntry =
            FindRuntimeEntryByRid(
                g_selectedDiscoveryRid);

        if (selectedEntry) {
            ImGui::TextUnformatted(
                selectedEntry->displayName.c_str());

            ImGui::SameLine();

            ImGui::TextDisabled(
                "(%s)",
                selectedEntry->category.c_str());
        } else {
            ImGui::TextUnformatted(
                "Unknown runtime item");
        }

        ImGui::Text(
            "Runtime RID: %016llX",
            static_cast<unsigned long long>(
                g_selectedDiscoveryRid));

        ImGui::SameLine();

        if (ImGui::Button(
                "Spawn Weapon",
                ImVec2(
                    155.0f,
                    32.0f))) {

            std::string label;

            if (selectedEntry) {
                label =
                    selectedEntry->displayName;
            } else {
                char textRid[64]{};
                sprintf_s(
                    textRid,
                    "Runtime RID %016llX",
                    static_cast<unsigned long long>(
                        g_selectedDiscoveryRid));
                label = textRid;
            }

            QueueOverlaySpawn(
                g_selectedDiscoveryRid,
                label);
        }

        if (!g_spawnMessage.empty()) {
            ImGui::SameLine();

            ImGui::TextDisabled(
                "%s",
                g_spawnMessage.c_str());
        }

    } else {

        ImGui::TextDisabled(
            "Select a runtime item.");
    }

    ImGui::EndChild();
}

void DrawWeaponsTab() {
    if (ImGui::BeginTabBar(
            "weapon_subtabs")) {

        if (ImGui::BeginTabItem(
                "Catalog")) {

            DrawCatalogWeaponsTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(
                "Runtime Discovery")) {

            DrawRuntimeDiscoveryTab();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

void DrawHotkeysTab() {
    ImGui::TextUnformatted(
        "Overlay");

    ImGui::SameLine();

    ImGui::TextDisabled(
        "Menu toggle key");

    ImGui::SetNextItemWidth(
        190.0f);

    if (g_captureOverlayToggleKey.load(
            std::memory_order_acquire)) {

        if (ImGui::Button(
                "Press a key...##overlay_toggle",
                ImVec2(
                    190.0f,
                    32.0f))) {

            g_captureOverlayToggleKey.store(
                false,
                std::memory_order_release);
        }

        CaptureOverlayToggleKey();

        ImGui::SameLine();

        ImGui::TextDisabled(
            "Esc cancels");

    } else {

        const std::string label =
            g_overlayToggleKeyName +
            "##overlay_toggle";

        if (ImGui::Button(
                label.c_str(),
                ImVec2(
                    190.0f,
                    32.0f))) {

            g_captureOverlayToggleKey.store(
                true,
                std::memory_order_release);

            g_spawnMessage =
                "Press a key for Overlay Toggle";
        }

        ImGui::SameLine();

        ImGui::TextDisabled(
            "Click, then press a new key");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const char* keys[] = {
        "F1", "F2", "F3", "F4",
        "F5", "F6", "F7", "F8",
        "F9", "F10", "F11", "F12"
    };

    const char* actions[] = {
        "License To Kill",
        "Manual Ammo",
        "Manual Loadout",
        "Q-Pistol Swap",
        "Weapon 1",
        "Weapon 2",
        "Weapon 3",
        "Weapon 4",
        "Weapon 5",
        "Weapon 6",
        "Weapon 7",
        "Weapon 8"
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

        for (int i = 0;
             i < 12;
             ++i) {

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::TextUnformatted(
                keys[i]);

            ImGui::TableNextColumn();

            ImGui::TextUnformatted(
                actions[i]);
        }

        ImGui::EndTable();
    }
}

void DrawFooter() {
    ImGui::Separator();

    if (ImGui::Button(
            "Save",
            ImVec2(
                105.0f,
                34.0f))) {

        SaveUiToIni();
    }

    ImGui::SameLine();

    if (ImGui::Button(
            "Reload",
            ImVec2(
                105.0f,
                34.0f))) {

        LoadUiFromIni();

        g_reloadRequested.store(
            true,
            std::memory_order_release);

        g_spawnMessage =
            "Reloaded";
    }

    ImGui::SameLine();

    if (ImGui::Button(
            "Defaults",
            ImVec2(
                105.0f,
                34.0f))) {

        ResetProfileDefaults();

        g_spawnMessage =
            "Profile defaults restored";
    }

    ImGui::SameLine();

    ImGui::TextDisabled(
        "%s closes Q Protocol",
        g_overlayToggleKeyName.c_str());
}

void ApplyStyle() {
    ImGuiStyle& style =
        ImGui::GetStyle();

    style.WindowPadding =
        ImVec2(18.0f, 16.0f);

    style.FramePadding =
        ImVec2(10.0f, 7.0f);

    style.ItemSpacing =
        ImVec2(10.0f, 9.0f);

    style.ItemInnerSpacing =
        ImVec2(7.0f, 6.0f);

    style.WindowRounding = 7.0f;
    style.ChildRounding = 5.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;

    ImVec4* c =
        style.Colors;

    c[ImGuiCol_Text] =
        ImVec4(
            0.92f,
            0.94f,
            0.97f,
            1.00f);

    c[ImGuiCol_TextDisabled] =
        ImVec4(
            0.48f,
            0.52f,
            0.58f,
            1.00f);

    c[ImGuiCol_WindowBg] =
        ImVec4(
            0.035f,
            0.043f,
            0.058f,
            0.97f);

    c[ImGuiCol_ChildBg] =
        ImVec4(
            0.055f,
            0.067f,
            0.090f,
            0.96f);

    c[ImGuiCol_PopupBg] =
        ImVec4(
            0.050f,
            0.060f,
            0.080f,
            0.99f);

    c[ImGuiCol_Border] =
        ImVec4(
            0.16f,
            0.26f,
            0.39f,
            0.95f);

    c[ImGuiCol_FrameBg] =
        ImVec4(
            0.075f,
            0.095f,
            0.130f,
            1.00f);

    c[ImGuiCol_FrameBgHovered] =
        ImVec4(
            0.11f,
            0.17f,
            0.25f,
            1.00f);

    c[ImGuiCol_FrameBgActive] =
        ImVec4(
            0.13f,
            0.20f,
            0.30f,
            1.00f);

    c[ImGuiCol_CheckMark] =
        ImVec4(
            0.22f,
            0.62f,
            1.00f,
            1.00f);

    c[ImGuiCol_Button] =
        ImVec4(
            0.08f,
            0.30f,
            0.56f,
            1.00f);

    c[ImGuiCol_ButtonHovered] =
        ImVec4(
            0.10f,
            0.42f,
            0.76f,
            1.00f);

    c[ImGuiCol_ButtonActive] =
        ImVec4(
            0.08f,
            0.34f,
            0.64f,
            1.00f);

    c[ImGuiCol_Header] =
        ImVec4(
            0.08f,
            0.30f,
            0.56f,
            0.70f);

    c[ImGuiCol_HeaderHovered] =
        ImVec4(
            0.10f,
            0.42f,
            0.76f,
            0.88f);

    c[ImGuiCol_HeaderActive] =
        ImVec4(
            0.08f,
            0.34f,
            0.64f,
            1.00f);

    c[ImGuiCol_Tab] =
        ImVec4(
            0.055f,
            0.070f,
            0.100f,
            1.00f);

    c[ImGuiCol_TabHovered] =
        ImVec4(
            0.10f,
            0.42f,
            0.76f,
            1.00f);

    c[ImGuiCol_TabSelected] =
        ImVec4(
            0.08f,
            0.30f,
            0.56f,
            1.00f);
}

void DrawOverlayWindow() {
    if (!g_uiLoaded) {
        LoadUiFromIni();
    }

    ImGuiIO& io =
        ImGui::GetIO();

    const ImVec2 size(
        (std::min)(
            980.0f,
            io.DisplaySize.x -
                40.0f),
        (std::min)(
            740.0f,
            io.DisplaySize.y -
                40.0f));

    const ImVec2 pos(
        (io.DisplaySize.x -
         size.x) *
            0.5f,
        (io.DisplaySize.y -
         size.y) *
            0.5f);

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
        ImVec4(
            0.30f,
            0.70f,
            1.0f,
            1.0f),
        "Q PROTOCOL");

    ImGui::SameLine();

    ImGui::TextDisabled(
        "007 FIRST LIGHT");

    ImGui::SameLine(
        ImGui::GetWindowWidth() -
            235.0f);

    if (g_playerReady.load(
            std::memory_order_acquire)) {

        ImGui::TextColored(
            ImVec4(
                0.25f,
                0.86f,
                0.48f,
                1.0f),
            "PLAYER READY");

    } else {

        ImGui::TextColored(
            ImVec4(
                0.96f,
                0.48f,
                0.25f,
                1.0f),
            "PLAYER NOT READY");
    }

    ImGui::Separator();

    if (ImGui::BeginTabBar(
            "main_tabs")) {

        if (ImGui::BeginTabItem(
                "Loadout")) {

            DrawLoadoutTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(
                "Weapons")) {

            DrawWeaponsTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(
                "Hotkeys")) {

            DrawHotkeysTab();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    DrawFooter();

    ImGui::End();
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

        if (g_overlayReady.load(
                std::memory_order_acquire) &&
            g_requestedVisible.load(
                std::memory_order_acquire) &&
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

    return DefWindowProcW(
        hwnd,
        msg,
        wParam,
        lParam);
}

bool WaitForFenceValue(
    UINT64 value,
    DWORD timeoutMs) {

    if (!value ||
        !g_fence ||
        !g_fenceEvent) {

        return true;
    }

    if (g_fence->GetCompletedValue() >=
        value) {

        return true;
    }

    if (FAILED(
            g_fence
                ->SetEventOnCompletion(
                    value,
                    g_fenceEvent))) {

        return false;
    }

    return WaitForSingleObject(
               g_fenceEvent,
               timeoutMs) ==
           WAIT_OBJECT_0;
}

bool WaitForAllFrames(
    DWORD timeoutMs) {

    UINT64 last = 0;

    for (const auto& frame :
         g_frames) {

        last =
            (std::max)(
                last,
                frame.fenceValue);
    }

    return WaitForFenceValue(
        last,
        timeoutMs);
}

void ReleaseDx12Resources() {
    for (auto& frame :
         g_frames) {

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
        CloseHandle(
            g_fenceEvent);
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
    g_backBufferFormat =
        DXGI_FORMAT_UNKNOWN;
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

bool SameDevice(
    ID3D12CommandQueue* queue,
    ID3D12Device* swapDevice) {

    if (!queue ||
        !swapDevice) {

        return false;
    }

    ID3D12Device* queueDevice =
        nullptr;

    if (FAILED(
            queue->GetDevice(
                IID_PPV_ARGS(
                    &queueDevice))) ||
        !queueDevice) {

        return false;
    }

    IUnknown* a = nullptr;
    IUnknown* b = nullptr;

    queueDevice->QueryInterface(
        IID_PPV_ARGS(&a));

    swapDevice->QueryInterface(
        IID_PPV_ARGS(&b));

    const bool same =
        a && b && a == b;

    if (a) {
        a->Release();
    }

    if (b) {
        b->Release();
    }

    queueDevice->Release();

    return same;
}

bool IsGameSwapChain(
    IDXGISwapChain* swapChain,
    DXGI_SWAP_CHAIN_DESC& desc,
    ID3D12Device** outDevice) {

    if (!swapChain ||
        !outDevice) {

        return false;
    }

    *outDevice = nullptr;

    if (FAILED(
            swapChain->GetDesc(
                &desc)) ||
        !desc.OutputWindow) {

        return false;
    }

    DWORD pid = 0;

    GetWindowThreadProcessId(
        desc.OutputWindow,
        &pid);

    if (pid !=
        GetCurrentProcessId()) {

        return false;
    }

    RECT rc{};

    if (!GetClientRect(
            desc.OutputWindow,
            &rc)) {

        return false;
    }

    if (rc.right - rc.left <
            640 ||
        rc.bottom - rc.top <
            360) {

        return false;
    }

    ID3D12Device* device =
        nullptr;

    if (FAILED(
            swapChain->GetDevice(
                IID_PPV_ARGS(
                    &device))) ||
        !device) {

        return false;
    }

    ID3D12CommandQueue* queue =
        g_commandQueue.load(
            std::memory_order_acquire);

    if (!queue ||
        !SameDevice(
            queue,
            device)) {

        device->Release();
        return false;
    }

    *outDevice = device;
    return true;
}

bool InitializeImGuiForSwapChain(
    IDXGISwapChain* swapChain) {

    DXGI_SWAP_CHAIN_DESC desc{};
    ID3D12Device* device =
        nullptr;

    if (!IsGameSwapChain(
            swapChain,
            desc,
            &device)) {

        return false;
    }

    g_device = device;

    if (FAILED(
            swapChain->QueryInterface(
                IID_PPV_ARGS(
                    &g_swapChain3))) ||
        !g_swapChain3) {

        ReleaseDx12Resources();
        return false;
    }

    g_gameWindow =
        desc.OutputWindow;

    g_backBufferFormat =
        desc.BufferDesc.Format;

    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
    rtvDesc.Type =
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    rtvDesc.NumDescriptors =
        desc.BufferCount;

    if (FAILED(
            g_device
                ->CreateDescriptorHeap(
                    &rtvDesc,
                    IID_PPV_ARGS(
                        &g_rtvHeap)))) {

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
            g_device
                ->CreateDescriptorHeap(
                    &srvDesc,
                    IID_PPV_ARGS(
                        &g_srvHeap)))) {

        ReleaseDx12Resources();
        return false;
    }

    g_rtvDescriptorSize =
        g_device
            ->GetDescriptorHandleIncrementSize(
                D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    g_frames.resize(
        desc.BufferCount);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv =
        g_rtvHeap
            ->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0;
         i < desc.BufferCount;
         ++i) {

        FrameContext& frame =
            g_frames[i];

        if (FAILED(
                g_device
                    ->CreateCommandAllocator(
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

        g_device
            ->CreateRenderTargetView(
                frame.renderTarget,
                nullptr,
                frame.rtv);

        rtv.ptr +=
            g_rtvDescriptorSize;
    }

    if (FAILED(
            g_device
                ->CreateCommandList(
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
                IID_PPV_ARGS(
                    &g_fence)))) {

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

        g_imguiContextCreated =
            true;

        ImGuiIO& io =
            ImGui::GetIO();

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

        g_imguiWin32Initialized =
            true;

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
            g_srvHeap
                ->GetCPUDescriptorHandleForHeapStart(),
            g_srvHeap
                ->GetGPUDescriptorHandleForHeapStart())) {

        ShutdownDx12Backend();
        return false;
    }

    g_imguiDx12Initialized =
        true;

    g_overlayReady.store(
        true,
        std::memory_order_release);

    g_stage.store(
        static_cast<int>(
            OverlayStage::ImGuiReady),
        std::memory_order_release);

    g_uiLoaded = false;

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
        g_swapChain3
            ->GetCurrentBackBufferIndex();

    if (index >=
        g_frames.size()) {

        return g_originalPresent(
            swapChain,
            syncInterval,
            flags);
    }

    FrameContext& frame =
        g_frames[index];

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
            frame.allocator
                ->Reset()) ||
        FAILED(
            g_commandList
                ->Reset(
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

    g_commandList
        ->ResourceBarrier(
            1,
            &barrier);

    g_commandList
        ->OMSetRenderTargets(
            1,
            &frame.rtv,
            FALSE,
            nullptr);

    ID3D12DescriptorHeap* heaps[] = {
        g_srvHeap
    };

    g_commandList
        ->SetDescriptorHeaps(
            1,
            heaps);

    ImGui_ImplDX12_RenderDrawData(
        ImGui::GetDrawData(),
        g_commandList);

    std::swap(
        barrier.Transition.StateBefore,
        barrier.Transition.StateAfter);

    g_commandList
        ->ResourceBarrier(
            1,
            &barrier);

    if (FAILED(
            g_commandList->Close())) {

        return g_originalPresent(
            swapChain,
            syncInterval,
            flags);
    }

    ID3D12CommandList* lists[] = {
        g_commandList
    };

    ID3D12CommandQueue* queue =
        g_commandQueue.load(
            std::memory_order_acquire);

    if (!queue) {
        return g_originalPresent(
            swapChain,
            syncInterval,
            flags);
    }

    queue->ExecuteCommandLists(
        1,
        lists);

    const UINT64 fenceValue =
        ++g_nextFenceValue;

    if (SUCCEEDED(
            queue->Signal(
                g_fence,
                fenceValue))) {

        frame.fenceValue =
            fenceValue;
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
            OverlayStage::QueueCaptured),
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

    if (queue) {
        const D3D12_COMMAND_QUEUE_DESC desc =
            queue->GetDesc();

        if (desc.Type ==
            D3D12_COMMAND_LIST_TYPE_DIRECT) {

            ID3D12CommandQueue* expected =
                nullptr;

            queue->AddRef();

            if (g_commandQueue
                    .compare_exchange_strong(
                        expected,
                        queue,
                        std::memory_order_acq_rel)) {

                g_stage.store(
                    static_cast<int>(
                        OverlayStage::QueueCaptured),
                    std::memory_order_release);

            } else {

                queue->Release();
            }
        }
    }

    g_originalExecuteCommandLists(
        queue,
        numCommandLists,
        lists);
}

bool InstallRuntimeHooks() {
    IDXGIFactory4* factory =
        nullptr;

    ID3D12Device* device =
        nullptr;

    ID3D12CommandQueue* queue =
        nullptr;

    IDXGISwapChain* swapChain =
        nullptr;

    HWND dummyWindow =
        CreateWindowExW(
            0,
            L"STATIC",
            L"QProtocolDX12Probe",
            WS_OVERLAPPED,
            0,
            0,
            32,
            32,
            nullptr,
            nullptr,
            GetModuleHandleW(nullptr),
            nullptr);

    if (!dummyWindow) {
        return false;
    }

    bool ok = false;

    do {
        if (FAILED(
                CreateDXGIFactory1(
                    IID_PPV_ARGS(
                        &factory))) ||
            !factory) {
            break;
        }

        if (FAILED(
                D3D12CreateDevice(
                    nullptr,
                    D3D_FEATURE_LEVEL_11_0,
                    IID_PPV_ARGS(
                        &device))) ||
            !device) {
            break;
        }

        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Type =
            D3D12_COMMAND_LIST_TYPE_DIRECT;

        if (FAILED(
                device
                    ->CreateCommandQueue(
                        &queueDesc,
                        IID_PPV_ARGS(
                            &queue))) ||
            !queue) {
            break;
        }

        DXGI_SWAP_CHAIN_DESC desc{};
        desc.BufferDesc.Width = 32;
        desc.BufferDesc.Height = 32;
        desc.BufferDesc.Format =
            DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.BufferUsage =
            DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 2;
        desc.OutputWindow =
            dummyWindow;
        desc.Windowed = TRUE;
        desc.SwapEffect =
            DXGI_SWAP_EFFECT_FLIP_DISCARD;

        if (FAILED(
                factory
                    ->CreateSwapChain(
                        queue,
                        &desc,
                        &swapChain)) ||
            !swapChain) {
            break;
        }

        void** swapVtable =
            *reinterpret_cast<void***>(
                swapChain);

        void** queueVtable =
            *reinterpret_cast<void***>(
                queue);

        const MH_STATUS presentCreate =
            MH_CreateHook(
                swapVtable[8],
                reinterpret_cast<void*>(
                    &HookPresent),
                reinterpret_cast<void**>(
                    &g_originalPresent));

        const MH_STATUS resizeCreate =
            MH_CreateHook(
                swapVtable[13],
                reinterpret_cast<void*>(
                    &HookResizeBuffers),
                reinterpret_cast<void**>(
                    &g_originalResizeBuffers));

        const MH_STATUS executeCreate =
            MH_CreateHook(
                queueVtable[10],
                reinterpret_cast<void*>(
                    &HookExecuteCommandLists),
                reinterpret_cast<void**>(
                    &g_originalExecuteCommandLists));

        if (presentCreate != MH_OK ||
            resizeCreate != MH_OK ||
            executeCreate != MH_OK) {
            break;
        }

        if (MH_EnableHook(
                swapVtable[8]) != MH_OK ||
            MH_EnableHook(
                swapVtable[13]) != MH_OK ||
            MH_EnableHook(
                queueVtable[10]) != MH_OK) {
            break;
        }

        ok = true;

    } while (false);

    if (swapChain) {
        swapChain->Release();
    }

    if (queue) {
        queue->Release();
    }

    if (device) {
        device->Release();
    }

    if (factory) {
        factory->Release();
    }

    DestroyWindow(
        dummyWindow);

    if (!ok) {
        return false;
    }

    g_hooksInstalled = true;

    g_stage.store(
        static_cast<int>(
            OverlayStage::RuntimeHooksReady),
        std::memory_order_release);

    return true;
}

} // namespace

bool OverlayInitialize(
    const std::wstring& iniPath) {

    g_iniPath = iniPath;
    LoadOverlayToggleKeyFromIni();

    const MH_STATUS init =
        MH_Initialize();

    if (init != MH_OK &&
        init !=
            MH_ERROR_ALREADY_INITIALIZED) {

        g_stage.store(
            static_cast<int>(
                OverlayStage::InitFailed),
            std::memory_order_release);

        return false;
    }

    g_minHookInitialized =
        true;

    if (!InstallRuntimeHooks()) {

        // Fail-open means no partially-installed overlay hooks survive.
        MH_DisableHook(
            MH_ALL_HOOKS);

        MH_Uninitialize();

        g_minHookInitialized =
            false;

        g_stage.store(
            static_cast<int>(
                OverlayStage::InitFailed),
            std::memory_order_release);

        return false;
    }

    return true;
}

bool OverlayPump(
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

    const int toggleVk =
        g_overlayToggleVk.load(
            std::memory_order_acquire);

    const bool toggleDown =
        (GetAsyncKeyState(
             toggleVk) &
         0x8000) != 0;

    const bool toggleWasDown =
        g_overlayToggleWasDown.exchange(
            toggleDown,
            std::memory_order_acq_rel);

    bool toggled = false;

    if (!g_captureOverlayToggleKey.load(
            std::memory_order_acquire) &&
        toggleDown &&
        !toggleWasDown) {

        const bool next =
            !g_requestedVisible.load(
                std::memory_order_acquire);

        g_requestedVisible.store(
            next,
            std::memory_order_release);

        toggled = true;

        if (next) {
            ClipCursor(nullptr);
            g_uiLoaded = false;
        }
    }

    return toggled;
}

bool OverlayConsumeReloadRequest() {
    return g_reloadRequested.exchange(
        false,
        std::memory_order_acq_rel);
}

bool OverlayConsumeSpawnRequest(
    std::uint64_t& displayRid) {

    displayRid =
        g_spawnRequest.exchange(
            0,
            std::memory_order_acq_rel);

    return displayRid != 0;
}

void OverlayPublishRuntimeWeaponRids(
    const std::uint64_t* displayRids,
    std::size_t count) {

    std::lock_guard<std::mutex> lock(
        g_runtimeRidMutex);

    g_currentRuntimeRids.clear();

    if (displayRids && count) {
        g_currentRuntimeRids.assign(
            displayRids,
            displayRids + count);

        std::sort(
            g_currentRuntimeRids.begin(),
            g_currentRuntimeRids.end());

        g_currentRuntimeRids.erase(
            std::unique(
                g_currentRuntimeRids.begin(),
                g_currentRuntimeRids.end()),
            g_currentRuntimeRids.end());

        for (const std::uint64_t rid :
             g_currentRuntimeRids) {

            if (std::find(
                    g_sessionRuntimeRids.begin(),
                    g_sessionRuntimeRids.end(),
                    rid) ==
                g_sessionRuntimeRids.end()) {

                g_sessionRuntimeRids.push_back(
                    rid);
            }
        }

        std::sort(
            g_sessionRuntimeRids.begin(),
            g_sessionRuntimeRids.end());
    }
}

bool OverlayIsVisible() {
    return
        g_overlayReady.load(
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
        return "A19 DX12 bootstrap";

    case OverlayStage::RuntimeHooksReady:
        return "A19 runtime hooks ready; waiting for DIRECT queue";

    case OverlayStage::QueueCaptured:
        return "A19 DIRECT queue captured; waiting for game Present";

    case OverlayStage::ImGuiReady:
        return "A19 DX12 ImGui ready";

    case OverlayStage::InitFailed:
        return "A19 overlay initialization failed (A5 gameplay unaffected)";

    case OverlayStage::FenceTimeout:
        return "A19 overlay fence timeout (overlay disabled; A5 gameplay unaffected)";

    default:
        return "A19 overlay unknown state";
    }
}

void OverlayShutdown() {
    g_requestedVisible.store(
        false,
        std::memory_order_release);

    ShutdownImGui();

    ID3D12CommandQueue* queue =
        g_commandQueue.exchange(
            nullptr,
            std::memory_order_acq_rel);

    if (queue) {
        queue->Release();
    }

    if (g_minHookInitialized) {
        MH_DisableHook(
            MH_ALL_HOOKS);

        MH_Uninitialize();

        g_minHookInitialized =
            false;
    }

    g_hooksInstalled = false;
}

} // namespace qp
