#pragma once

#include <cstdint>
#include <string>

namespace qp {

// Read-only runtime probe for cosmetic/unlockable discovery.
// Requires MinHook to have been initialized by the overlay.
bool CosmeticDiscoveryInitialize(
    std::uintptr_t exeBase,
    const std::wstring& iniPath);

void CosmeticDiscoveryShutdown();

} // namespace qp
