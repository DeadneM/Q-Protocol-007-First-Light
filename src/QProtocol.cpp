#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

namespace qp {

constexpr std::uintptr_t kExpectedSizeOfImage = 0x06EC1000;
constexpr DWORD kExpectedTimeDateStamp = 0x6ABCDDDB;

// October 2026 executable mappings from docs/PRIMITIVE_AUDIT_OCT2026.md.
constexpr std::uintptr_t kPlayerResolverRva       = 0x0171DF40;
constexpr std::uintptr_t kPlayerRegistryHelperRva = 0x007D7770;
constexpr std::uintptr_t kPlayerRegistryGlobalRva = 0x069225A0;
constexpr std::uintptr_t kRuntimePlayerGlobalRva  = 0x064576F8;
constexpr std::uintptr_t kPlayerLoadoutVtableRva  = 0x02EDEB70;
constexpr std::uintptr_t kLicenseToKillRva        = 0x0191C344;

constexpr BYTE kLtkOff[6] = {0x32, 0xD2, 0x4C, 0x8B, 0x15, 0x53};
constexpr BYTE kLtkOn [6] = {0xB2, 0x01, 0x4C, 0x8B, 0x15, 0x53};

struct PlayerContext {
    void* loadout = nullptr;
    std::uint32_t playerId = 0;

    explicit operator bool() const {
        return loadout != nullptr && playerId != 0;
    }
};

HMODULE g_module = nullptr;
HANDLE g_log = INVALID_HANDLE_VALUE;
std::uintptr_t g_exeBase = 0;
bool g_running = true;

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

    return true;
}

using ResolveLocalPlayerFn = void(__fastcall*)(std::uint32_t, std::uint64_t*);
using LookupPlayerFn = void*(__fastcall*)(std::uint64_t*, void*);

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

    const std::uintptr_t expectedVtable = g_exeBase + kPlayerLoadoutVtableRva;
    if (vtable != expectedVtable) {
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

bool WriteCodeBytes(std::uintptr_t address, const BYTE* bytes, SIZE_T size) {
    DWORD oldProtect = 0;
    if (!VirtualProtect(reinterpret_cast<void*>(address), size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }

    memcpy(reinterpret_cast<void*>(address), bytes, size);
    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(address), size);

    DWORD ignored = 0;
    VirtualProtect(reinterpret_cast<void*>(address), size, oldProtect, &ignored);
    return true;
}

enum class LtkState {
    Off,
    On,
    Unknown
};

LtkState ReadLicenseToKillState() {
    BYTE current[6]{};
    if (!SafeRead(g_exeBase + kLicenseToKillRva, current)) {
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

    BYTE current[6]{};
    SafeRead(site, current);
    Log(
        "[ERROR] F1 License To Kill: preimage mismatch: %02X %02X %02X %02X %02X %02X",
        current[0], current[1], current[2], current[3], current[4], current[5]);
    return false;
}

bool KeyPressedEdge(int vk, SHORT& previous) {
    const SHORT now = GetAsyncKeyState(vk);
    const bool downNow = (now & 0x8000) != 0;
    const bool downBefore = (previous & 0x8000) != 0;
    previous = now;
    return downNow && !downBefore;
}

DWORD WINAPI WorkerThread(LPVOID) {
    const std::wstring dir = ModuleDirectory(g_module);
    const std::wstring logPath = dir + L"\\QProtocol.log";

    g_log = CreateFileW(
        logPath.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    Log("Q Protocol Fresh Core A1");
    Log("Scope: bootstrap + ResolvePlayer + F1 License To Kill only.");

    if (!ValidateTargetExecutable()) {
        Log("Fresh Core A1 disabled because executable validation failed.");
        return 0;
    }

    Log("Target executable accepted.");
    Log("F1 = License To Kill toggle.");
    Log("F2-F12 and AUTO are intentionally inactive in A1.");

    PlayerContext previousPlayer{};
    bool previousReady = false;
    SHORT f1Previous = 0;

    while (g_running) {
        const PlayerContext player = ResolvePlayer();
        const bool ready = static_cast<bool>(player);

        if (ready != previousReady ||
            (ready && (player.loadout != previousPlayer.loadout ||
                       player.playerId != previousPlayer.playerId))) {
            if (ready) {
                Log(
                    "PLAYER READY loadout=0x%p playerId=%u",
                    player.loadout,
                    player.playerId);
            } else if (previousReady) {
                Log("PLAYER NOT READY");
            }

            previousPlayer = player;
            previousReady = ready;
        }

        if (KeyPressedEdge(VK_F1, f1Previous)) {
            ToggleLicenseToKill();
        }

        Sleep(16);
    }

    return 0;
}

} // namespace qp

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        qp::g_module = module;
        DisableThreadLibraryCalls(module);

        HANDLE thread = CreateThread(nullptr, 0, qp::WorkerThread, nullptr, 0, nullptr);
        if (thread) {
            CloseHandle(thread);
        }
    } else if (reason == DLL_PROCESS_DETACH) {
        qp::g_running = false;
    }

    return TRUE;
}
