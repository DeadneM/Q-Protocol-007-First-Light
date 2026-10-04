#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

#include "Overlay.h"

namespace qp {

constexpr std::uintptr_t kExpectedSizeOfImage = 0x06EC1000;
constexpr DWORD kExpectedTimeDateStamp = 0x6ABCDDDB;

// October 2026 executable mappings.
constexpr std::uintptr_t kPlayerResolverRva       = 0x0171DF40;
constexpr std::uintptr_t kPlayerRegistryHelperRva = 0x007D7770;
constexpr std::uintptr_t kPlayerRegistryGlobalRva = 0x069225A0;
constexpr std::uintptr_t kRuntimePlayerGlobalRva  = 0x064576F8;
constexpr std::uintptr_t kAmmoOwnerGlobalRva      = 0x064576E0;
constexpr std::uintptr_t kPlayerLoadoutVtableRva  = 0x02EDEB70;
constexpr std::uintptr_t kLicenseToKillRva        = 0x0191C344;

constexpr std::uintptr_t kItemEntryVtableRva      = 0x02ECB538;
constexpr std::uintptr_t kSpawnerVtableRva        = 0x02ECC800;
constexpr std::uintptr_t kNativeSpawnRva          = 0x016B6B10;
constexpr std::uintptr_t kNativeAmmoInsertRva     = 0x00116170;
constexpr std::uintptr_t kNativeAmmoNotifyRva     = 0x012A8FC0;
constexpr std::uintptr_t kGameplayHookRva         = 0x0194D891;

constexpr std::uintptr_t kAmmoNotifyContextOffset = 0x20AD0;
constexpr std::uintptr_t kAmmoInputVectorOffset   = 0x20B70;
constexpr std::uintptr_t kAmmoLockOffset          = 0x238E0;
constexpr std::uint32_t kAmmoNotifyEventId        = 0x1DE;

constexpr std::uintptr_t kGraphScanBegin = 0x2C000000;
constexpr std::uintptr_t kGraphScanEnd   = 0x30000000;

constexpr std::uint64_t kDonorDisplayRid = 0x016886A4B599391CULL;

constexpr BYTE kLtkOff[6] = {0x32, 0xD2, 0x4C, 0x8B, 0x15, 0x53};
constexpr BYTE kLtkOn [6] = {0xB2, 0x01, 0x4C, 0x8B, 0x15, 0x53};

constexpr BYTE kGameplayHookPreimage[13] = {
    0x48, 0x81, 0xC4, 0x00, 0x01, 0x00, 0x00,
    0x41, 0x5F,
    0x41, 0x5E,
    0x41, 0x5D
};

struct PlayerContext {
    void* loadout = nullptr;
    std::uint32_t playerId = 0;

    explicit operator bool() const {
        return loadout != nullptr && playerId != 0;
    }
};

struct WeaponGraph {
    std::uintptr_t itemEntry = 0;
    std::uintptr_t spawner = 0;
};

struct ActiveWeapon {
    bool active = false;
    bool seenBusy = false;
    std::uint64_t requestedRid = 0;
    std::uintptr_t donorItem = 0;
    std::uintptr_t donorSpawner = 0;
    std::uint64_t originalDonorRid = 0;
    std::uint64_t originalDonorTemplate = 0;
    ULONGLONG setupAt = 0;
};

struct AmmoProfile {
    std::uint32_t qPistol = 0;
    std::uint32_t smg = 0;
    std::uint32_t assaultRifle = 0;
    std::uint32_t shotgun = 0;
    std::uint32_t sniper = 0;
    std::uint32_t heavyPistol = 0;
};

#pragma pack(push, 1)
struct AddFirearmAmmunitionInput {
    std::uint32_t playerId = 0;
    std::uint32_t amount = 0;
    std::uint32_t firearmClass = 0;
};
#pragma pack(pop)

static_assert(sizeof(AddFirearmAmmunitionInput) == 12);

struct PendingAmmoRequest {
    AmmoProfile profile{};
    std::uint32_t playerId = 0;
    const char* source = nullptr;
};

enum class LtkState {
    Off,
    On,
    Unknown
};

enum class BeginWeaponResult {
    Started,
    DonorBusy,
    Failed
};

using ResolveLocalPlayerFn = void(__fastcall*)(std::uint32_t, std::uint64_t*);
using LookupPlayerFn = void*(__fastcall*)(std::uint64_t*, void*);
using NativeSpawnFn = void(__fastcall*)(void*);
using NativeAmmoInsertFn = void(__fastcall*)(void*, const AddFirearmAmmunitionInput*);
using NativeAmmoNotifyFn = void(__fastcall*)(void*, std::uint32_t, void*);

extern "C" void QpGameplayHook();

HMODULE g_module = nullptr;
HANDLE g_log = INVALID_HANDLE_VALUE;
std::uintptr_t g_exeBase = 0;
std::atomic<bool> g_running{true};

NativeSpawnFn g_nativeSpawn = nullptr;
bool g_gameplayHookInstalled = false;

std::atomic<std::uintptr_t> g_pendingSpawner{0};
std::atomic<bool> g_spawnTriggered{false};
std::atomic<bool> g_spawnException{false};
std::atomic<ULONGLONG> g_spawnTriggeredAt{0};

PendingAmmoRequest g_pendingAmmo{};
std::atomic<bool> g_ammoPending{false};

std::unordered_map<std::uint64_t, WeaponGraph> g_graphs;
bool g_graphIndexBuilt = false;
std::vector<std::uint64_t> g_seenRuntimeGraphRids;
std::deque<std::uint64_t> g_weaponQueue;
ActiveWeapon g_activeWeapon{};
ULONGLONG g_nextWeaponAllowedAt = 0;
constexpr ULONGLONG kWeaponInterRequestDelayMs = 500;

std::uint64_t g_qpistolModeA = 0;
std::uint64_t g_qpistolModeB = 0;
bool g_qpistolNextB = true;
std::uint64_t g_hotkeyWeapons[8]{};

struct LoadoutProfile {
    std::uint64_t qPistol = 0;
    std::uint64_t oneHanded = 0;
    std::uint64_t twoHanded = 0;
};

LoadoutProfile g_manualLoadout{};
LoadoutProfile g_autoLoadout{};
AmmoProfile g_manualAmmo{};
AmmoProfile g_autoAmmo{};
bool g_autoEnabled = false;

std::wstring ModuleDirectory(HMODULE module) {
    wchar_t path[MAX_PATH]{};
    const DWORD len = GetModuleFileNameW(module, path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return L".";
    }

    wchar_t* slash = wcsrchr(path, L'\\');
    if (!slash) {
        return L".";
    }
    *slash = L'\0';
    return path;
}

void Log(const char* fmt, ...) {
    if (g_log == INVALID_HANDLE_VALUE) {
        return;
    }

    char buffer[2048]{};
    va_list args;
    va_start(args, fmt);
    const int written = vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, fmt, args);
    va_end(args);

    if (written <= 0) {
        return;
    }

    DWORD out = 0;
    WriteFile(g_log, buffer, static_cast<DWORD>(strlen(buffer)), &out, nullptr);
    WriteFile(g_log, "\r\n", 2, &out, nullptr);
    FlushFileBuffers(g_log);
}

template <typename T>
bool SafeRead(std::uintptr_t address, T& out) {
    SIZE_T bytes = 0;
    return ReadProcessMemory(
               GetCurrentProcess(),
               reinterpret_cast<LPCVOID>(address),
               &out,
               sizeof(T),
               &bytes) != FALSE &&
           bytes == sizeof(T);
}

template <typename T>
bool SafeWrite(std::uintptr_t address, const T& value) {
    SIZE_T bytes = 0;
    return WriteProcessMemory(
               GetCurrentProcess(),
               reinterpret_cast<LPVOID>(address),
               &value,
               sizeof(T),
               &bytes) != FALSE &&
           bytes == sizeof(T);
}

bool WriteCodeBytes(std::uintptr_t address, const BYTE* bytes, SIZE_T size) {
    DWORD oldProtect = 0;
    if (!VirtualProtect(
            reinterpret_cast<void*>(address),
            size,
            PAGE_EXECUTE_READWRITE,
            &oldProtect)) {
        return false;
    }

    memcpy(reinterpret_cast<void*>(address), bytes, size);
    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(address), size);

    DWORD ignored = 0;
    VirtualProtect(reinterpret_cast<void*>(address), size, oldProtect, &ignored);
    return true;
}

bool ValidateTargetExecutable() {
    g_exeBase = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    if (!g_exeBase) {
        Log("[ERROR] GetModuleHandleW(nullptr) failed.");
        return false;
    }

    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(g_exeBase);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        Log("[ERROR] Invalid DOS header.");
        return false;
    }

    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(
        g_exeBase + static_cast<std::uintptr_t>(dos->e_lfanew));

    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        Log("[ERROR] Invalid NT header.");
        return false;
    }

    Log("EXE base = 0x%p", reinterpret_cast<void*>(g_exeBase));
    Log("EXE TimeDateStamp = 0x%08lX", nt->FileHeader.TimeDateStamp);
    Log("EXE SizeOfImage = 0x%08lX", nt->OptionalHeader.SizeOfImage);

    if (nt->FileHeader.TimeDateStamp != kExpectedTimeDateStamp ||
        nt->OptionalHeader.SizeOfImage != kExpectedSizeOfImage) {
        Log("[ERROR] Unsupported 007FirstLight.exe build. No game memory will be modified.");
        return false;
    }

    g_nativeSpawn = reinterpret_cast<NativeSpawnFn>(g_exeBase + kNativeSpawnRva);
    return true;
}

bool ResolveNativeLoadout(void** outLoadout) {
    if (!outLoadout || !g_exeBase) {
        return false;
    }

    *outLoadout = nullptr;

    auto resolve = reinterpret_cast<ResolveLocalPlayerFn>(g_exeBase + kPlayerResolverRva);
    auto lookup = reinterpret_cast<LookupPlayerFn>(g_exeBase + kPlayerRegistryHelperRva);

    std::uint64_t handle[2]{};

    __try {
        resolve(0, handle);
        if (handle[0] == 0) {
            return false;
        }

        *outLoadout = lookup(
            handle,
            reinterpret_cast<void*>(g_exeBase + kPlayerRegistryGlobalRva));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        *outLoadout = nullptr;
        return false;
    }

    return *outLoadout != nullptr;
}

PlayerContext ResolvePlayer() {
    PlayerContext result{};

    void* loadout = nullptr;
    if (!ResolveNativeLoadout(&loadout)) {
        return result;
    }

    std::uintptr_t vtable = 0;
    if (!SafeRead(reinterpret_cast<std::uintptr_t>(loadout), vtable)) {
        return result;
    }

    if (vtable != g_exeBase + kPlayerLoadoutVtableRva) {
        return result;
    }

    std::uintptr_t root = 0;
    std::uintptr_t node = 0;
    std::uint32_t playerId = 0;

    if (!SafeRead(g_exeBase + kRuntimePlayerGlobalRva, root) || !root) {
        return result;
    }
    if (!SafeRead(root + 0x10, node) || !node) {
        return result;
    }
    if (!SafeRead(node + 0x30, playerId) || playerId == 0) {
        return result;
    }

    result.loadout = loadout;
    result.playerId = playerId;
    return result;
}

LtkState ReadLicenseToKillState() {
    BYTE current[6]{};
    SIZE_T bytes = 0;
    if (!ReadProcessMemory(
            GetCurrentProcess(),
            reinterpret_cast<LPCVOID>(g_exeBase + kLicenseToKillRva),
            current,
            sizeof(current),
            &bytes) ||
        bytes != sizeof(current)) {
        return LtkState::Unknown;
    }

    if (memcmp(current, kLtkOff, sizeof(kLtkOff)) == 0) {
        return LtkState::Off;
    }
    if (memcmp(current, kLtkOn, sizeof(kLtkOn)) == 0) {
        return LtkState::On;
    }
    return LtkState::Unknown;
}

bool ToggleLicenseToKill() {
    const auto site = g_exeBase + kLicenseToKillRva;
    const LtkState state = ReadLicenseToKillState();

    if (state == LtkState::Off) {
        if (!WriteCodeBytes(site, kLtkOn, sizeof(kLtkOn))) {
            Log("[ERROR] F1 License To Kill: write failed.");
            return false;
        }
        Log("F1 License To Kill = ON");
        return true;
    }

    if (state == LtkState::On) {
        if (!WriteCodeBytes(site, kLtkOff, sizeof(kLtkOff))) {
            Log("[ERROR] F1 License To Kill: write failed.");
            return false;
        }
        Log("F1 License To Kill = OFF");
        return true;
    }

    Log("[ERROR] F1 License To Kill: preimage mismatch.");
    return false;
}

std::uint64_t RotateRid(std::uint64_t value) {
    return (value << 32) | (value >> 32);
}

bool ParseRid(const std::wstring& text, std::uint64_t& internalRid) {
    if (text.empty() || _wcsicmp(text.c_str(), L"None") == 0) {
        internalRid = 0;
        return true;
    }

    wchar_t* end = nullptr;
    const unsigned long long display = wcstoull(text.c_str(), &end, 16);
    if (!end || *end != L'\0') {
        return false;
    }

    internalRid = RotateRid(static_cast<std::uint64_t>(display));
    return internalRid != 0;
}

std::wstring IniRead(
    const std::wstring& iniPath,
    const wchar_t* section,
    const wchar_t* key) {

    wchar_t buffer[256]{};
    GetPrivateProfileStringW(
        section,
        key,
        L"",
        buffer,
        static_cast<DWORD>(std::size(buffer)),
        iniPath.c_str());
    return buffer;
}

std::uint32_t ReadAmmoAmount(
    const std::wstring& iniPath,
    const wchar_t* section,
    const wchar_t* key) {

    const UINT value = GetPrivateProfileIntW(
        section,
        key,
        0,
        iniPath.c_str());

    return std::min<std::uint32_t>(
        static_cast<std::uint32_t>(value),
        100000u);
}

bool ResolveConfiguredRid(
    const std::wstring& iniPath,
    const wchar_t* section,
    const wchar_t* key,
    std::uint64_t& internalRid) {

    const std::wstring value = IniRead(iniPath, section, key);
    if (value.empty()) {
        internalRid = 0;
        return false;
    }

    if (ParseRid(value, internalRid)) {
        return true;
    }

    const std::wstring catalog = IniRead(iniPath, L"WeaponCatalog", value.c_str());
    if (catalog.empty()) {
        internalRid = 0;
        return false;
    }

    return ParseRid(catalog, internalRid);
}

void LogRid(const char* label, std::uint64_t internalRid) {
    if (!internalRid) {
        Log("%s = None", label);
        return;
    }
    const std::uint64_t display = RotateRid(internalRid);
    Log("%s = %016llX", label, static_cast<unsigned long long>(display));
}

void LoadConfig(const std::wstring& iniPath) {
    ResolveConfiguredRid(iniPath, L"Hotkey_F4", L"ModeA", g_qpistolModeA);
    ResolveConfiguredRid(iniPath, L"Hotkey_F4", L"ModeB", g_qpistolModeB);

    for (int i = 0; i < 8; ++i) {
        wchar_t section[32]{};
        swprintf_s(section, L"Hotkey_F%d", i + 5);
        ResolveConfiguredRid(iniPath, section, L"Weapon", g_hotkeyWeapons[i]);

    }

    ResolveConfiguredRid(iniPath, L"ManualLoadout", L"QPistol", g_manualLoadout.qPistol);
    ResolveConfiguredRid(iniPath, L"ManualLoadout", L"OneHanded", g_manualLoadout.oneHanded);
    ResolveConfiguredRid(iniPath, L"ManualLoadout", L"TwoHanded", g_manualLoadout.twoHanded);

    ResolveConfiguredRid(iniPath, L"AutoLoadout", L"QPistol", g_autoLoadout.qPistol);
    ResolveConfiguredRid(iniPath, L"AutoLoadout", L"OneHanded", g_autoLoadout.oneHanded);
    ResolveConfiguredRid(iniPath, L"AutoLoadout", L"TwoHanded", g_autoLoadout.twoHanded);

    g_autoEnabled =
        GetPrivateProfileIntW(L"Auto", L"Enabled", 1, iniPath.c_str()) != 0;

    g_manualAmmo.qPistol = ReadAmmoAmount(iniPath, L"ManualAmmo", L"QPistol");
    g_manualAmmo.smg = ReadAmmoAmount(iniPath, L"ManualAmmo", L"SMG");
    g_manualAmmo.assaultRifle = ReadAmmoAmount(iniPath, L"ManualAmmo", L"AssaultRifle");
    g_manualAmmo.shotgun = ReadAmmoAmount(iniPath, L"ManualAmmo", L"Shotgun");
    g_manualAmmo.sniper = ReadAmmoAmount(iniPath, L"ManualAmmo", L"Sniper");
    g_manualAmmo.heavyPistol = ReadAmmoAmount(iniPath, L"ManualAmmo", L"HeavyPistol");

    g_autoAmmo.qPistol = ReadAmmoAmount(iniPath, L"AutoAmmo", L"QPistol");
    g_autoAmmo.smg = ReadAmmoAmount(iniPath, L"AutoAmmo", L"SMG");
    g_autoAmmo.assaultRifle = ReadAmmoAmount(iniPath, L"AutoAmmo", L"AssaultRifle");
    g_autoAmmo.shotgun = ReadAmmoAmount(iniPath, L"AutoAmmo", L"Shotgun");
    g_autoAmmo.sniper = ReadAmmoAmount(iniPath, L"AutoAmmo", L"Sniper");
    g_autoAmmo.heavyPistol = ReadAmmoAmount(iniPath, L"AutoAmmo", L"HeavyPistol");

    LogRid("F4 Q-Pistol ModeA", g_qpistolModeA);
    LogRid("F4 Q-Pistol ModeB", g_qpistolModeB);
    for (int i = 0; i < 8; ++i) {
        char label[32]{};
        sprintf_s(label, "F%d weapon", i + 5);
        LogRid(label, g_hotkeyWeapons[i]);
    }

    LogRid("ManualLoadout QPistol", g_manualLoadout.qPistol);
    LogRid("ManualLoadout OneHanded", g_manualLoadout.oneHanded);
    LogRid("ManualLoadout TwoHanded", g_manualLoadout.twoHanded);

    Log("AUTO Enabled = %d", g_autoEnabled ? 1 : 0);
    LogRid("AutoLoadout QPistol", g_autoLoadout.qPistol);
    LogRid("AutoLoadout OneHanded", g_autoLoadout.oneHanded);
    LogRid("AutoLoadout TwoHanded", g_autoLoadout.twoHanded);

    Log("ManualAmmo QPistol = %u", g_manualAmmo.qPistol);
    Log("ManualAmmo SMG/MachinePistol = %u", g_manualAmmo.smg);
    Log("ManualAmmo AssaultRifle = %u", g_manualAmmo.assaultRifle);
    Log("ManualAmmo Shotgun = %u", g_manualAmmo.shotgun);
    Log("ManualAmmo Sniper/Marksman = %u", g_manualAmmo.sniper);
    Log("ManualAmmo HeavyPistol50Cal = %u", g_manualAmmo.heavyPistol);

    Log("AutoAmmo QPistol = %u", g_autoAmmo.qPistol);
    Log("AutoAmmo SMG/MachinePistol = %u", g_autoAmmo.smg);
    Log("AutoAmmo AssaultRifle = %u", g_autoAmmo.assaultRifle);
    Log("AutoAmmo Shotgun = %u", g_autoAmmo.shotgun);
    Log("AutoAmmo Sniper/Marksman = %u", g_autoAmmo.sniper);
    Log("AutoAmmo HeavyPistol50Cal = %u", g_autoAmmo.heavyPistol);
}

bool IsReadableProtection(DWORD protect) {
    if ((protect & PAGE_GUARD) || (protect & PAGE_NOACCESS)) {
        return false;
    }

    const DWORD base = protect & 0xFF;
    return base == PAGE_READONLY ||
           base == PAGE_READWRITE ||
           base == PAGE_WRITECOPY ||
           base == PAGE_EXECUTE_READ ||
           base == PAGE_EXECUTE_READWRITE ||
           base == PAGE_EXECUTE_WRITECOPY;
}

bool BuildGraphIndex() {
    g_graphs.clear();

    const std::uintptr_t itemVtable = g_exeBase + kItemEntryVtableRva;
    const std::uintptr_t spawnerVtable = g_exeBase + kSpawnerVtableRva;

    std::unordered_map<std::uintptr_t, std::uint64_t> itemToRid;
    std::vector<std::uintptr_t> spawners;
    std::vector<BYTE> buffer(0x10000);

    std::uintptr_t cursor = kGraphScanBegin;
    while (cursor < kGraphScanEnd) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (VirtualQuery(reinterpret_cast<LPCVOID>(cursor), &mbi, sizeof(mbi)) != sizeof(mbi)) {
            cursor += 0x1000;
            continue;
        }

        const std::uintptr_t regionBase =
            reinterpret_cast<std::uintptr_t>(mbi.BaseAddress);
        const std::uintptr_t regionEnd =
            regionBase + static_cast<std::uintptr_t>(mbi.RegionSize);

        if (mbi.State == MEM_COMMIT && IsReadableProtection(mbi.Protect)) {
            std::uintptr_t chunk = regionBase;
            while (chunk < regionEnd && chunk < kGraphScanEnd) {
                const SIZE_T wanted = static_cast<SIZE_T>(
                    std::min<std::uintptr_t>(
                        buffer.size(),
                        std::min<std::uintptr_t>(regionEnd, kGraphScanEnd) - chunk));

                SIZE_T got = 0;
                if (ReadProcessMemory(
                        GetCurrentProcess(),
                        reinterpret_cast<LPCVOID>(chunk),
                        buffer.data(),
                        wanted,
                        &got) &&
                    got >= sizeof(std::uintptr_t)) {

                    for (SIZE_T off = 0; off + sizeof(std::uintptr_t) <= got; off += 8) {
                        std::uintptr_t value = 0;
                        memcpy(&value, buffer.data() + off, sizeof(value));

                        const std::uintptr_t candidate = chunk + off;

                        if (value == itemVtable) {
                            std::uint64_t rid = 0;
                            if (SafeRead(candidate + 0x120, rid) && rid != 0) {
                                itemToRid.emplace(candidate, rid);
                                auto& graph = g_graphs[rid];
                                if (!graph.itemEntry) {
                                    graph.itemEntry = candidate;
                                }
                            }
                        } else if (value == spawnerVtable) {
                            spawners.push_back(candidate);
                        }
                    }
                }

                chunk += wanted ? wanted : 0x1000;
            }
        }

        const std::uintptr_t next = regionEnd > cursor ? regionEnd : cursor + 0x1000;
        cursor = next;
    }

    for (const std::uintptr_t spawner : spawners) {
        std::uintptr_t begin = 0;
        std::uintptr_t end = 0;

        if (!SafeRead(spawner + 0x18, begin) ||
            !SafeRead(spawner + 0x20, end) ||
            !begin ||
            end <= begin) {
            continue;
        }

        const std::uintptr_t bytes = end - begin;
        if (bytes > 0x10000 || (bytes & 0xF) != 0) {
            continue;
        }

        for (std::uintptr_t entry = begin; entry + 8 <= end; entry += 0x10) {
            std::uintptr_t item = 0;
            if (!SafeRead(entry, item)) {
                continue;
            }

            const auto it = itemToRid.find(item);
            if (it == itemToRid.end()) {
                continue;
            }

            auto& graph = g_graphs[it->second];
            if (!graph.spawner) {
                graph.spawner = spawner;
            }
        }
    }

    for (auto it = g_graphs.begin(); it != g_graphs.end();) {
        if (!it->second.itemEntry || !it->second.spawner) {
            it = g_graphs.erase(it);
        } else {
            ++it;
        }
    }

    g_graphIndexBuilt = true;

    std::vector<std::uint64_t> displayRids;
    displayRids.reserve(g_graphs.size());

    for (const auto& [internalRid, graph] : g_graphs) {
        (void)graph;
        displayRids.push_back(RotateRid(internalRid));
    }

    std::sort(displayRids.begin(), displayRids.end());

    OverlayPublishRuntimeWeaponRids(
        displayRids.empty() ? nullptr : displayRids.data(),
        displayRids.size());

    Log(
        "Weapon graph index built: %zu ItemEntry/Spawner RID graph(s), %zu spawner candidate(s).",
        g_graphs.size(),
        spawners.size());

    for (const std::uint64_t displayRid : displayRids) {
        if (std::find(
                g_seenRuntimeGraphRids.begin(),
                g_seenRuntimeGraphRids.end(),
                displayRid) ==
            g_seenRuntimeGraphRids.end()) {

            g_seenRuntimeGraphRids.push_back(displayRid);

            Log(
                "DISCOVERY runtime graph RID=%016llX",
                static_cast<unsigned long long>(displayRid));
        }
    }

    return !g_graphs.empty();
}

bool ValidateGraph(std::uint64_t rid, const WeaponGraph& graph) {
    if (!graph.itemEntry || !graph.spawner) {
        return false;
    }

    std::uintptr_t itemVtable = 0;
    std::uintptr_t spawnerVtable = 0;
    std::uint64_t currentRid = 0;

    return SafeRead(graph.itemEntry, itemVtable) &&
           SafeRead(graph.spawner, spawnerVtable) &&
           SafeRead(graph.itemEntry + 0x120, currentRid) &&
           itemVtable == g_exeBase + kItemEntryVtableRva &&
           spawnerVtable == g_exeBase + kSpawnerVtableRva &&
           currentRid == rid;
}

bool ResolveGraph(std::uint64_t rid, WeaponGraph& graph) {
    auto it = g_graphs.find(rid);
    if (it != g_graphs.end() && ValidateGraph(rid, it->second)) {
        graph = it->second;
        return true;
    }

    g_graphIndexBuilt = false;
    if (!BuildGraphIndex()) {
        return false;
    }

    it = g_graphs.find(rid);
    if (it == g_graphs.end() || !ValidateGraph(rid, it->second)) {
        return false;
    }

    graph = it->second;
    return true;
}

bool IsSpawnerIdle(std::uintptr_t spawner, bool& idle) {
    std::uint64_t first = 0;
    std::uint64_t second = 0;
    BYTE state = 0xFF;

    if (!SafeRead(spawner + 0x38, first) ||
        !SafeRead(spawner + 0x40, second) ||
        !SafeRead(spawner + 0x50, state)) {
        return false;
    }

    idle = (first == second && state == 0);
    return true;
}

void RestoreActiveDonor(const char* reason) {
    if (!g_activeWeapon.active) {
        return;
    }

    const bool ridOk = SafeWrite(
        g_activeWeapon.donorItem + 0x120,
        g_activeWeapon.originalDonorRid);
    const bool templateOk = SafeWrite(
        g_activeWeapon.donorItem + 0x128,
        g_activeWeapon.originalDonorTemplate);

    Log(
        "GiveWeapon donor restored (%s): RID=%s template=%s",
        reason,
        ridOk ? "OK" : "FAILED",
        templateOk ? "OK" : "FAILED");

    g_activeWeapon = {};
    g_pendingSpawner.store(0, std::memory_order_release);
    g_spawnTriggered.store(false, std::memory_order_release);
    g_spawnException.store(false, std::memory_order_release);
    g_spawnTriggeredAt.store(0, std::memory_order_release);
}

BeginWeaponResult BeginWeaponRequest(std::uint64_t requestedRid) {
    const std::uint64_t donorRid = RotateRid(kDonorDisplayRid);

    WeaponGraph donor{};
    WeaponGraph source{};

    if (!ResolveGraph(donorRid, donor)) {
        Log("[ERROR] GiveWeapon: donor graph NOT FOUND.");
        return BeginWeaponResult::Failed;
    }

    if (!ResolveGraph(requestedRid, source)) {
        Log(
            "[ERROR] GiveWeapon: source graph NOT FOUND for RID %016llX.",
            static_cast<unsigned long long>(RotateRid(requestedRid)));
        return BeginWeaponResult::Failed;
    }

    bool donorIdle = false;
    if (!IsSpawnerIdle(donor.spawner, donorIdle)) {
        Log("[ERROR] GiveWeapon: donor spawner state unreadable.");
        return BeginWeaponResult::Failed;
    }

    if (!donorIdle) {
        return BeginWeaponResult::DonorBusy;
    }

    std::uint64_t donorCurrentRid = 0;
    std::uint64_t donorTemplate = 0;
    std::uint64_t sourceCurrentRid = 0;
    std::uint64_t sourceTemplate = 0;

    if (!SafeRead(donor.itemEntry + 0x120, donorCurrentRid) ||
        !SafeRead(donor.itemEntry + 0x128, donorTemplate) ||
        !SafeRead(source.itemEntry + 0x120, sourceCurrentRid) ||
        !SafeRead(source.itemEntry + 0x128, sourceTemplate) ||
        donorCurrentRid != donorRid ||
        sourceCurrentRid != requestedRid ||
        sourceTemplate == 0) {
        Log("[ERROR] GiveWeapon: descriptor read/validation FAILED.");
        return BeginWeaponResult::Failed;
    }

    if (!SafeWrite(donor.itemEntry + 0x120, requestedRid) ||
        !SafeWrite(donor.itemEntry + 0x128, sourceTemplate)) {
        SafeWrite(donor.itemEntry + 0x120, donorCurrentRid);
        SafeWrite(donor.itemEntry + 0x128, donorTemplate);
        Log("[ERROR] GiveWeapon: temporary descriptor write FAILED.");
        return BeginWeaponResult::Failed;
    }

    std::uint64_t verifyRid = 0;
    std::uint64_t verifyTemplate = 0;
    if (!SafeRead(donor.itemEntry + 0x120, verifyRid) ||
        !SafeRead(donor.itemEntry + 0x128, verifyTemplate) ||
        verifyRid != requestedRid ||
        verifyTemplate != sourceTemplate) {
        SafeWrite(donor.itemEntry + 0x120, donorCurrentRid);
        SafeWrite(donor.itemEntry + 0x128, donorTemplate);
        Log("[ERROR] GiveWeapon: temporary descriptor verification FAILED.");
        return BeginWeaponResult::Failed;
    }

    g_activeWeapon.active = true;
    g_activeWeapon.seenBusy = false;
    g_activeWeapon.requestedRid = requestedRid;
    g_activeWeapon.donorItem = donor.itemEntry;
    g_activeWeapon.donorSpawner = donor.spawner;
    g_activeWeapon.originalDonorRid = donorCurrentRid;
    g_activeWeapon.originalDonorTemplate = donorTemplate;
    g_activeWeapon.setupAt = GetTickCount64();

    g_spawnTriggered.store(false, std::memory_order_release);
    g_spawnException.store(false, std::memory_order_release);
    g_spawnTriggeredAt.store(0, std::memory_order_release);
    g_pendingSpawner.store(donor.spawner, std::memory_order_release);

    Log(
        "GiveWeapon prepared RID=%016llX donorItem=0x%p donorSpawner=0x%p",
        static_cast<unsigned long long>(RotateRid(requestedRid)),
        reinterpret_cast<void*>(donor.itemEntry),
        reinterpret_cast<void*>(donor.spawner));

    return BeginWeaponResult::Started;
}

void ProcessWeaponQueue() {
    if (g_activeWeapon.active) {
        if (g_spawnException.load(std::memory_order_acquire)) {
            RestoreActiveDonor("native trigger exception");
            return;
        }

        const ULONGLONG now = GetTickCount64();
        const bool triggered = g_spawnTriggered.load(std::memory_order_acquire);

        if (!triggered) {
            if (now - g_activeWeapon.setupAt > 2000) {
                Log("[ERROR] GiveWeapon: gameplay trigger timeout.");
                RestoreActiveDonor("trigger timeout");
            }
            return;
        }

        bool idle = false;
        if (!IsSpawnerIdle(g_activeWeapon.donorSpawner, idle)) {
            Log("[ERROR] GiveWeapon: donor spawner became unreadable.");
            RestoreActiveDonor("spawner unreadable");
            return;
        }

        if (!idle) {
            g_activeWeapon.seenBusy = true;
        }

        const ULONGLONG triggeredAt =
            g_spawnTriggeredAt.load(std::memory_order_acquire);
        const ULONGLONG elapsed = triggeredAt ? now - triggeredAt : 0;

        if (idle && (g_activeWeapon.seenBusy || elapsed >= 250)) {
            Log(
                "GiveWeapon COMPLETE RID=%016llX",
                static_cast<unsigned long long>(
                    RotateRid(g_activeWeapon.requestedRid)));
            g_nextWeaponAllowedAt = now + kWeaponInterRequestDelayMs;
            Log(
                "GiveWeapon queue cooldown = %llu ms",
                static_cast<unsigned long long>(kWeaponInterRequestDelayMs));
            RestoreActiveDonor("complete");
            return;
        }

        if (elapsed > 5000) {
            Log(
                "[ERROR] GiveWeapon TIMEOUT RID=%016llX",
                static_cast<unsigned long long>(
                    RotateRid(g_activeWeapon.requestedRid)));
            RestoreActiveDonor("completion timeout");
        }

        return;
    }

    if (g_weaponQueue.empty()) {
        return;
    }

    const ULONGLONG now = GetTickCount64();
    if (g_nextWeaponAllowedAt && now < g_nextWeaponAllowedAt) {
        return;
    }

    const std::uint64_t rid = g_weaponQueue.front();
    const BeginWeaponResult result = BeginWeaponRequest(rid);

    if (result == BeginWeaponResult::Started ||
        result == BeginWeaponResult::Failed) {
        g_weaponQueue.pop_front();
    }
}

bool QueueWeapon(std::uint64_t rid, const char* source) {
    if (!rid) {
        Log("%s ignored: no weapon configured.", source);
        return false;
    }

    if (g_weaponQueue.size() >= 16) {
        Log("%s ignored: weapon queue full.", source);
        return false;
    }

    g_weaponQueue.push_back(rid);
    Log(
        "%s queued RID=%016llX",
        source,
        static_cast<unsigned long long>(RotateRid(rid)));
    return true;
}

std::size_t QueueLoadout(const LoadoutProfile& profile, const char* source) {
    std::size_t queued = 0;

    struct Entry {
        const char* role;
        std::uint64_t rid;
    };

    const Entry entries[] = {
        {"QPistol", profile.qPistol},
        {"OneHanded", profile.oneHanded},
        {"TwoHanded", profile.twoHanded},
    };

    for (const auto& entry : entries) {
        if (!entry.rid) {
            continue;
        }

        char label[80]{};
        sprintf_s(label, "%s %s", source, entry.role);
        if (QueueWeapon(entry.rid, label)) {
            ++queued;
        }
    }

    return queued;
}


bool QueueAmmoProfile(
    const AmmoProfile& profile,
    std::uint32_t playerId,
    const char* source) {

    if (!playerId) {
        Log("%s ignored: invalid playerId.", source);
        return false;
    }

    if (g_ammoPending.load(std::memory_order_acquire)) {
        Log("%s ignored: ammo request already pending.", source);
        return false;
    }

    g_pendingAmmo.profile = profile;
    g_pendingAmmo.playerId = playerId;
    g_pendingAmmo.source = source;
    g_ammoPending.store(true, std::memory_order_release);

    Log("%s queued for playerId=%u.", source, playerId);
    return true;
}

bool RuntimePlayerMatches(std::uint32_t playerId) {
    std::uintptr_t root = 0;
    std::uintptr_t node = 0;
    std::uint32_t currentPlayerId = 0;

    return playerId != 0 &&
           SafeRead(g_exeBase + kRuntimePlayerGlobalRva, root) &&
           root != 0 &&
           SafeRead(root + 0x10, node) &&
           node != 0 &&
           SafeRead(node + 0x30, currentPlayerId) &&
           currentPlayerId == playerId;
}

bool PublishAmmoProfileGameplayThread(const PendingAmmoRequest& request) {
    if (!RuntimePlayerMatches(request.playerId)) {
        Log(
            "[ERROR] %s AddAmmo aborted: player identity changed.",
            request.source ? request.source : "Ammo");
        return false;
    }

    std::uintptr_t owner = 0;
    if (!SafeRead(g_exeBase + kAmmoOwnerGlobalRva, owner) || !owner) {
        Log(
            "[ERROR] %s AddAmmo: native ammo owner unavailable.",
            request.source ? request.source : "Ammo");
        return false;
    }

    const auto insert = reinterpret_cast<NativeAmmoInsertFn>(
        g_exeBase + kNativeAmmoInsertRva);
    const auto notify = reinterpret_cast<NativeAmmoNotifyFn>(
        g_exeBase + kNativeAmmoNotifyRva);

    struct ClassAmount {
        const char* name;
        std::uint32_t firearmClass;
        std::uint32_t amount;
    };

    const ClassAmount entries[] = {
        {"Q-Pistol", 0, request.profile.qPistol},
        {"SMG/MachinePistol", 1, request.profile.smg},
        {"AssaultRifle", 2, request.profile.assaultRifle},
        {"Shotgun", 5, request.profile.shotgun},
        {"Sniper/Marksman", 6, request.profile.sniper},
        {"HeavyPistol50Cal", 7, request.profile.heavyPistol},
    };

    auto* criticalSection = reinterpret_cast<LPCRITICAL_SECTION>(
        owner + kAmmoLockOffset);

    bool locked = false;
    bool nativeOk = true;
    std::size_t published = 0;

    __try {
        EnterCriticalSection(criticalSection);
        locked = true;

        for (const auto& entry : entries) {
            if (!entry.amount) {
                continue;
            }

            const AddFirearmAmmunitionInput input{
                request.playerId,
                entry.amount,
                entry.firearmClass,
            };

            insert(
                reinterpret_cast<void*>(owner + kAmmoInputVectorOffset),
                &input);

            ++published;
            Log(
                "%s AddAmmo class=%u (%s) amount=+%u",
                request.source ? request.source : "Ammo",
                entry.firearmClass,
                entry.name,
                entry.amount);
        }

        if (published) {
            notify(
                reinterpret_cast<void*>(owner),
                kAmmoNotifyEventId,
                reinterpret_cast<void*>(owner + kAmmoNotifyContextOffset));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        nativeOk = false;
        Log(
            "[ERROR] %s AddAmmo: native publication exception.",
            request.source ? request.source : "Ammo");
    }

    if (locked) {
        __try {
            LeaveCriticalSection(criticalSection);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            nativeOk = false;
            Log(
                "[ERROR] %s AddAmmo: unlock exception.",
                request.source ? request.source : "Ammo");
        }
    }

    if (!nativeOk) {
        return false;
    }

    if (!published) {
        Log(
            "%s: all configured ammo amounts are zero.",
            request.source ? request.source : "Ammo");
        return true;
    }

    Log(
        "%s PUBLISHED through native ammo event 0x%X (%zu class(es)).",
        request.source ? request.source : "Ammo",
        kAmmoNotifyEventId,
        published);
    return true;
}

bool InstallGameplayHook() {
    const std::uintptr_t site = g_exeBase + kGameplayHookRva;

    BYTE current[sizeof(kGameplayHookPreimage)]{};
    SIZE_T bytes = 0;
    if (!ReadProcessMemory(
            GetCurrentProcess(),
            reinterpret_cast<LPCVOID>(site),
            current,
            sizeof(current),
            &bytes) ||
        bytes != sizeof(current) ||
        memcmp(current, kGameplayHookPreimage, sizeof(current)) != 0) {
        Log("[ERROR] Gameplay hook preimage mismatch.");
        return false;
    }

    BYTE patch[13] = {
        0x49, 0xBB, // mov r11, imm64
        0,0,0,0,0,0,0,0,
        0x41, 0xFF, 0xE3 // jmp r11
    };

    const std::uint64_t target =
        reinterpret_cast<std::uint64_t>(&QpGameplayHook);
    memcpy(&patch[2], &target, sizeof(target));

    if (!WriteCodeBytes(site, patch, sizeof(patch))) {
        Log("[ERROR] Gameplay hook install write failed.");
        return false;
    }

    g_gameplayHookInstalled = true;
    Log("Gameplay hook installed at EXE+0x%llX.", static_cast<unsigned long long>(kGameplayHookRva));
    return true;
}

bool KeyPressedEdge(int vk, SHORT& previous) {
    const SHORT now = GetAsyncKeyState(vk);
    const bool downNow = (now & 0x8000) != 0;
    const bool downBefore = (previous & 0x8000) != 0;
    previous = now;
    return downNow && !downBefore;
}

void ResetWeaponRuntime(const char* reason) {
    if (g_activeWeapon.active) {
        RestoreActiveDonor(reason);
    }
    g_weaponQueue.clear();
    g_nextWeaponAllowedAt = 0;
    g_ammoPending.store(false, std::memory_order_release);
    g_graphs.clear();
    g_graphIndexBuilt = false;
    OverlayPublishRuntimeWeaponRids(nullptr, 0);
    Log("Weapon runtime reset: %s", reason);
}

DWORD WINAPI WorkerThread(LPVOID) {
    const std::wstring dir = ModuleDirectory(g_module);
    const std::wstring logPath = dir + L"\\QProtocol.log";
    const std::wstring iniPath = dir + L"\\QProtocol.ini";

    g_log = CreateFileW(
        logPath.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    Log("Q Protocol Fresh Core A18");
    Log("Scope: validated A5 gameplay core + A18 remappable overlay key.");

    if (!ValidateTargetExecutable()) {
        Log("Fresh Core A8 disabled because executable validation failed.");
        return 0;
    }

    Log("Target executable accepted.");
    LoadConfig(iniPath);

    if (!InstallGameplayHook()) {
        Log("[ERROR] Fresh Core A8 disabled: gameplay hook unavailable.");
        return 0;
    }

    if (OverlayInitialize(iniPath)) {
        Log("Overlay bootstrap started in fail-open mode. Insert = open/close when DX12 ImGui is ready.");
        Log("Overlay status: %s", OverlayStatus());
    } else {
        Log("[ERROR] Overlay bootstrap failed. Gameplay core remains active.");
    }

    Log("F1 = License To Kill toggle.");
    Log("F2 = ManualAmmo through native AddFirearmAmmunitionToPlayer.");
    Log("F3 = ManualLoadout through shared GiveWeapon.");
    Log("F4 = Q-Pistol swap through shared GiveWeapon.");
    Log("F5-F12 = configured weapons through shared GiveWeapon.");
    Log("AUTO = same shared GiveWeapon + AddAmmo primitives using AutoLoadout/AutoAmmo.");
    Log("Overlay = Insert, INI-backed, Save/Reload/Reset Defaults.");

    PlayerContext previousPlayer{};
    bool previousReady = false;
    bool autoDone = false;
    std::string lastOverlayStatus = OverlayStatus();

    SHORT keyPrevious[13]{};

    while (g_running.load(std::memory_order_acquire)) {
        const PlayerContext player = ResolvePlayer();
        const bool ready = static_cast<bool>(player);

        const bool playerIdentityChanged =
            ready && previousReady &&
            player.playerId != previousPlayer.playerId;

        const bool loadoutPointerChanged =
            ready && previousReady &&
            player.playerId == previousPlayer.playerId &&
            player.loadout != previousPlayer.loadout;

        if (ready != previousReady || playerIdentityChanged) {
            if (ready) {
                Log(
                    "PLAYER READY loadout=0x%p playerId=%u",
                    player.loadout,
                    player.playerId);
            } else if (previousReady) {
                Log("PLAYER NOT READY");
            }

            ResetWeaponRuntime(
                ready ? "player identity changed/ready" : "player not ready");

            autoDone = false;
            previousPlayer = player;
            previousReady = ready;
        } else if (loadoutPointerChanged) {
            Log(
                "PLAYER LOADOUT REFRESH loadout=0x%p playerId=%u - preserving weapon queue.",
                player.loadout,
                player.playerId);

            g_graphs.clear();
            g_graphIndexBuilt = false;

            previousPlayer = player;
        }

        if (ready && g_autoEnabled && !autoDone) {
            const std::size_t autoWeapons =
                QueueLoadout(g_autoLoadout, "AUTO Loadout");
            const bool autoAmmoQueued =
                QueueAmmoProfile(g_autoAmmo, player.playerId, "AUTO Ammo");

            autoDone = true;

            Log(
                "AUTO committed once for current READY cycle: weapons=%zu ammo=%s.",
                autoWeapons,
                autoAmmoQueued ? "queued" : "not queued");
        }

        OverlayPump(
            ready,
            autoDone,
            g_weaponQueue.size() + (g_activeWeapon.active ? 1u : 0u),
            g_qpistolNextB);

        const char* overlayStatus = OverlayStatus();
        if (overlayStatus && lastOverlayStatus != overlayStatus) {
            Log("Overlay status: %s", overlayStatus);
            lastOverlayStatus = overlayStatus;
        }

        if (OverlayConsumeReloadRequest()) {
            LoadConfig(iniPath);
            Log("Overlay requested INI reload: runtime config refreshed.");
        }

        std::uint64_t overlayDisplayRid = 0;
        if (OverlayConsumeSpawnRequest(overlayDisplayRid)) {
            if (!ready) {
                Log("Overlay Spawn Weapon ignored: player not ready.");
            } else {
                const std::uint64_t internalRid = RotateRid(overlayDisplayRid);
                if (QueueWeapon(internalRid, "Overlay Spawn Weapon")) {
                    Log(
                        "Overlay Spawn Weapon queued RID=%016llX.",
                        static_cast<unsigned long long>(overlayDisplayRid));
                } else {
                    Log(
                        "[ERROR] Overlay Spawn Weapon rejected RID=%016llX.",
                        static_cast<unsigned long long>(overlayDisplayRid));
                }
            }
        }

        const bool overlayVisible = OverlayIsVisible();

        if (!overlayVisible && KeyPressedEdge(VK_F1, keyPrevious[1])) {
            ToggleLicenseToKill();
        }

        if (!overlayVisible && KeyPressedEdge(VK_F2, keyPrevious[2])) {
            if (!ready) {
                Log("F2 ignored: player not ready.");
            } else {
                QueueAmmoProfile(
                    g_manualAmmo,
                    player.playerId,
                    "F2 ManualAmmo");
            }
        }

        if (!overlayVisible && KeyPressedEdge(VK_F3, keyPrevious[3])) {
            if (!ready) {
                Log("F3 ignored: player not ready.");
            } else {
                const std::size_t queued =
                    QueueLoadout(g_manualLoadout, "F3 ManualLoadout");

                Log(
                    "F3 ManualLoadout queued %zu/3 role(s).",
                    queued);
            }
        }

        if (!overlayVisible && KeyPressedEdge(VK_F4, keyPrevious[4])) {
            if (!ready) {
                Log("F4 ignored: player not ready.");
            } else {
                const std::uint64_t rid =
                    g_qpistolNextB ? g_qpistolModeB : g_qpistolModeA;
                if (QueueWeapon(rid, "F4 Q-Pistol")) {
                    g_qpistolNextB = !g_qpistolNextB;
                }
            }
        }

        for (int i = 0; i < 8; ++i) {
            const int vk = VK_F5 + i;
            if (!overlayVisible && KeyPressedEdge(vk, keyPrevious[5 + i])) {
                char source[16]{};
                sprintf_s(source, "F%d", i + 5);

                if (!ready) {
                    Log("%s ignored: player not ready.", source);
                } else {
                    QueueWeapon(g_hotkeyWeapons[i], source);
                }
            }
        }

        if (ready) {
            ProcessWeaponQueue();
        } else if (g_activeWeapon.active || !g_weaponQueue.empty()) {
            ResetWeaponRuntime("player unavailable");
        }

        Sleep(16);
    }

    if (g_activeWeapon.active) {
        RestoreActiveDonor("DLL shutdown");
    }

    OverlayShutdown();
    return 0;
}

} // namespace qp

extern "C" void QpGameplayTick() {
    if (qp::g_ammoPending.load(std::memory_order_acquire)) {
        const qp::PendingAmmoRequest request = qp::g_pendingAmmo;
        qp::g_ammoPending.store(false, std::memory_order_release);
        qp::PublishAmmoProfileGameplayThread(request);
    }

    const std::uintptr_t spawner =
        qp::g_pendingSpawner.exchange(0, std::memory_order_acq_rel);

    if (!spawner || !qp::g_nativeSpawn) {
        return;
    }

    __try {
        qp::g_nativeSpawn(reinterpret_cast<void*>(spawner));
        qp::g_spawnTriggeredAt.store(GetTickCount64(), std::memory_order_release);
        qp::g_spawnTriggered.store(true, std::memory_order_release);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        qp::g_spawnException.store(true, std::memory_order_release);
    }
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        qp::g_module = module;
        DisableThreadLibraryCalls(module);

        HANDLE thread = CreateThread(nullptr, 0, qp::WorkerThread, nullptr, 0, nullptr);
        if (thread) {
            CloseHandle(thread);
        }
    } else if (reason == DLL_PROCESS_DETACH) {
        qp::g_running.store(false, std::memory_order_release);
    }

    return TRUE;
}
