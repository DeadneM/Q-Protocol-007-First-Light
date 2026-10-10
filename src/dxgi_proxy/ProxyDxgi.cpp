#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cwchar>
#include "dxgi_proxy_manifest.h"

// Q Protocol dedicated DXGI proxy loader.
//
// Never replace or overwrite the Windows DXGI binary. This file is deployed
// locally alongside the game executable, beside QProtocol.asi.
//
// Do not call LoadLibrary from DllMain: Windows' loader lock is held there.
// The first exported DXGI call resolves the real System32 module and loads
// QProtocol.asi once, then tail-jumps into the unchanged native implementation.

namespace {
HMODULE g_self = nullptr;
HMODULE g_realDxgi = nullptr;
HMODULE g_loadedAsi = nullptr;
INIT_ONCE g_initOnce = INIT_ONCE_STATIC_INIT;
FARPROC g_resolved[256]{};
DWORD g_dxgiError = 0;
DWORD g_asiError = 0;
bool g_skipAsi = false;
bool g_asiAlreadyLoaded = false;

bool ModulePath(wchar_t* result, std::size_t count, HMODULE module) {
    if (!result || count < MAX_PATH) return false;
    DWORD n = GetModuleFileNameW(module, result, static_cast<DWORD>(count));
    return n && n < count;
}

bool GetModuleDirectory(wchar_t* result, std::size_t count) {
    if (!ModulePath(result, count, g_self)) return false;
    wchar_t* last = wcsrchr(result, L'\\');
    if (!last) return false;
    *(last + 1) = L'\0';
    return true;
}

void WriteStartupLog(const wchar_t* gameDir) {
    if (!gameDir) return;
    wchar_t path[MAX_PATH]{};
    if (wcscpy_s(path, gameDir) ||
        wcscat_s(path, L"QProtocolDXGI.log")) return;

    HANDLE file = CreateFileW(
        path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;

    char text[600]{};
    int n = sprintf_s(text,
        "Q Protocol custom DXGI proxy TEST\r\n"
        "Real System32 DXGI loaded: %s (Win32=%lu)\r\n"
        "Native exports enumerated: %zu\r\n"
        "QProtocol.asi: %s (Win32=%lu)\r\n"
        "Note: Use only one dxgi.dll proxy in the game directory.\r\n",
        g_realDxgi ? "yes" : "no",
        static_cast<unsigned long>(g_dxgiError),
        kDXGI_ExportCount,
        g_skipAsi ? "skipped for CI smoke check" :
          g_asiAlreadyLoaded ? "already present" :
          g_loadedAsi ? "loaded" : "not found/failed",
        static_cast<unsigned long>(g_asiError));
    if (n > 0) {
        DWORD written = 0;
        WriteFile(file, text, static_cast<DWORD>(n), &written, nullptr);
    }
    CloseHandle(file);
}

BOOL CALLBACK InitializeNativeDXGI(PINIT_ONCE, PVOID, PVOID*) {
    wchar_t systemPath[MAX_PATH]{};
    const UINT n = GetSystemDirectoryW(systemPath, MAX_PATH);
    if (!n || n >= MAX_PATH ||
        wcscat_s(systemPath, L"\\dxgi.dll") != 0) {
        g_dxgiError = GetLastError();
        return TRUE;
    }

    // Absolute System32 path: never self-load local dxgi.dll recursively.
    g_realDxgi = LoadLibraryW(systemPath);
    if (!g_realDxgi) {
        g_dxgiError = GetLastError();
        return TRUE;
    }

    for (std::size_t i = 0; i < kDXGI_ExportCount; ++i) {
        const auto& desc = kDXGI_Exports[i];
        g_resolved[i] = GetProcAddress(
            g_realDxgi,
            desc.name ? desc.name : MAKEINTRESOURCEA(desc.ordinal));
    }

    wchar_t gameDir[MAX_PATH]{};
    if (!GetModuleDirectory(gameDir, MAX_PATH)) {
        g_asiError = ERROR_BAD_PATHNAME;
        return TRUE;
    }

    // CI smoke test only; normally the loader always attempts the ASI.
    wchar_t optOut[4]{};
    g_skipAsi = (GetEnvironmentVariableW(
        L"QPROTOCOL_DXGI_NO_ASI", optOut, 4) != 0);
    if (!g_skipAsi) {
        g_loadedAsi = GetModuleHandleW(L"QProtocol.asi");
        g_asiAlreadyLoaded = g_loadedAsi != nullptr;
        if (!g_loadedAsi) {
            wchar_t path[MAX_PATH]{};
            if (wcscpy_s(path, gameDir) == 0 &&
                wcscat_s(path, L"QProtocol.asi") == 0) {
                g_loadedAsi = LoadLibraryExW(path, nullptr,
                    LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR |
                    LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
                if (!g_loadedAsi) g_asiError = GetLastError();
            } else {
                g_asiError = ERROR_BAD_PATHNAME;
            }
        }
    }

    WriteStartupLog(gameDir);
    return TRUE;
}
} // namespace

// The generated MASM tail-dispatch thunk preserves RCX/RDX/R8/R9 and
// XMM0-XMM5 and all stack arguments before jumping to the system function.
extern "C" HRESULT WINAPI DXGI_MissingExport() {
    SetLastError(ERROR_PROC_NOT_FOUND);
    return E_NOTIMPL;
}

extern "C" FARPROC DXGI_ResolveExport(unsigned int index) {
    InitOnceExecuteOnce(&g_initOnce, InitializeNativeDXGI, nullptr, nullptr);
    if (index >= kDXGI_ExportCount || !g_resolved[index])
        return reinterpret_cast<FARPROC>(&DXGI_MissingExport);
    return g_resolved[index];
}

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_self = module;
        DisableThreadLibraryCalls(module);
    }
    return TRUE;
}
