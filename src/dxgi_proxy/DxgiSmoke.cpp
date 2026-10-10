#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dxgi1_2.h>
#include <cstdio>

// Headless-friendly smoke check for ABI/name/ordinal forwarding.
// This program must reside next to the generated local dxgi.dll.
int wmain() {
    HMODULE proxy = LoadLibraryW(L"dxgi.dll");
    if (!proxy) {
        std::printf("Cannot load local proxy: %lu\n", GetLastError());
        return 1;
    }
    auto f0 = GetProcAddress(proxy, "CreateDXGIFactory");
    auto f1 = GetProcAddress(proxy, "CreateDXGIFactory1");
    auto f2 = GetProcAddress(proxy, "CreateDXGIFactory2");
    if (!f0 || !f1 || !f2) {
        std::printf("Required DXGI exports are missing\n");
        return 2;
    }
    using CreateFactory1 = HRESULT (WINAPI*)(REFIID, void**);
    IDXGIFactory1* factory = nullptr;
    const HRESULT hr = reinterpret_cast<CreateFactory1>(f1)(
        __uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory));
    std::printf("Proxy CreateDXGIFactory1 HRESULT=0x%08lX, factory=%p\n",
        static_cast<unsigned long>(hr), factory);
    if (factory) factory->Release();
    if (FAILED(hr)) return 3;
    FreeLibrary(proxy);
    return 0;
}
