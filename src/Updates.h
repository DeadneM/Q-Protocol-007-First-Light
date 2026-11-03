#pragma once
#include <string>

namespace qp::updates {
inline constexpr int Build = 32;
inline constexpr char Version[] = "A32 TEST";
struct Status {
    bool busy = false, available = false, queued = false, checkedBeta = false;
    int build = 0;
    std::string phase, version, message = "Check GitHub for a compatible update.";
};
bool EnsureDefaultIni(const std::wstring& path);
void Initialize(const std::wstring& path);
void Shutdown();
Status Snapshot();
bool CheckAtStart();
bool Beta();
void SetOptions(bool checkAtStart, bool beta);
void Check();
void InstallOnExit();
void Cancel();
}
