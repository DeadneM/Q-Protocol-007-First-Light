#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "CosmeticDiscovery.h"
#include "QProtocolInternal.h"

#include "MinHook.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

namespace qp {
namespace {

// October 2026 executable, verified from the uploaded 007FirstLight.exe.
// These are runtime handlers associated with the reflected KNT cosmetic lists.
constexpr std::uintptr_t kGadgetSkinsHandlerRva   = 0x0138C4C0;
constexpr std::uintptr_t kFirearmSkinsHandlerRva  = 0x0138C5C0;
constexpr std::uintptr_t kOutfitsHandlerRva       = 0x0139FAC0;
constexpr std::uintptr_t kUnlockablesHandlerRva   = 0x01399440;

constexpr BYTE kGadgetSkinsPreimage[] = {
    0x48,0x89,0x5C,0x24,0x18,0x57,0x48,0x83,
    0xEC,0x40,0x48,0x8B,0x7A,0x08,0x48,0x8D
};

constexpr BYTE kFirearmSkinsPreimage[] = {
    0x48,0x8B,0xC4,0x48,0x89,0x48,0x08,0x55,
    0x53,0x48,0x8D,0x68,0xB8,0x48,0x81,0xEC
};

constexpr BYTE kOutfitsPreimage[] = {
    0x48,0x89,0x5C,0x24,0x20,0x57,0x48,0x83,
    0xEC,0x40,0xF7,0x02,0xFF,0xFF,0xFF,0x3F
};

constexpr BYTE kUnlockablesPreimage[] = {
    0x48,0x89,0x4C,0x24,0x08,0x55,0x53,0x56,
    0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41
};

using OneArgFn = std::uintptr_t(__fastcall*)(void*);
using TwoArgFn = std::uintptr_t(__fastcall*)(void*, void*);

OneArgFn g_originalFirearmSkins = nullptr;
OneArgFn g_originalUnlockables = nullptr;
TwoArgFn g_originalGadgetSkins = nullptr;
TwoArgFn g_originalOutfits = nullptr;

std::uintptr_t g_exeBase = 0;
bool g_enabled = false;
int g_maxCallsPerHook = 8;

std::atomic<int> g_gadgetCalls{0};
std::atomic<int> g_firearmCalls{0};
std::atomic<int> g_outfitCalls{0};
std::atomic<int> g_unlockableCalls{0};

std::mutex g_logMutex;
std::vector<void*> g_installedTargets;

void Logf(const char* fmt, ...) {
    char line[2048]{};

    va_list args;
    va_start(args, fmt);
    vsnprintf_s(
        line,
        sizeof(line),
        _TRUNCATE,
        fmt,
        args);
    va_end(args);

    std::lock_guard<std::mutex> lock(
        g_logMutex);

    QpDiagnosticLogLine(line);
}

bool IsReadableProtection(DWORD protect) {
    if ((protect & PAGE_GUARD) ||
        (protect & PAGE_NOACCESS)) {
        return false;
    }

    const DWORD base =
        protect & 0xFF;

    return
        base == PAGE_READONLY ||
        base == PAGE_READWRITE ||
        base == PAGE_WRITECOPY ||
        base == PAGE_EXECUTE_READ ||
        base == PAGE_EXECUTE_READWRITE ||
        base == PAGE_EXECUTE_WRITECOPY;
}

bool IsReadableAddress(
    std::uintptr_t address,
    SIZE_T size = 1) {

    if (!address ||
        address < 0x10000) {
        return false;
    }

    MEMORY_BASIC_INFORMATION mbi{};

    if (VirtualQuery(
            reinterpret_cast<LPCVOID>(address),
            &mbi,
            sizeof(mbi)) != sizeof(mbi)) {
        return false;
    }

    if (mbi.State != MEM_COMMIT ||
        !IsReadableProtection(mbi.Protect)) {
        return false;
    }

    const std::uintptr_t begin =
        reinterpret_cast<std::uintptr_t>(
            mbi.BaseAddress);

    const std::uintptr_t end =
        begin +
        static_cast<std::uintptr_t>(
            mbi.RegionSize);

    return
        address >= begin &&
        address + size >= address &&
        address + size <= end;
}

bool SafeReadBlock(
    std::uintptr_t address,
    void* out,
    SIZE_T size) {

    if (!out ||
        !size ||
        !IsReadableAddress(
            address,
            size)) {
        return false;
    }

    SIZE_T got = 0;

    return
        ReadProcessMemory(
            GetCurrentProcess(),
            reinterpret_cast<LPCVOID>(address),
            out,
            size,
            &got) != FALSE &&
        got == size;
}

bool MatchPreimage(
    std::uintptr_t address,
    const BYTE* expected,
    SIZE_T size) {

    std::array<BYTE, 32> current{};

    if (!expected ||
        size > current.size() ||
        !SafeReadBlock(
            address,
            current.data(),
            size)) {
        return false;
    }

    return memcmp(
        current.data(),
        expected,
        size) == 0;
}

bool TryReadAscii(
    std::uintptr_t address,
    std::string& out) {

    out.clear();

    if (!IsReadableAddress(
            address,
            1)) {
        return false;
    }

    char buffer[160]{};
    SIZE_T got = 0;

    MEMORY_BASIC_INFORMATION mbi{};

    if (VirtualQuery(
            reinterpret_cast<LPCVOID>(address),
            &mbi,
            sizeof(mbi)) != sizeof(mbi)) {
        return false;
    }

    const std::uintptr_t regionEnd =
        reinterpret_cast<std::uintptr_t>(
            mbi.BaseAddress) +
        static_cast<std::uintptr_t>(
            mbi.RegionSize);

    const SIZE_T wanted =
        static_cast<SIZE_T>(
            std::min<std::uintptr_t>(
                sizeof(buffer) - 1,
                regionEnd - address));

    if (!wanted ||
        !ReadProcessMemory(
            GetCurrentProcess(),
            reinterpret_cast<LPCVOID>(address),
            buffer,
            wanted,
            &got) ||
        got < 3) {
        return false;
    }

    SIZE_T length = 0;

    while (length < got &&
           length < sizeof(buffer) - 1) {

        const unsigned char c =
            static_cast<unsigned char>(
                buffer[length]);

        if (c == 0) {
            break;
        }

        if (c < 0x20 ||
            c > 0x7E) {
            return false;
        }

        ++length;
    }

    if (length < 3 ||
        length >= got) {
        return false;
    }

    out.assign(
        buffer,
        buffer + length);

    return true;
}

void DumpQwords(
    const char* label,
    std::uintptr_t address) {

    if (!label ||
        !address) {
        return;
    }

    std::array<std::uint64_t, 80> qwords{};

    if (!SafeReadBlock(
            address,
            qwords.data(),
            sizeof(qwords))) {

        Logf(
            "[COSDISC] %s address=0x%p unreadable",
            label,
            reinterpret_cast<void*>(
                address));

        return;
    }

    Logf(
        "[COSDISC] %s address=0x%p",
        label,
        reinterpret_cast<void*>(
            address));

    const std::size_t keyOffsets[] = {
        0x00, 0x08, 0x10, 0x18,
        0x20, 0x28, 0x30, 0x38,
        0x40, 0x50,
        0x150, 0x158, 0x160, 0x168,
        0x1D0, 0x1D8, 0x1E0, 0x1E8,
        0x1F0, 0x1F8, 0x200, 0x208,
        0x210, 0x218, 0x220, 0x228
    };

    for (const std::size_t offset :
         keyOffsets) {

        if (offset + sizeof(std::uint64_t) >
            sizeof(qwords)) {
            continue;
        }

        std::uint64_t value = 0;

        memcpy(
            &value,
            reinterpret_cast<const BYTE*>(
                qwords.data()) +
                offset,
            sizeof(value));

        Logf(
            "[COSDISC] %s +0x%03zX = 0x%016llX",
            label,
            offset,
            static_cast<unsigned long long>(
                value));
    }
}

void DumpCandidateStrings(
    const char* label,
    std::uintptr_t address) {

    if (!label ||
        !address) {
        return;
    }

    constexpr SIZE_T kBytes = 0x280;

    std::array<BYTE, kBytes> block{};

    if (!SafeReadBlock(
            address,
            block.data(),
            block.size())) {
        return;
    }

    std::vector<std::string> seen;
    int emitted = 0;

    // Inline ASCII strings.
    for (SIZE_T i = 0;
         i + 3 < block.size() &&
         emitted < 32;) {

        const unsigned char c =
            block[i];

        if (c < 0x20 ||
            c > 0x7E) {
            ++i;
            continue;
        }

        SIZE_T end = i;

        while (end < block.size()) {
            const unsigned char x =
                block[end];

            if (x == 0) {
                break;
            }

            if (x < 0x20 ||
                x > 0x7E) {
                break;
            }

            ++end;
        }

        if (end > i + 2 &&
            end < block.size() &&
            block[end] == 0) {

            std::string value(
                reinterpret_cast<const char*>(
                    block.data() + i),
                end - i);

            if (std::find(
                    seen.begin(),
                    seen.end(),
                    value) ==
                seen.end()) {

                seen.push_back(value);

                Logf(
                    "[COSDISC] %s inline +0x%03zX \"%s\"",
                    label,
                    i,
                    value.c_str());

                ++emitted;
            }

            i = end + 1;

        } else {

            ++i;
        }
    }

    // Direct pointers to ASCII strings.
    for (SIZE_T i = 0;
         i + sizeof(std::uintptr_t) <=
             block.size() &&
         emitted < 48;
         i += sizeof(std::uintptr_t)) {

        std::uintptr_t candidate = 0;

        memcpy(
            &candidate,
            block.data() + i,
            sizeof(candidate));

        std::string value;

        if (!TryReadAscii(
                candidate,
                value)) {
            continue;
        }

        if (std::find(
                seen.begin(),
                seen.end(),
                value) !=
            seen.end()) {
            continue;
        }

        seen.push_back(value);

        Logf(
            "[COSDISC] %s ptr +0x%03zX -> 0x%p \"%s\"",
            label,
            i,
            reinterpret_cast<void*>(
                candidate),
            value.c_str());

        ++emitted;
    }
}

void DumpInvocation(
    const char* label,
    int callIndex,
    void* self,
    void* args = nullptr) {

    Logf(
        "[COSDISC] ===== %s call=%d self=0x%p args=0x%p =====",
        label,
        callIndex,
        self,
        args);

    const auto selfAddress =
        reinterpret_cast<std::uintptr_t>(
            self);

    DumpQwords(
        label,
        selfAddress);

    DumpCandidateStrings(
        label,
        selfAddress);

    if (args) {
        std::string argLabel =
            std::string(label) +
            ".args";

        DumpQwords(
            argLabel.c_str(),
            reinterpret_cast<std::uintptr_t>(
                args));

        DumpCandidateStrings(
            argLabel.c_str(),
            reinterpret_cast<std::uintptr_t>(
                args));
    }
}

std::uintptr_t __fastcall HookGadgetSkins(
    void* self,
    void* args) {

    const std::uintptr_t result =
        g_originalGadgetSkins
            ? g_originalGadgetSkins(
                  self,
                  args)
            : 0;

    const int call =
        g_gadgetCalls.fetch_add(
            1,
            std::memory_order_acq_rel) +
        1;

    if (call <= g_maxCallsPerHook) {
        DumpInvocation(
            "gadgetSkins",
            call,
            self,
            args);
    }

    return result;
}

std::uintptr_t __fastcall HookFirearmSkins(
    void* self) {

    const std::uintptr_t result =
        g_originalFirearmSkins
            ? g_originalFirearmSkins(
                  self)
            : 0;

    const int call =
        g_firearmCalls.fetch_add(
            1,
            std::memory_order_acq_rel) +
        1;

    if (call <= g_maxCallsPerHook) {
        DumpInvocation(
            "firearmSkins",
            call,
            self);
    }

    return result;
}

std::uintptr_t __fastcall HookOutfits(
    void* self,
    void* args) {

    const std::uintptr_t result =
        g_originalOutfits
            ? g_originalOutfits(
                  self,
                  args)
            : 0;

    const int call =
        g_outfitCalls.fetch_add(
            1,
            std::memory_order_acq_rel) +
        1;

    if (call <= g_maxCallsPerHook) {
        DumpInvocation(
            "outfits",
            call,
            self,
            args);
    }

    return result;
}

std::uintptr_t __fastcall HookUnlockables(
    void* self) {

    const std::uintptr_t result =
        g_originalUnlockables
            ? g_originalUnlockables(
                  self)
            : 0;

    const int call =
        g_unlockableCalls.fetch_add(
            1,
            std::memory_order_acq_rel) +
        1;

    if (call <= g_maxCallsPerHook) {
        DumpInvocation(
            "online.unlockables",
            call,
            self);
    }

    return result;
}

bool InstallHook(
    const char* label,
    std::uintptr_t rva,
    const BYTE* preimage,
    SIZE_T preimageSize,
    void* detour,
    void** original) {

    const std::uintptr_t target =
        g_exeBase + rva;

    if (!MatchPreimage(
            target,
            preimage,
            preimageSize)) {

        Logf(
            "[COSDISC] %s preimage mismatch at RVA 0x%08llX; hook skipped",
            label,
            static_cast<unsigned long long>(
                rva));

        return false;
    }

    const MH_STATUS create =
        MH_CreateHook(
            reinterpret_cast<void*>(
                target),
            detour,
            original);

    if (create != MH_OK) {
        Logf(
            "[COSDISC] %s MH_CreateHook failed status=%d",
            label,
            static_cast<int>(
                create));

        return false;
    }

    const MH_STATUS enable =
        MH_EnableHook(
            reinterpret_cast<void*>(
                target));

    if (enable != MH_OK) {
        MH_RemoveHook(
            reinterpret_cast<void*>(
                target));

        Logf(
            "[COSDISC] %s MH_EnableHook failed status=%d",
            label,
            static_cast<int>(
                enable));

        return false;
    }

    g_installedTargets.push_back(
        reinterpret_cast<void*>(
            target));

    Logf(
        "[COSDISC] %s hook installed RVA=0x%08llX",
        label,
        static_cast<unsigned long long>(
            rva));

    return true;
}

} // namespace

bool CosmeticDiscoveryInitialize(
    std::uintptr_t exeBase,
    const std::wstring& iniPath) {

    g_exeBase = exeBase;

    g_enabled =
        GetPrivateProfileIntW(
            L"CosmeticDiscovery",
            L"Enabled",
            1,
            iniPath.c_str()) != 0;

    g_maxCallsPerHook =
        std::clamp(
            static_cast<int>(
                GetPrivateProfileIntW(
                    L"CosmeticDiscovery",
                    L"MaxCallsPerHook",
                    8,
                    iniPath.c_str())),
            1,
            32);

    if (!g_enabled) {
        Logf(
            "[COSDISC] disabled by INI");
        return true;
    }

    if (!g_exeBase) {
        Logf(
            "[COSDISC] no EXE base; discovery disabled");
        return false;
    }

    g_gadgetCalls.store(0);
    g_firearmCalls.store(0);
    g_outfitCalls.store(0);
    g_unlockableCalls.store(0);

    bool any = false;

    any |= InstallHook(
        "gadgetSkins",
        kGadgetSkinsHandlerRva,
        kGadgetSkinsPreimage,
        sizeof(kGadgetSkinsPreimage),
        reinterpret_cast<void*>(
            &HookGadgetSkins),
        reinterpret_cast<void**>(
            &g_originalGadgetSkins));

    any |= InstallHook(
        "firearmSkins",
        kFirearmSkinsHandlerRva,
        kFirearmSkinsPreimage,
        sizeof(kFirearmSkinsPreimage),
        reinterpret_cast<void*>(
            &HookFirearmSkins),
        reinterpret_cast<void**>(
            &g_originalFirearmSkins));

    any |= InstallHook(
        "outfits",
        kOutfitsHandlerRva,
        kOutfitsPreimage,
        sizeof(kOutfitsPreimage),
        reinterpret_cast<void*>(
            &HookOutfits),
        reinterpret_cast<void**>(
            &g_originalOutfits));

    any |= InstallHook(
        "online.unlockables",
        kUnlockablesHandlerRva,
        kUnlockablesPreimage,
        sizeof(kUnlockablesPreimage),
        reinterpret_cast<void*>(
            &HookUnlockables),
        reinterpret_cast<void**>(
            &g_originalUnlockables));

    Logf(
        "[COSDISC] A20 read-only cosmetic discovery %s; MaxCallsPerHook=%d",
        any ? "ACTIVE" : "NO HOOKS INSTALLED",
        g_maxCallsPerHook);

    return any;
}

void CosmeticDiscoveryShutdown() {

    for (void* target :
         g_installedTargets) {

        MH_DisableHook(target);
        MH_RemoveHook(target);
    }

    g_installedTargets.clear();

    g_originalGadgetSkins = nullptr;
    g_originalFirearmSkins = nullptr;
    g_originalOutfits = nullptr;
    g_originalUnlockables = nullptr;

    if (g_enabled) {
        Logf(
            "[COSDISC] shutdown");
    }

    g_enabled = false;
}

} // namespace qp
