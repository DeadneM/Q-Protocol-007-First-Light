#pragma once

#include <cstddef>
#include <string>

namespace qp {

bool OverlayInitialize(const std::wstring& iniPath);
void OverlayPump(
    bool playerReady,
    bool autoDone,
    std::size_t weaponQueueCount,
    bool qpistolNextB);
bool OverlayConsumeReloadRequest();
bool OverlayIsVisible();
const char* OverlayStatus();
void OverlayShutdown();

} // namespace qp
