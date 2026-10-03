#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace qp {

bool OverlayInitialize(const std::wstring& iniPath);
void OverlayPump(
    bool playerReady,
    bool autoDone,
    std::size_t weaponQueueCount,
    bool qpistolNextB);
bool OverlayConsumeReloadRequest();
bool OverlayConsumeSpawnRequest(std::uint64_t& displayRid);
void OverlayPublishRuntimeWeaponRids(
    const std::uint64_t* displayRids,
    std::size_t count);
bool OverlayIsVisible();
const char* OverlayStatus();
void OverlayShutdown();

} // namespace qp
