#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#include <mutex>
#include "runtime/NativePlayer.h"
#include "runtime/NativeInventory.h"
#include "runtime/GameplayMailbox.h"

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
#include "Updates.h"

namespace qp {

constexpr std::uintptr_t kExpectedSizeOfImage = 0x06D31000;
constexpr DWORD kExpectedTimeDateStamp = 0x6AE0DF3C;

// A28 TEST: Steam build 26252352, SHA-256 6e63df4d2891ea085a71ef9a6f4d24b9db52144879945e8cc1a2e8106fabd3f1.
constexpr std::uintptr_t kPlayerResolverRva       = 0x0171D2E0;
constexpr std::uintptr_t kPlayerRegistryHelperRva = 0x007D6E80;
constexpr std::uintptr_t kPlayerRegistryGlobalRva = 0x06792438;
constexpr std::uintptr_t kRuntimePlayerGlobalRva  = 0x06452BF8;
constexpr std::uintptr_t kAmmoOwnerGlobalRva      = 0x06452BE0;
constexpr std::uintptr_t kPlayerLoadoutVtableRva  = 0x02EDD278;
constexpr std::uintptr_t kLicenseToKillRva         = 0x0191B694;
constexpr std::uintptr_t kLicenseToKillNativeOnRva = 0x0191B6FB;
constexpr std::uintptr_t kLtkObserveHookRva         = 0x0191B6B8;
constexpr std::uintptr_t kLtkObserveTrueRva         = 0x0191B6C7;
constexpr std::uintptr_t kLtkObserveFalseRva        = 0x0191B743;

constexpr std::uintptr_t kItemEntryVtableRva      = 0x02ECA0A8;
constexpr std::uintptr_t kSpawnerVtableRva        = 0x02ECA270;
constexpr std::uintptr_t kNativeSpawnRva          = 0x016B5E90;
constexpr std::uintptr_t kNativeAmmoInsertRva     = 0x001157B0;
constexpr std::uintptr_t kNativeAmmoNotifyRva     = 0x012A8470;
constexpr std::uintptr_t kGameplayHookRva         = 0x0194CBD1;

constexpr std::uintptr_t kAmmoNotifyContextOffset = 0x20AD0;
constexpr std::uintptr_t kAmmoInputVectorOffset   = 0x20B70;
constexpr std::uintptr_t kAmmoLockOffset          = 0x238E0;
constexpr std::uint32_t kAmmoNotifyEventId        = 0x1DE;

constexpr std::uintptr_t kGraphScanBegin = 0x2C000000;
constexpr std::uintptr_t kGraphScanEnd   = 0x30000000;

constexpr BYTE kLtkNaturalFalse[2] = {0x32, 0xD2};
constexpr BYTE kLtkForceTrue[2]     = {0xB2, 0x01};
constexpr BYTE kLtkNativeTrue[2]    = {0xB2, 0x01};
constexpr BYTE kLtkForceFalse[2]    = {0x32, 0xD2};

constexpr BYTE kLtkObserveHookPreimage[15] = {
    0x49, 0x8B, 0xBB, 0xF0, 0x06, 0x00, 0x00,
    0x4C, 0x23, 0x04, 0xC7,
    0x84, 0xD2,
    0x74, 0x7C
};

constexpr BYTE kGameplayHookPreimage[13] = {
    0x48, 0x81, 0xC4, 0x00, 0x01, 0x00, 0x00,
    0x41, 0x5F,
    0x41, 0x5E,
    0x41, 0x5D
};

struct PlayerContext {
    void* loadout = nullptr;
    void* character = nullptr;
    std::uint32_t playerId = 0;
    runtime::PlayerSnapshot snapshot{};
    const char* reason = "no gameplay observation";
    explicit operator bool() const { return snapshot.Fresh(GetTickCount64()); }
};
struct SpawnResult {
    bool triggered = false, exception = false, valid = false, accepted = false, pending = false;
    bool invoked = false;
    runtime::SpawnToken token{};
    unsigned int state = 0xFFFFFFFF;
    ULONGLONG at = 0;
};

struct WeaponGraph {
    std::uintptr_t itemEntry = 0;
    std::uintptr_t spawner = 0;
};

struct ActiveWeapon {
    bool active = false;
    bool packageOwned = false;
    bool observationLogged = false;
    bool dispatchDeferredLogged = false;
    runtime::Ticket ticket{};
    SpawnResult result{};
    runtime::EntityReference created{};
    runtime::Ticket pickupTicket{};
    bool pickupSubmitted = false;
    ULONGLONG pickupAcknowledgedAt = 0;
    std::uint64_t requestedRid = 0;
    std::uintptr_t spawner = 0;
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
struct PendingPickupRequest {
    runtime::NativeItem item{};
    std::uint32_t playerId = 0;
};
struct NativePickupInput { std::uint32_t playerId = 0, itemId = 0, mode = 0; };
static_assert(sizeof(NativePickupInput) == 12);

enum class BeginWeaponResult {
    Started,
    SpawnerBusy,
    Failed
};

using ResolveLocalPlayerFn = void(__fastcall*)(std::uint32_t, std::uint64_t*);
using LookupPlayerFn = void*(__fastcall*)(std::uint64_t*, void*);
using NativeSpawnFn = void(__fastcall*)(void*);
using NativeAmmoInsertFn = void(__fastcall*)(void*, const AddFirearmAmmunitionInput*);
using NativeAmmoNotifyFn = void(__fastcall*)(void*, std::uint32_t, void*);

extern "C" void QpGameplayHook();
extern "C" void QpLtkObserveHook();

extern "C" volatile LONG g_qpLtkObservedPacked = 0;
extern "C" std::uintptr_t g_qpLtkObserveTrueTarget = 0;
extern "C" std::uintptr_t g_qpLtkObserveFalseTarget = 0;

HMODULE g_module = nullptr;
HANDLE g_log = INVALID_HANDLE_VALUE;
std::uintptr_t g_exeBase = 0;
std::atomic<bool> g_running{true};

NativeSpawnFn g_nativeSpawn = nullptr;
bool g_ltkObserveHookInstalled = false;

struct SpawnPayload { std::uint64_t rid = 0; WeaponGraph graph{}; };
runtime::GameplayMailbox<SpawnPayload, SpawnResult> g_spawnMailbox;
runtime::GameplayMailbox<PendingAmmoRequest, bool> g_ammoMailbox;
runtime::GameplayMailbox<PendingPickupRequest, bool> g_pickupMailbox;
std::mutex g_playerMutex;
runtime::PlayableSession g_playerSession;
PlayerContext g_publishedPlayer{}, g_workerPlayer{}, g_tickPlayer{};
std::atomic_flag g_tickBusy = ATOMIC_FLAG_INIT;
runtime::Ticket g_lastAmmoTicket{}, g_autoAmmoTicket{};
const char* g_lastAmmoSource = "Ammo";

std::unordered_map<std::uint64_t, WeaponGraph> g_graphs;
bool g_graphIndexBuilt = false;
std::uintptr_t g_graphScanPlayerLoadout = 0; // Worker thread only; runtime discovery anchor
std::vector<std::uint64_t> g_seenRuntimeGraphRids;
std::deque<std::uint64_t> g_weaponQueue;
// A24: bounded burst protection for manual F4-F12 and Debug Spawn only.
// Worker-thread state; AUTO/F3 package transaction and native spawn unchanged.
std::deque<ULONGLONG> g_recentUnitaryWeaponRequests;
constexpr std::size_t kUnitaryPendingLimit = 4;
constexpr std::size_t kUnitaryBurstLimit = 12;
constexpr ULONGLONG kUnitaryBurstWindowMs = 30000;
ActiveWeapon g_activeWeapon{};
ULONGLONG g_nextWeaponAllowedAt = 0;
unsigned int g_unitaryRetryCount = 0;
ULONGLONG g_unitaryRetryAt = 0;
constexpr ULONGLONG kWeaponInterRequestDelayMs = 500;
constexpr ULONGLONG kWeaponRetryDelayMs = 250;
constexpr unsigned int kWeaponMaxRetries = 5;

std::uint64_t g_qpistolModeA = 0;
std::uint64_t g_qpistolModeB = 0;
bool g_qpistolNextB = true;
std::uint64_t g_hotkeyWeapons[8]{};

struct LoadoutProfile {
    std::uint64_t qPistol = 0;
    std::uint64_t oneHanded = 0;
    std::uint64_t twoHanded = 0;
};

enum class LoadoutSource {
    None,
    Manual,
    Auto
};

enum class LoadoutPhase {
    QPistol,
    OneHanded,
    TwoHanded
};

struct LoadoutPackage {
    bool active = false;
    bool justCompleted = false;
    bool justFailed = false;
    bool hadFailure = false;
    LoadoutSource source = LoadoutSource::None;
    LoadoutProfile profile{};
    std::uint32_t playerId = 0;
    LoadoutPhase phase = LoadoutPhase::QPistol;
    unsigned int retries = 0;
    ULONGLONG retryAt = 0;
};

LoadoutPackage g_loadoutPackage{};

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

// A28 compatibility gate: verify native code and relocated type links before any hook.
// This is a static-map check, not proof of runtime gameplay acceptance.
#include "NativePlayerGuards.inl"
#include "NativeDeliveryGuards.inl"

bool ValidateNovemberNativeMap() {
    auto code = [](const char* name, std::uintptr_t rva, const BYTE* expected, SIZE_T size) {
        BYTE current[64]{};
        SIZE_T got = 0;
        if (size > sizeof(current) || rva >= kExpectedSizeOfImage ||
            size > kExpectedSizeOfImage - rva ||
            !ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(g_exeBase + rva),
                               current, size, &got) || got != size ||
            memcmp(current, expected, size) != 0) {
            Log("[ERROR] A28 native-map mismatch: %s at EXE+0x%llX. No hooks installed.",
                name, static_cast<unsigned long long>(rva));
            return false;
        }
        return true;
    };
    const BYTE expected0[] = {0x40, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x45, 0x33, 0xC9, 0x48, 0x8B, 0xDA, 0x4C, 0x39, 0x0D, 0x3D, 0x6E, 0xD3, 0x04, 0xBA, 0xAC, 0x01, 0x00, 0x00, 0x0F, 0x84, 0xEA, 0x00, 0x00, 0x00, 0x48, 0x8B};
    if (!code("kPlayerResolverRva", 0x0171D2E0, expected0, sizeof(expected0))) return false;
    const BYTE expected1[] = {0x48, 0x83, 0xEC, 0x08, 0x44, 0x8B, 0x41, 0x08, 0x4C, 0x8B, 0xDA, 0x48, 0x8B, 0x05, 0xF6, 0x07, 0x52, 0x05, 0xBA, 0xAC, 0x01, 0x00, 0x00, 0x46, 0x8B, 0x0C, 0x80, 0x90, 0x65, 0x48, 0x8B, 0x04};
    if (!code("kPlayerRegistryHelperRva", 0x007D6E80, expected1, sizeof(expected1))) return false;
    const BYTE expected2[] = {0x40, 0x55, 0x56, 0x41, 0x55, 0x48, 0x8D, 0x6C, 0x24, 0xB9, 0x48, 0x81, 0xEC, 0xF0, 0x00, 0x00, 0x00, 0x8B, 0x05, 0x81, 0x47, 0x05, 0x05, 0x33, 0xF6, 0x4C, 0x8B, 0xE9, 0x89, 0x45, 0x87, 0x8B};
    if (!code("kNativeSpawnRva", 0x016B5E90, expected2, sizeof(expected2))) return false;
    const BYTE expected3[] = {0x48, 0x89, 0x5C, 0x24, 0x18, 0x55, 0x56, 0x57, 0x41, 0x54, 0x41, 0x56, 0x48, 0x83, 0xEC, 0x30, 0x4C, 0x8B, 0x51, 0x08, 0x48, 0x8B, 0xEA, 0x4C, 0x8B, 0x61, 0x10, 0x48, 0x8B, 0xD9, 0x49, 0xBB};
    if (!code("kNativeAmmoInsertRva", 0x001157B0, expected3, sizeof(expected3))) return false;
    const BYTE expected4[] = {0xB8, 0x01, 0x00, 0x00, 0x00, 0x44, 0x8B, 0xD2, 0x44, 0x8B, 0xC8, 0x87, 0x81, 0x08, 0x39, 0x02, 0x00, 0x85, 0xC0, 0x74, 0x44, 0x8B, 0x81, 0x08, 0x39, 0x02, 0x00, 0x85, 0xC0, 0x74, 0x2B, 0x90};
    if (!code("kNativeAmmoNotifyRva", 0x012A8470, expected4, sizeof(expected4))) return false;
    const BYTE expected5[] = {0x32, 0xD2, 0x4C, 0x8B, 0x15, 0x03, 0x76, 0xB3, 0x04, 0x44, 0x0F, 0xB7, 0xCB, 0x0F, 0xB7, 0xC3, 0x41, 0x8B, 0xC9, 0x48, 0xC1, 0xE8, 0x06, 0x41, 0xB8, 0x01, 0x00, 0x00, 0x00, 0x49, 0xD3, 0xE0};
    if (!code("kLicenseToKillRva", 0x0191B694, expected5, sizeof(expected5))) return false;
    const BYTE expected6[] = {0xB2, 0x01, 0xEB, 0x97, 0x48, 0x8B, 0x87, 0x08, 0x07, 0x00, 0x00, 0x4E, 0x85, 0x1C, 0xD0, 0x74, 0x88, 0x41, 0x83, 0x79, 0x0C, 0x00, 0x77, 0xE8, 0x48, 0x8B, 0x87, 0xF0, 0x06, 0x00, 0x00, 0x4E};
    if (!code("kLicenseToKillNativeOnRva", 0x0191B6FB, expected6, sizeof(expected6))) return false;
    const BYTE expected7[] = {0x49, 0x8B, 0xBB, 0xF0, 0x06, 0x00, 0x00, 0x4C, 0x23, 0x04, 0xC7, 0x84, 0xD2, 0x74, 0x7C, 0x4D, 0x85, 0xC0, 0x0F, 0x84, 0x29, 0x02, 0x00, 0x00, 0x41, 0xB8, 0x4A, 0x00, 0x00, 0x00, 0x0F, 0xB7};
    if (!code("kLtkObserveHookRva", 0x0191B6B8, expected7, sizeof(expected7))) return false;
    const BYTE expected8[] = {0x4D, 0x85, 0xC0, 0x0F, 0x84, 0x29, 0x02, 0x00, 0x00, 0x41, 0xB8, 0x4A, 0x00, 0x00, 0x00, 0x0F, 0xB7, 0xD3, 0x49, 0x8B, 0xCA, 0xE8, 0x6F, 0x46, 0x82, 0xFF, 0x48, 0x8B, 0x8C, 0x24, 0x20, 0x01};
    if (!code("kLtkObserveTrueRva", 0x0191B6C7, expected8, sizeof(expected8))) return false;
    const BYTE expected9[] = {0x4D, 0x85, 0xC0, 0x0F, 0x85, 0xAD, 0x01, 0x00, 0x00, 0x0F, 0xB7, 0xCB, 0x41, 0xB8, 0x01, 0x00, 0x00, 0x00, 0x8B, 0xD1, 0x49, 0xD3, 0xE0, 0x48, 0xC1, 0xEA, 0x06, 0x4C, 0x85, 0x04, 0xD7, 0x0F};
    if (!code("kLtkObserveFalseRva", 0x0191B743, expected9, sizeof(expected9))) return false;
    const BYTE expected10[] = {0x48, 0x81, 0xC4, 0x00, 0x01, 0x00, 0x00, 0x41, 0x5F, 0x41, 0x5E, 0x41, 0x5D, 0x41, 0x5C, 0x5F, 0x5E, 0x5B, 0xC3};
    if (!code("kGameplayHookRva", 0x0194CBD1, expected10, sizeof(expected10))) return false;
    const BYTE expected11[] = {0x48, 0x8D, 0x8D, 0x70, 0x0B, 0x02, 0x00, 0x48, 0x8D, 0x54, 0x24, 0x30, 0xE8, 0xDD, 0xFC, 0xB3, 0xFE, 0x4C, 0x8D, 0x85, 0xD0, 0x0A, 0x02, 0x00, 0xBA, 0xDE, 0x01, 0x00, 0x00, 0x48, 0x8B, 0xCD, 0xE8, 0x89, 0x29, 0xCD, 0xFF};
    if (!code("AmmoPublicationCallsite", 0x015D5AC2, expected11, sizeof(expected11))) return false;
    const std::uintptr_t links[][2] = {
        {0x03740FB8, 0x06792438},
        {0x02EDD5D8, 0x0171D2E0},
        {0x02EDD278, 0x0171BFA0},
        {0x02ECA0A8, 0x016B8810},
        {0x02ECA270, 0x016B6A30},
    };
    for (const auto& link : links) {
        std::uintptr_t value = 0;
        if (!SafeRead(g_exeBase + link[0], value) || value != g_exeBase + link[1]) {
            Log("[ERROR] A28 type-map mismatch at EXE+0x%llX. No hooks installed.",
                static_cast<unsigned long long>(link[0]));
            return false;
        }
    }
    Log("A28 native-map code/type checks PASS; runtime validation pending.");
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

    if (!ValidateNovemberNativeMap() || !ValidateNativePlayerMap() || !ValidateNativeDeliveryMap()) return false;

    g_nativeSpawn = reinterpret_cast<NativeSpawnFn>(g_exeBase + kNativeSpawnRva);
    return true;
}

struct NativeMemoryReader {
    template<class T> bool operator()(std::uintptr_t address, T& value) const { return SafeRead(address, value); }
};
bool ReadNativeThreadMask(std::uint32_t& mask) {
    __try {
        const auto tls = static_cast<std::uintptr_t>(__readgsqword(0x58));
        std::uintptr_t slot = 0;
        return tls && SafeRead(tls, slot) && slot && SafeRead(slot + 0x1AC, mask) && mask != 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
runtime::NativePlayer ReadCurrentNativePlayer() {
    NativeMemoryReader read;
    std::uint32_t mask = 0;
    runtime::NativePlayer native{};
    if (ReadNativeThreadMask(mask)) native = runtime::ReadNativePlayer(read, g_exeBase, mask);
    else native.reason = "native observer TLS unavailable";
    return native;
}
PlayerContext ObserveWorkerPlayer() {
    const auto native = ReadCurrentNativePlayer();
    PlayerContext player{};
    player.character = reinterpret_cast<void*>(native.character);
    player.loadout = reinterpret_cast<void*>(native.loadout);
    player.playerId = native.observation.identity.character;
    player.reason = native.reason;
    bool changed = false;
    {
        std::lock_guard<std::mutex> lock(g_playerMutex);
        const auto before = g_playerSession.Snapshot();
        player.snapshot = g_playerSession.Observe(native.observation, GetTickCount64());
        changed = player.snapshot.epoch != before.epoch;
        g_publishedPlayer = player;
    }
    if (changed) { g_spawnMailbox.Cancel(); g_ammoMailbox.Cancel(); g_pickupMailbox.Cancel(); }
    return player;
}
PlayerContext ResolvePlayer() {
    return ObserveWorkerPlayer();
}
PlayerContext ReadDispatchPlayer() {
    const auto native = ReadCurrentNativePlayer();
    std::lock_guard<std::mutex> lock(g_playerMutex);
    auto player = g_publishedPlayer;
    player.snapshot = runtime::RevalidateDispatch(player.snapshot, native.observation, GetTickCount64());
    if (!player.snapshot.playable) player.reason = native.reason;
    return player;
}
runtime::AutoStage AutoStage() {
    std::lock_guard<std::mutex> lock(g_playerMutex);
    return g_playerSession.Stage();
}
bool BeginAuto(const PlayerContext& player) {
    std::lock_guard<std::mutex> lock(g_playerMutex);
    return g_playerSession.BeginAuto(player.snapshot.session, g_autoEnabled, GetTickCount64());
}

bool InstallLicenseToKillObserveHook() {
    const std::uintptr_t site = g_exeBase + kLtkObserveHookRva;

    BYTE current[sizeof(kLtkObserveHookPreimage)]{};
    SIZE_T bytes = 0;

    if (!ReadProcessMemory(
            GetCurrentProcess(),
            reinterpret_cast<LPCVOID>(site),
            current,
            sizeof(current),
            &bytes) ||
        bytes != sizeof(current) ||
        memcmp(
            current,
            kLtkObserveHookPreimage,
            sizeof(current)) != 0) {

        Log("[ERROR] LTK observe hook preimage mismatch.");
        return false;
    }

    g_qpLtkObserveTrueTarget =
        g_exeBase + kLtkObserveTrueRva;
    g_qpLtkObserveFalseTarget =
        g_exeBase + kLtkObserveFalseRva;

    InterlockedExchange(
        &g_qpLtkObservedPacked,
        0);

    BYTE patch[15] = {
        0xFF, 0x25, 0x00, 0x00, 0x00, 0x00,
        0, 0, 0, 0, 0, 0, 0, 0,
        0x90
    };

    const std::uint64_t target =
        reinterpret_cast<std::uint64_t>(
            &QpLtkObserveHook);

    memcpy(
        &patch[6],
        &target,
        sizeof(target));

    if (!WriteCodeBytes(
            site,
            patch,
            sizeof(patch))) {

        Log("[ERROR] LTK observe hook install write failed.");
        return false;
    }

    g_ltkObserveHookInstalled = true;
    Log(
        "LTK native-state observe hook installed at EXE+0x%llX.",
        static_cast<unsigned long long>(
            kLtkObserveHookRva));
    return true;
}

bool RemoveLicenseToKillObserveHook() {
    if (!g_ltkObserveHookInstalled) {
        return true;
    }

    const std::uintptr_t site =
        g_exeBase + kLtkObserveHookRva;

    const bool restored =
        WriteCodeBytes(
            site,
            kLtkObserveHookPreimage,
            sizeof(kLtkObserveHookPreimage));

    if (restored) {
        g_ltkObserveHookInstalled = false;
        InterlockedExchange(
            &g_qpLtkObservedPacked,
            0);
    }

    return restored;
}

bool ReadCode2(std::uintptr_t address, BYTE (&out)[2]) {
    SIZE_T bytes = 0;
    return ReadProcessMemory(
               GetCurrentProcess(),
               reinterpret_cast<LPCVOID>(address),
               out,
               sizeof(out),
               &bytes) &&
           bytes == sizeof(out);
}

bool ApplyLicenseToKillForce(bool enabled) {
    const auto falseSite = g_exeBase + kLicenseToKillRva;
    const auto trueSite = g_exeBase + kLicenseToKillNativeOnRva;

    BYTE oldFalse[2]{};
    BYTE oldTrue[2]{};
    if (!ReadCode2(falseSite, oldFalse) ||
        !ReadCode2(trueSite, oldTrue)) {
        Log("[ERROR] F1 License To Kill: unable to read native sites.");
        return false;
    }

    const BYTE* targetFalse =
        enabled ? kLtkForceTrue : kLtkNaturalFalse;
    const BYTE* targetTrue =
        enabled ? kLtkNativeTrue : kLtkForceFalse;

    // Patch the path that preserves the requested state first, then the other.
    if (!WriteCodeBytes(trueSite, targetTrue, 2) ||
        !WriteCodeBytes(falseSite, targetFalse, 2)) {

        WriteCodeBytes(falseSite, oldFalse, 2);
        WriteCodeBytes(trueSite, oldTrue, 2);
        Log("[ERROR] F1 License To Kill: force write failed; rolled back.");
        return false;
    }

    return true;
}

bool RestoreLicenseToKillNatural() {
    const auto falseSite = g_exeBase + kLicenseToKillRva;
    const auto trueSite = g_exeBase + kLicenseToKillNativeOnRva;

    const bool a = WriteCodeBytes(
        falseSite,
        kLtkNaturalFalse,
        sizeof(kLtkNaturalFalse));
    const bool b = WriteCodeBytes(
        trueSite,
        kLtkNativeTrue,
        sizeof(kLtkNativeTrue));

    return a && b;
}

bool ToggleLicenseToKill() {
    const LONG observed =
        InterlockedCompareExchange(
            &g_qpLtkObservedPacked,
            0,
            0);

    if (observed == 1) {
        if (!ApplyLicenseToKillForce(true)) {
            return false;
        }

        Log(
            "F1 License To Kill: native state OFF -> FORCE ON");
        return true;
    }

    if (observed == 2) {
        if (!ApplyLicenseToKillForce(false)) {
            return false;
        }

        Log(
            "F1 License To Kill: native state ON -> FORCE OFF");
        return true;
    }

    Log(
        "[ERROR] F1 License To Kill: native runtime state not observed yet.");
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
        GetPrivateProfileIntW(L"Auto", L"Enabled", 0, iniPath.c_str()) != 0;

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

    // A23: no fixed heap address can be trusted across different game
    // sessions, graphics initialization patterns, or Windows heap layouts.
    // Scan the original known-good range, and extend it around the live
    // player loadout (within 32 MiB in either direction).
    struct ScanStats {
        std::size_t regions = 0;
        std::size_t readableRegions = 0;
        std::size_t readCalls = 0;
        std::size_t readFailures = 0;
        std::size_t bytesRead = 0;
    };
    ScanStats fixedStats{};
    ScanStats localStats{};

    auto scanRange = [&](std::uintptr_t scanBegin, std::uintptr_t scanEnd,
                         ScanStats& stats) {
        std::uintptr_t cursor = scanBegin;
        while (cursor < scanEnd) {
            MEMORY_BASIC_INFORMATION mbi{};
            if (VirtualQuery(reinterpret_cast<LPCVOID>(cursor), &mbi, sizeof(mbi)) != sizeof(mbi)) {
                cursor += 0x1000;
                continue;
            }

            ++stats.regions;
            const std::uintptr_t regionBase =
                reinterpret_cast<std::uintptr_t>(mbi.BaseAddress);
            const std::uintptr_t regionEnd =
                regionBase + static_cast<std::uintptr_t>(mbi.RegionSize);

            if (mbi.State == MEM_COMMIT && IsReadableProtection(mbi.Protect)) {
                ++stats.readableRegions;
                std::uintptr_t chunk = (std::max)(regionBase, scanBegin);
                while (chunk < regionEnd && chunk < scanEnd) {
                    const SIZE_T wanted = static_cast<SIZE_T>(
                        std::min<std::uintptr_t>(
                            buffer.size(),
                            std::min<std::uintptr_t>(regionEnd, scanEnd) - chunk));

                    SIZE_T got = 0;
                    ++stats.readCalls;
                    const BOOL readOk = ReadProcessMemory(
                        GetCurrentProcess(),
                        reinterpret_cast<LPCVOID>(chunk),
                        buffer.data(),
                        wanted,
                        &got);
                    if (!readOk) ++stats.readFailures;
                    if (readOk && got >= sizeof(std::uintptr_t)) {
                        stats.bytesRead += got;

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

    };

    scanRange(kGraphScanBegin, kGraphScanEnd, fixedStats);

    const std::uintptr_t anchor = g_graphScanPlayerLoadout;

    // A27: 0x80000000 is NOT the Windows x64 user-mode address ceiling.
    // The previous 32-bit gate silently ignored e.g. 0x80B60E50 (as seen
    // in the user's A26 log), losing the entire player-relative scan.
    // Use the actual OS user address bound and VirtualQuery instead.
    SYSTEM_INFO systemInfo{};
    GetNativeSystemInfo(&systemInfo);
    const std::uintptr_t maxUserAddress =
        reinterpret_cast<std::uintptr_t>(systemInfo.lpMaximumApplicationAddress);
    constexpr std::uintptr_t kAnchorRadius = 0x02000000; // 32 MiB each side
    constexpr std::uintptr_t kMinAnchor = 0x10000000;
    constexpr std::uintptr_t kPageMask = ~std::uintptr_t{0xFFFF};
    constexpr std::uintptr_t kPageRound = 0xFFFF;

    MEMORY_BASIC_INFORMATION anchorMemory{};
    const bool anchorInUserSpace =
        anchor >= kMinAnchor && anchor <= maxUserAddress;
    const bool anchorQueried =
        anchorInUserSpace &&
        VirtualQuery(reinterpret_cast<LPCVOID>(anchor),
                     &anchorMemory, sizeof(anchorMemory)) == sizeof(anchorMemory);
    const bool anchorReadable =
        anchorQueried && anchorMemory.State == MEM_COMMIT &&
        IsReadableProtection(anchorMemory.Protect);

    std::uintptr_t localBegin = 0;
    std::uintptr_t localEnd = 0;
    if (anchorReadable) {
        // All arithmetic is bounded by the actual user-mode address range.
        // The scan is never wider than 64 MiB + alignment and always
        // excludes duplicate areas already visited by the fixed scan.
        localBegin = (anchor - (std::min)(anchor, kAnchorRadius)) & kPageMask;
        const std::uintptr_t endUnaligned =
            maxUserAddress - anchor < kAnchorRadius
                ? maxUserAddress : anchor + kAnchorRadius;
        localEnd = endUnaligned <= maxUserAddress - kPageRound
            ? (endUnaligned + kPageRound) & kPageMask
            : maxUserAddress;

        if (localBegin < kGraphScanBegin) {
            scanRange(localBegin, (std::min)(localEnd, kGraphScanBegin), localStats);
        }
        if (localEnd > kGraphScanEnd) {
            scanRange((std::max)(localBegin, kGraphScanEnd), localEnd, localStats);
        }
    } else {
        Log("A27 graph scan: player anchor=%p rejected (userSpace=%d queried=%d "
            "committed=%d readable=%d); fixed-range fallback only.",
            reinterpret_cast<void*>(anchor),
            anchorInUserSpace ? 1 : 0, anchorQueried ? 1 : 0,
            anchorQueried && anchorMemory.State == MEM_COMMIT ? 1 : 0,
            anchorReadable ? 1 : 0);
    }

    Log("A27 graph scan: anchor=%p maxUser=%p local=%p..%p fixed=0x%llX..0x%llX "
        "fixed{regions=%zu readable=%zu calls=%zu failed=%zu bytes=%zu} "
        "local{regions=%zu readable=%zu calls=%zu failed=%zu bytes=%zu}.",
        reinterpret_cast<void*>(anchor), reinterpret_cast<void*>(maxUserAddress),
        reinterpret_cast<void*>(localBegin), reinterpret_cast<void*>(localEnd),
        static_cast<unsigned long long>(kGraphScanBegin),
        static_cast<unsigned long long>(kGraphScanEnd),
        fixedStats.regions, fixedStats.readableRegions, fixedStats.readCalls,
        fixedStats.readFailures, fixedStats.bytesRead,
        localStats.regions, localStats.readableRegions, localStats.readCalls,
        localStats.readFailures, localStats.bytesRead);

    std::sort(spawners.begin(), spawners.end());
    spawners.erase(std::unique(spawners.begin(), spawners.end()), spawners.end());

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
        "A23 graph index: %zu linked graph(s), %zu item RID entry(s), %zu spawner(s), player loadout anchor=0x%p.",
        g_graphs.size(),
        itemToRid.size(),
        spawners.size(),
        reinterpret_cast<void*>(anchor));

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

void ResetActiveWeaponState() {
    g_activeWeapon = {};
    g_spawnMailbox.Cancel();
    g_pickupMailbox.Cancel();
}

BeginWeaponResult BeginWeaponRequest(std::uint64_t requestedRid, bool packageOwned) {
    WeaponGraph source{};

    if (!ResolveGraph(requestedRid, source)) {
        Log(
            "[ERROR] GiveWeapon: source graph NOT FOUND for RID %016llX.",
            static_cast<unsigned long long>(RotateRid(requestedRid)));
        return BeginWeaponResult::Failed;
    }

    bool sourceIdle = false;
    if (!IsSpawnerIdle(source.spawner, sourceIdle)) {
        Log("[ERROR] DIRECT graph: source spawner state unreadable.");
        return BeginWeaponResult::Failed;
    }

    if (!sourceIdle) {
        return BeginWeaponResult::SpawnerBusy;
    }

    g_activeWeapon = {};
    g_activeWeapon.active = true;
    g_activeWeapon.packageOwned = packageOwned;
    g_activeWeapon.requestedRid = requestedRid;
    g_activeWeapon.spawner = source.spawner;
    g_activeWeapon.setupAt = GetTickCount64();

    g_activeWeapon.ticket = g_spawnMailbox.Submit(g_workerPlayer.snapshot, SpawnPayload{requestedRid, source}, GetTickCount64());
    if (!g_activeWeapon.ticket) {
        g_activeWeapon = {};
        return BeginWeaponResult::SpawnerBusy;
    }


    Log(
        "DIRECT graph prepared RID=%016llX item=0x%p spawner=0x%p",
        static_cast<unsigned long long>(RotateRid(requestedRid)),
        reinterpret_cast<void*>(source.itemEntry),
        reinterpret_cast<void*>(source.spawner));

    return BeginWeaponResult::Started;
}

const char* LoadoutSourceName(LoadoutSource source) {
    switch (source) {
    case LoadoutSource::Manual:
        return "F3 MANUAL";
    case LoadoutSource::Auto:
        return "AUTO";
    default:
        return "UNKNOWN";
    }
}

const char* LoadoutPhaseName(LoadoutPhase phase) {
    switch (phase) {
    case LoadoutPhase::QPistol:
        return "QPistol";
    case LoadoutPhase::OneHanded:
        return "OneHanded";
    case LoadoutPhase::TwoHanded:
        return "TwoHanded";
    default:
        return "Unknown";
    }
}

std::uint64_t CurrentPackageRid() {
    switch (g_loadoutPackage.phase) {
    case LoadoutPhase::QPistol:
        return g_loadoutPackage.profile.qPistol;
    case LoadoutPhase::OneHanded:
        return g_loadoutPackage.profile.oneHanded;
    case LoadoutPhase::TwoHanded:
        return g_loadoutPackage.profile.twoHanded;
    default:
        return 0;
    }
}

void FinishLoadoutPackage(bool success, const char* reason) {
    const char* source = LoadoutSourceName(g_loadoutPackage.source);

    g_loadoutPackage.active = false;
    g_loadoutPackage.retryAt = 0;
    g_loadoutPackage.retries = 0;
    g_loadoutPackage.justCompleted = success;
    g_loadoutPackage.justFailed = !success;

    Log(
        "%s PACKAGE %s%s%s",
        source,
        success ? "COMPLETE" : "FAILED",
        reason ? ": " : "",
        reason ? reason : "");
}

void AdvanceLoadoutPackage() {
    if (!g_loadoutPackage.active) {
        return;
    }

    g_loadoutPackage.retries = 0;
    g_loadoutPackage.retryAt = 0;

    if (g_loadoutPackage.phase == LoadoutPhase::QPistol) {
        g_loadoutPackage.phase = LoadoutPhase::OneHanded;
        return;
    }

    if (g_loadoutPackage.phase == LoadoutPhase::OneHanded) {
        g_loadoutPackage.phase = LoadoutPhase::TwoHanded;
        return;
    }

    FinishLoadoutPackage(!g_loadoutPackage.hadFailure,
        g_loadoutPackage.hadFailure ? "one or more roles unconfirmed; no blind replay" : nullptr);
}

bool StartLoadoutPackage(
    const LoadoutProfile& profile,
    LoadoutSource source,
    std::uint32_t playerId) {

    if (g_loadoutPackage.active) {
        Log(
            "%s package request ignored: another loadout package is already active.",
            LoadoutSourceName(source));
        return false;
    }

    g_loadoutPackage = {};
    g_loadoutPackage.active = true;
    g_loadoutPackage.source = source;
    g_loadoutPackage.profile = profile;
    g_loadoutPackage.playerId = playerId;
    g_loadoutPackage.phase = LoadoutPhase::QPistol;

    Log(
        "%s PACKAGE START playerId=%u QPistol=%016llX OneHanded=%016llX TwoHanded=%016llX",
        LoadoutSourceName(source),
        playerId,
        static_cast<unsigned long long>(RotateRid(profile.qPistol)),
        static_cast<unsigned long long>(RotateRid(profile.oneHanded)),
        static_cast<unsigned long long>(RotateRid(profile.twoHanded)));

    return true;
}

void RetryLoadoutPackage(const char* reason) {
    if (!g_loadoutPackage.active) {
        return;
    }

    ++g_loadoutPackage.retries;

    if (g_loadoutPackage.retries > kWeaponMaxRetries) {
        FinishLoadoutPackage(false, reason);
        return;
    }

    g_graphs.clear();
    g_graphIndexBuilt = false;
    g_loadoutPackage.retryAt = GetTickCount64() + kWeaponRetryDelayMs;

    Log(
        "[WARN] %s package role %s retry %u/%u in %llu ms: %s",
        LoadoutSourceName(g_loadoutPackage.source),
        LoadoutPhaseName(g_loadoutPackage.phase),
        g_loadoutPackage.retries,
        kWeaponMaxRetries,
        static_cast<unsigned long long>(kWeaponRetryDelayMs),
        reason ? reason : "unknown failure");
}

void RetryUnitaryWeapon(const char* reason) {
    if (g_weaponQueue.empty()) {
        return;
    }

    ++g_unitaryRetryCount;

    if (g_unitaryRetryCount > kWeaponMaxRetries) {
        Log(
            "[ERROR] GiveWeapon unitary request DROPPED after %u retries RID=%016llX: %s",
            kWeaponMaxRetries,
            static_cast<unsigned long long>(RotateRid(g_weaponQueue.front())),
            reason ? reason : "unknown failure");
        g_weaponQueue.pop_front();
        g_unitaryRetryCount = 0;
        g_unitaryRetryAt = 0;
        return;
    }

    g_graphs.clear();
    g_graphIndexBuilt = false;
    g_unitaryRetryAt = GetTickCount64() + kWeaponRetryDelayMs;

    Log(
        "[WARN] GiveWeapon unitary retry %u/%u in %llu ms RID=%016llX: %s",
        g_unitaryRetryCount,
        kWeaponMaxRetries,
        static_cast<unsigned long long>(kWeaponRetryDelayMs),
        static_cast<unsigned long long>(RotateRid(g_weaponQueue.front())),
        reason ? reason : "unknown failure");
}

void FailActiveWeapon(const char* reason) {
    if (!g_activeWeapon.active) {
        return;
    }

    const bool packageOwned = g_activeWeapon.packageOwned;
    const std::uint64_t rid = g_activeWeapon.requestedRid;
    const bool nativeEntered = g_activeWeapon.result.invoked || g_activeWeapon.pickupSubmitted;

    Log(
        "[ERROR] GiveWeapon attempt FAILED RID=%016llX: %s",
        static_cast<unsigned long long>(RotateRid(rid)),
        reason ? reason : "unknown failure");

    ResetActiveWeaponState();

    if (nativeEntered) {
        // Exceptions, unreadable post-state and timeouts do not undo a native
        // effect. Preserve later roles, but never spawn this role again blindly.
        g_nextWeaponAllowedAt = GetTickCount64() + kWeaponInterRequestDelayMs;
        if (packageOwned) {
            g_loadoutPackage.hadFailure = true;
            AdvanceLoadoutPackage();
        } else {
            if (!g_weaponQueue.empty() && g_weaponQueue.front() == rid) g_weaponQueue.pop_front();
            g_unitaryRetryCount = 0; g_unitaryRetryAt = 0;
        }
        Log("Weapon outcome unconfirmed; native effect not replayed RID=%016llX.",
            static_cast<unsigned long long>(RotateRid(rid)));
        return;
    }
    if (packageOwned) {
        RetryLoadoutPackage(reason);
    } else {
        RetryUnitaryWeapon(reason);
    }
}

void CompleteActiveWeapon() {
    if (!g_activeWeapon.active) {
        return;
    }

    const bool packageOwned = g_activeWeapon.packageOwned;
    const std::uint64_t rid = g_activeWeapon.requestedRid;
    const ULONGLONG now = GetTickCount64();

    Log(
        "DELIVERY CONFIRMED in current player attachment list RID=%016llX",
        static_cast<unsigned long long>(RotateRid(rid)));

    g_nextWeaponAllowedAt = now + kWeaponInterRequestDelayMs;
    Log(
        "GiveWeapon queue cooldown = %llu ms",
        static_cast<unsigned long long>(kWeaponInterRequestDelayMs));

    ResetActiveWeaponState();

    if (packageOwned) {
        Log(
            "%s package role %s COMPLETE RID=%016llX",
            LoadoutSourceName(g_loadoutPackage.source),
            LoadoutPhaseName(g_loadoutPackage.phase),
            static_cast<unsigned long long>(RotateRid(rid)));
        AdvanceLoadoutPackage();
        return;
    }

    if (!g_weaponQueue.empty() && g_weaponQueue.front() == rid) {
        g_weaponQueue.pop_front();
    } else {
        Log(
            "[ERROR] GiveWeapon unitary queue mismatch at completion RID=%016llX.",
            static_cast<unsigned long long>(RotateRid(rid)));
    }

    g_unitaryRetryCount = 0;
    g_unitaryRetryAt = 0;
}

void ProcessWeaponQueue() {
    if (g_activeWeapon.active) {
        if (const auto result = g_spawnMailbox.TakeReply(g_activeWeapon.ticket)) g_activeWeapon.result = *result;
        const auto result = g_activeWeapon.result;
        if (result.exception) {
            FailActiveWeapon("native trigger exception");
            return;
        }

        const ULONGLONG now = GetTickCount64();
        const bool triggered = result.triggered;

        if (!triggered) {
            if (!g_activeWeapon.dispatchDeferredLogged && now - g_activeWeapon.setupAt > 2000) {
                Log("Weapon request=%llu session=%llu waiting for safe gameplay dispatch; no native call repeated.",
                    g_activeWeapon.ticket.request, g_activeWeapon.ticket.session);
                g_activeWeapon.dispatchDeferredLogged = true;
            }
            return;
        }

        if (!g_activeWeapon.observationLogged) {
            const bool valid =
                result.valid;
            const bool accepted =
                result.accepted;
            const unsigned int state =
                result.state;
            const bool pending =
                result.pending;

            Log(
                "Native Spawn observation RID=%016llX valid=%s accepted=%s state=%u pending=%s",
                static_cast<unsigned long long>(
                    RotateRid(g_activeWeapon.requestedRid)),
                valid ? "yes" : "no",
                accepted ? "yes" : "no",
                state,
                pending ? "yes" : "no");

            g_activeWeapon.observationLogged = true;
        }

        if (!result.valid) {
            FailActiveWeapon("native post-call state unreadable");
            return;
        }

        if (!result.accepted) {
            FailActiveWeapon("native spawner did not accept request");
            return;
        }

        NativeMemoryReader read;
        std::uint32_t mask = 0;
        if (!g_activeWeapon.created.pointer) {
            auto created = result.token.output;
            if (ReadNativeThreadMask(mask) && runtime::LiveEntity(read, g_exeBase, mask, result.token.source)) {
                if (!runtime::LiveEntity(read, g_exeBase, mask, created))
                    created = runtime::ReadSpawnOutput(read, g_activeWeapon.spawner, result.token);
                if (runtime::LiveEntity(read, g_exeBase, mask, created)) g_activeWeapon.created = created;
            }
        }
        bool idle = false;
        if (!IsSpawnerIdle(g_activeWeapon.spawner, idle)) {
            FailActiveWeapon("spawner became unreadable");
            return;
        }

        if (idle && ReadNativeThreadMask(mask)) {
            const auto item = runtime::ReadNativeItem(read, g_exeBase, mask, g_activeWeapon.created);
            const auto attachment = runtime::ReadAttachment(read, g_exeBase, mask, g_workerPlayer.playerId, item);
            if (attachment == runtime::Attachment::Present) {
                CompleteActiveWeapon();
                return;
            }
            if (g_activeWeapon.pickupTicket) {
                if (const auto published = g_pickupMailbox.TakeReply(g_activeWeapon.pickupTicket)) {
                    Log("Native pickup publication %s RID=%016llX; waiting for attachment confirmation.",
                        *published ? "processed" : "not confirmed",
                        static_cast<unsigned long long>(RotateRid(g_activeWeapon.requestedRid)));
                    g_activeWeapon.pickupTicket = {};
                    g_activeWeapon.pickupAcknowledgedAt = now;
                }
            }
            if (attachment == runtime::Attachment::Absent && !g_activeWeapon.pickupSubmitted) {
                g_activeWeapon.pickupTicket = g_pickupMailbox.Submit(g_workerPlayer.snapshot,
                    PendingPickupRequest{item, g_workerPlayer.playerId}, now);
                if (g_activeWeapon.pickupTicket) {
                    g_activeWeapon.pickupSubmitted = true;
                    Log("Created item absent from player; native pickup queued once RID=%016llX itemId=%u request=%llu.",
                        static_cast<unsigned long long>(RotateRid(g_activeWeapon.requestedRid)), item.id,
                        g_activeWeapon.pickupTicket.request);
                    return;
                }
            }
        }

        const ULONGLONG triggeredAt =
            g_activeWeapon.pickupAcknowledgedAt ? g_activeWeapon.pickupAcknowledgedAt : result.at;
        const ULONGLONG elapsed = triggeredAt ? now - triggeredAt : 0;

        if (g_activeWeapon.pickupTicket && g_pickupMailbox.Busy()) return;
        if (elapsed > 5000) {
            FailActiveWeapon("delivery timeout: attachment not confirmed");
        }

        return;
    }

    const ULONGLONG now = GetTickCount64();
    if (g_nextWeaponAllowedAt && now < g_nextWeaponAllowedAt) {
        return;
    }

    while (g_loadoutPackage.active && CurrentPackageRid() == 0) {
        Log(
            "%s package role %s = Off/None; skipping.",
            LoadoutSourceName(g_loadoutPackage.source),
            LoadoutPhaseName(g_loadoutPackage.phase));
        AdvanceLoadoutPackage();
    }

    if (g_loadoutPackage.active) {
        if (g_loadoutPackage.retryAt && now < g_loadoutPackage.retryAt) {
            return;
        }

        const std::uint64_t rid = CurrentPackageRid();
        const BeginWeaponResult result = BeginWeaponRequest(rid, true);

        if (result == BeginWeaponResult::Started) {
            Log(
                "%s package role %s armed RID=%016llX",
                LoadoutSourceName(g_loadoutPackage.source),
                LoadoutPhaseName(g_loadoutPackage.phase),
                static_cast<unsigned long long>(RotateRid(rid)));
        } else if (result == BeginWeaponResult::Failed) {
            RetryLoadoutPackage("BeginWeaponRequest failed");
        }
        return;
    }

    if (g_weaponQueue.empty()) {
        return;
    }

    if (g_unitaryRetryAt && now < g_unitaryRetryAt) {
        return;
    }

    const std::uint64_t rid = g_weaponQueue.front();
    const BeginWeaponResult result = BeginWeaponRequest(rid, false);

    if (result == BeginWeaponResult::Failed) {
        RetryUnitaryWeapon("BeginWeaponRequest failed");
    }
}

bool QueueWeapon(std::uint64_t rid, const char* source) {
    if (!rid) {
        Log("%s ignored: no weapon configured.", source);
        return false;
    }

    // A24: never enqueue identical requests already pending or in-flight.
    // A23's crash log had 33 manual weapon hotkey enqueues and 41 successful
    // native spawns with numerous duplicate F5-F8 requests.
    if (g_activeWeapon.active &&
        !g_activeWeapon.packageOwned &&
        g_activeWeapon.requestedRid == rid) {

        Log(
            "[A24] %s suppressed: RID=%016llX already active.",
            source,
            static_cast<unsigned long long>(RotateRid(rid)));
        return false;
    }

    if (std::find(g_weaponQueue.begin(), g_weaponQueue.end(), rid) !=
        g_weaponQueue.end()) {

        Log(
            "[A24] %s suppressed: RID=%016llX already queued.",
            source,
            static_cast<unsigned long long>(RotateRid(rid)));
        return false;
    }

    if (g_weaponQueue.size() >= kUnitaryPendingLimit) {
        Log(
            "[A24] %s suppressed: pending weapon queue limit (%zu).",
            source,
            kUnitaryPendingLimit);
        return false;
    }

    const ULONGLONG now = GetTickCount64();
    while (!g_recentUnitaryWeaponRequests.empty() &&
           now - g_recentUnitaryWeaponRequests.front() >=
               kUnitaryBurstWindowMs) {
        g_recentUnitaryWeaponRequests.pop_front();
    }

    if (g_recentUnitaryWeaponRequests.size() >= kUnitaryBurstLimit) {
        Log(
            "[A24] %s suppressed: rapid native spawn request guard "
            "(%zu requests per %llu ms).",
            source,
            kUnitaryBurstLimit,
            static_cast<unsigned long long>(kUnitaryBurstWindowMs));
        return false;
    }

    g_recentUnitaryWeaponRequests.push_back(now);
    g_weaponQueue.push_back(rid);

    Log(
        "%s queued RID=%016llX (pending=%zu, recent=%zu/%zu)",
        source,
        static_cast<unsigned long long>(RotateRid(rid)),
        g_weaponQueue.size(),
        g_recentUnitaryWeaponRequests.size(),
        kUnitaryBurstLimit);
    return true;
}

bool QueueAmmoProfile(const AmmoProfile& profile, std::uint32_t playerId, const char* source) {
    if (!playerId || !g_workerPlayer || playerId != g_workerPlayer.playerId) return false;
    const auto ticket = g_ammoMailbox.Submit(g_workerPlayer.snapshot, PendingAmmoRequest{profile, playerId, source}, GetTickCount64());
    if (!ticket) return false;
    g_lastAmmoTicket = ticket;
    g_lastAmmoSource = source;
    Log("%s queued: session=%llu request=%llu playerId=%u.", source, ticket.session, ticket.request, playerId);
    return true;
}
bool RuntimePlayerMatches(std::uint32_t playerId) {
    return g_tickPlayer && g_tickPlayer.playerId == playerId;
}

bool PublishAmmoProfileGameplayThread(const PendingAmmoRequest& request) {
    if (!RuntimePlayerMatches(request.playerId)) {
        return false;
    }

    std::uintptr_t owner = 0;
    if (!SafeRead(g_exeBase + kAmmoOwnerGlobalRva, owner) || !owner) {
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
    }

    if (locked) {
        __try {
            LeaveCriticalSection(criticalSection);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            nativeOk = false;
        }
    }

    if (!nativeOk) {
        return false;
    }

    if (!published) {
        return true;
    }
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
        ResetActiveWeaponState();
    }
    g_weaponQueue.clear();
    g_recentUnitaryWeaponRequests.clear();
    g_nextWeaponAllowedAt = 0;
    g_unitaryRetryCount = 0;
    g_unitaryRetryAt = 0;
    g_loadoutPackage = {};
    g_spawnMailbox.Cancel();
    g_ammoMailbox.Cancel();
    g_pickupMailbox.Cancel();
    g_lastAmmoTicket = {}; g_autoAmmoTicket = {};
    {
        std::lock_guard<std::mutex> lock(g_playerMutex);
        g_playerSession.InterruptAuto(g_workerPlayer.snapshot.session);
    }


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

    Log("Q Protocol A36 / Native Delivery TEST");
    Log("Scope: accepted A28/A29 core and menu; stable updates by default.");

    if (!ValidateTargetExecutable()) {
        Log("Fresh Core disabled because executable validation failed.");
        return 0;
    }

    Log("Target executable accepted.");
    if (!updates::EnsureDefaultIni(iniPath)) {
        Log("[ERROR] Cannot create missing default INI; core not started.");
        return 0;
    }
    LoadConfig(iniPath);
    updates::Initialize(iniPath);

    if (!InstallGameplayHook()) {
        Log("[ERROR] Fresh Core disabled: gameplay hook unavailable.");
        return 0;
    }

    if (!InstallLicenseToKillObserveHook()) {
        Log("[ERROR] F1 native-state toggle disabled: observe hook unavailable.");
    }

    if (OverlayInitialize(iniPath)) {
        Log("Overlay bootstrap started in fail-open mode. Toggle key loaded from [Overlay] ToggleKey.");
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
    Log("Overlay = remappable [Overlay] ToggleKey, INI-backed, Save/Reload/Reset Defaults.");

    PlayerContext previousPlayer{};
    bool previousReady = false;

    std::string lastOverlayStatus = OverlayStatus();

    SHORT keyPrevious[13]{};

    while (g_running.load(std::memory_order_acquire)) {
        const PlayerContext player = ResolvePlayer();
        g_workerPlayer = player;
        const bool ready = static_cast<bool>(player);
        g_graphScanPlayerLoadout = ready
            ? reinterpret_cast<std::uintptr_t>(player.loadout ? player.loadout : player.character)
            : 0;

        const bool playerIdentityChanged =
            ready && previousReady &&
            player.snapshot.epoch != previousPlayer.snapshot.epoch;

        const bool loadoutPointerChanged =
            ready && previousReady &&
            player.playerId == previousPlayer.playerId &&
            player.loadout != previousPlayer.loadout;

        if (ready != previousReady || playerIdentityChanged) {
            if (ready) {
                Log(
                    "PLAYER PLAYABLE character=0x%p playerId=%u session=%llu epoch=%llu",
                    player.character, player.playerId, player.snapshot.session, player.snapshot.epoch);
            } else if (previousReady) {
                Log("PLAYER NOT PLAYABLE: %s", player.reason);
            }

            ResetWeaponRuntime(
                ready ? "player identity changed/ready" : "player not ready");

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

        if (ready && g_autoEnabled && AutoStage() == runtime::AutoStage::Waiting &&
            !g_loadoutPackage.active && !g_activeWeapon.active && g_weaponQueue.empty()) {
            if (StartLoadoutPackage(g_autoLoadout, LoadoutSource::Auto, player.playerId)) {
                if (BeginAuto(player)) Log("AUTO started for playable session=%llu.", player.snapshot.session);
                else ResetWeaponRuntime("playable session changed before AUTO start");
            }
        }
        if (g_lastAmmoTicket) {
            if (const auto result = g_ammoMailbox.TakeReply(g_lastAmmoTicket)) {
                Log("%s native processing %s: session=%llu request=%llu; inventory delivery not independently verified.",
                    g_lastAmmoSource, *result ? "finished" : "failed", g_lastAmmoTicket.session, g_lastAmmoTicket.request);
                if (g_autoAmmoTicket == g_lastAmmoTicket) {
                    std::lock_guard<std::mutex> lock(g_playerMutex);
                    g_playerSession.AmmoProcessed(g_autoAmmoTicket.session, *result);
                    g_autoAmmoTicket = {};
                }
                g_lastAmmoTicket = {};
            }
        }
        if (ready && AutoStage() == runtime::AutoStage::NeedAmmo &&
            QueueAmmoProfile(g_autoAmmo, player.playerId, "AUTO Ammo")) {
            std::lock_guard<std::mutex> lock(g_playerMutex);
            if (g_playerSession.AmmoSubmitted(player.snapshot.session)) g_autoAmmoTicket = g_lastAmmoTicket;
        }


        const bool overlayTogglePressed =
            OverlayPump(ready);

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

        const bool overlayVisible =
            OverlayIsVisible();

        // Always advance edge state, even while the overlay is open.
        // This prevents held keys from leaking into gameplay when the menu closes.
        const bool f1Pressed =
            KeyPressedEdge(
                VK_F1,
                keyPrevious[1]);

        const bool f2Pressed =
            KeyPressedEdge(
                VK_F2,
                keyPrevious[2]);

        const bool f3Pressed =
            KeyPressedEdge(
                VK_F3,
                keyPrevious[3]);

        const bool f4Pressed =
            KeyPressedEdge(
                VK_F4,
                keyPrevious[4]);

        if (!overlayVisible &&
            !overlayTogglePressed &&
            f1Pressed) {

            ToggleLicenseToKill();
        }

        if (!overlayVisible &&
            !overlayTogglePressed &&
            f2Pressed) {

            if (!ready) {
                Log("F2 ignored: player not ready.");
            } else {
                QueueAmmoProfile(
                    g_manualAmmo,
                    player.playerId,
                    "F2 ManualAmmo");
            }
        }

        if (!overlayVisible &&
            !overlayTogglePressed &&
            f3Pressed) {

            if (!ready) {
                Log("F3 ignored: player not ready.");
            } else {
                if (!StartLoadoutPackage(
                        g_manualLoadout,
                        LoadoutSource::Manual,
                        player.playerId)) {

                    Log("F3 ManualLoadout ignored: loadout package busy.");
                }
            }
        }

        if (!overlayVisible &&
            !overlayTogglePressed &&
            f4Pressed) {

            if (!ready) {
                Log("F4 ignored: player not ready.");
            } else {
                const std::uint64_t rid =
                    g_qpistolNextB
                        ? g_qpistolModeB
                        : g_qpistolModeA;

                if (QueueWeapon(
                        rid,
                        "F4 Q-Pistol")) {

                    g_qpistolNextB =
                        !g_qpistolNextB;
                }
            }
        }

        for (int i = 0;
             i < 8;
             ++i) {

            const int vk =
                VK_F5 + i;

            const bool pressed =
                KeyPressedEdge(
                    vk,
                    keyPrevious[5 + i]);

            if (overlayVisible ||
                overlayTogglePressed ||
                !pressed) {
                continue;
            }

            char source[16]{};
            sprintf_s(
                source,
                "F%d",
                i + 5);

            if (!ready) {
                Log(
                    "%s ignored: player not ready.",
                    source);
            } else {
                QueueWeapon(
                    g_hotkeyWeapons[i],
                    source);
            }
        }

        if (ready) {
            ProcessWeaponQueue();

            if (g_loadoutPackage.justCompleted) {
                const LoadoutSource completedSource =
                    g_loadoutPackage.source;
                const std::uint32_t completedPlayerId =
                    g_loadoutPackage.playerId;

                g_loadoutPackage.justCompleted = false;

                if (completedSource == LoadoutSource::Auto) {
                    std::lock_guard<std::mutex> lock(g_playerMutex);
                    g_playerSession.WeaponsProcessed(player.snapshot.session, completedPlayerId == player.playerId);
                }
            }


            if (g_loadoutPackage.justFailed) {
                const LoadoutSource failedSource =
                    g_loadoutPackage.source;

                g_loadoutPackage.justFailed = false;

                if (failedSource == LoadoutSource::Auto) {
                    {
                        std::lock_guard<std::mutex> lock(g_playerMutex);
                        g_playerSession.WeaponsProcessed(player.snapshot.session, false);
                    }
                    Log("[ERROR] AUTO PACKAGE failed for this session; partial grants will not be blindly repeated.");
                }
            }
        } else if (g_activeWeapon.active ||
                   !g_weaponQueue.empty() ||
                   g_loadoutPackage.active) {
            ResetWeaponRuntime("player unavailable");
        }

        Sleep(16);
    }

    if (g_activeWeapon.active) {
        ResetActiveWeaponState();
    }

    if (!RestoreLicenseToKillNatural()) {
        Log("[ERROR] License To Kill natural-state restore failed during DLL shutdown.");
    }

    if (!RemoveLicenseToKillObserveHook()) {
        Log("[ERROR] LTK observe hook restore failed during DLL shutdown.");
    }

    updates::Shutdown();
    OverlayShutdown();
    return 0;
}

} // namespace qp

namespace qp {
SpawnResult ExecuteSpawn(const SpawnPayload& payload) {
    SpawnResult result{};
    result.triggered = true;
    result.at = GetTickCount64();
    if (!g_nativeSpawn || !ValidateGraph(payload.rid, payload.graph)) return result;
    __try {
        const auto spawner = payload.graph.spawner;
        bool idle = false;
        if (!IsSpawnerIdle(spawner, idle) || !idle) return result;
        result.invoked = true;
        g_nativeSpawn(reinterpret_cast<void*>(spawner));
        NativeMemoryReader read;
        result.token = runtime::CaptureSpawnToken(read, spawner, payload.graph.itemEntry);
        std::uintptr_t begin = 0, end = 0; BYTE state = 0xFF;
        result.valid = SafeRead(spawner + 0x38, begin) && SafeRead(spawner + 0x40, end) && SafeRead(spawner + 0x50, state);
        result.pending = result.valid && begin != end;
        result.accepted = result.valid && (state != 0 || result.pending);
        result.state = result.valid ? state : 0xFFFFFFFF;
        result.triggered = true; result.at = GetTickCount64();
    } __except (EXCEPTION_EXECUTE_HANDLER) { result.exception = true; }
    return result;
}

bool PublishPickupGameplayThread(const PendingPickupRequest& request) {
    if (!RuntimePlayerMatches(request.playerId)) return false;
    NativeMemoryReader read;
    std::uint32_t mask = 0;
    if (!ReadNativeThreadMask(mask)) return false;
    const auto item = runtime::ReadNativeItem(read, g_exeBase, mask, request.item.created);
    if (!item || item.id != request.item.id || item.interfacePointer != request.item.interfacePointer) return false;
    const auto attachment = runtime::ReadAttachment(read, g_exeBase, mask, request.playerId, item);
    if (attachment == runtime::Attachment::Present) return true;
    if (attachment != runtime::Attachment::Absent) return false;
    std::uintptr_t owner = 0;
    if (!SafeRead(g_exeBase + kAmmoOwnerGlobalRva, owner) || !owner) return false;
    using InsertPickupFn = void(__fastcall*)(void*, const NativePickupInput*);
    const auto insert = reinterpret_cast<InsertPickupFn>(g_exeBase + kNativeAmmoInsertRva);
    const auto notify = reinterpret_cast<NativeAmmoNotifyFn>(g_exeBase + kNativeAmmoNotifyRva);
    auto* criticalSection = reinterpret_cast<LPCRITICAL_SECTION>(owner + kAmmoLockOffset);
    const NativePickupInput input{request.playerId, item.id, 0}; // Native default pickup mode.
    bool locked = false, ok = true;
    __try {
        EnterCriticalSection(criticalSection);
        locked = true;
        insert(reinterpret_cast<void*>(owner + 0x10A68), &input);
        notify(reinterpret_cast<void*>(owner), 0xF3, reinterpret_cast<void*>(owner + 0x109C8));
    } __except (EXCEPTION_EXECUTE_HANDLER) { ok = false; }
    if (locked) {
        __try { LeaveCriticalSection(criticalSection); }
        __except (EXCEPTION_EXECUTE_HANDLER) { ok = false; }
    }
    return ok; // Publication only; worker still requires native attachment proof.
}
}
extern "C" void QpGameplayTick() {
    if (qp::g_tickBusy.test_and_set(std::memory_order_acquire)) return;
    qp::g_tickPlayer = qp::ReadDispatchPlayer();
    const auto state = qp::g_tickPlayer.snapshot;
    if (const auto command = qp::g_spawnMailbox.TryTake(state)) {
        const auto result = qp::ExecuteSpawn(command->payload);
        qp::g_tickPlayer = qp::ReadDispatchPlayer();
        qp::g_spawnMailbox.Complete(*command, qp::g_tickPlayer.snapshot, result);
    }
    if (const auto command = qp::g_pickupMailbox.TryTake(qp::g_tickPlayer.snapshot)) {
        const bool result = qp::PublishPickupGameplayThread(command->payload);
        qp::g_tickPlayer = qp::ReadDispatchPlayer();
        qp::g_pickupMailbox.Complete(*command, qp::g_tickPlayer.snapshot, result);
    }
    if (const auto command = qp::g_ammoMailbox.TryTake(qp::g_tickPlayer.snapshot)) {
        const bool result = qp::PublishAmmoProfileGameplayThread(command->payload);
        qp::g_tickPlayer = qp::ReadDispatchPlayer();
        qp::g_ammoMailbox.Complete(*command, qp::g_tickPlayer.snapshot, result);
    }
    qp::g_tickBusy.clear(std::memory_order_release);
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
