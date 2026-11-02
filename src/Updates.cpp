#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "Updates.h"
#include <atomic>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

namespace qp::updates {
namespace {
namespace fs = std::filesystem;
std::mutex lock;
Status status;
std::wstring ini;
std::atomic_bool running{false}, startup{true}, betaChannel{false};
enum class Action { None, Check, Stage, Cancel };
Action action = Action::None;

HMODULE Module() {
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Module), &module)) throw std::runtime_error("ASI module not found");
    return module;
}
std::vector<char> Resource(int id) {
    HMODULE module = Module();
    HRSRC r = FindResourceW(module, MAKEINTRESOURCEW(id), RT_RCDATA);
    DWORD length = r ? SizeofResource(module, r) : 0;
    HGLOBAL loaded = r ? LoadResource(module, r) : nullptr;
    const char* bytes = loaded ? static_cast<const char*>(LockResource(loaded)) : nullptr;
    if (!bytes || !length || length > 2 * 1024 * 1024) throw std::runtime_error("Embedded updater/config resource missing");
    return {bytes, bytes + length};
}
fs::path Folder() {
    wchar_t file[32768]{};
    DWORD size = GetModuleFileNameW(Module(), file, 32768);
    if (!size || size >= 32768) throw std::runtime_error("ASI path unavailable");
    return fs::path(file).parent_path();
}
void Failure(const std::string& text) {
    std::lock_guard<std::mutex> guard(lock);
    status.busy = false; status.queued = false; status.available = false;
    status.phase = "error"; status.message = text;
}
std::map<std::string,std::string> ReadStatus(const fs::path& path) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                          nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return {};
    char bytes[8193]{}; DWORD count = 0;
    BOOL ok = ReadFile(h, bytes, 8192, &count, nullptr); CloseHandle(h);
    if (!ok || !count || count >= 8192) return {};
    std::map<std::string,std::string> fields;
    const std::string text(bytes, count);
    size_t start = 0;
    while (start < text.size()) {
        const size_t end = text.find('\n', start);
        std::string line = text.substr(start, end == std::string::npos ? end : end - start);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const size_t equal = line.find('=');
        if (equal != std::string::npos) fields[line.substr(0,equal)] = line.substr(equal+1);
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return fields;
}
std::wstring Quote(const std::wstring& s) { return L"\"" + s + L"\""; }
void Worker() {
    HANDLE process = nullptr;
    fs::path current;
    try {
        wchar_t local[32768]{};
        DWORD size = GetEnvironmentVariableW(L"LOCALAPPDATA",local,32768);
        if (!size || size >= 32768) throw std::runtime_error("LOCALAPPDATA unavailable");
        fs::path session = fs::path(local) / L"QProtocolUpdater" /
            (std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()));
        fs::create_directories(session);
        fs::path helper = session / L"QProtocolUpdater.exe";
        auto resource = Resource(101);
        { std::ofstream out(helper, std::ios::binary); out.write(resource.data(),static_cast<std::streamsize>(resource.size()));
          if (!out.good()) throw std::runtime_error("Cannot extract updater helper"); }
        unsigned request = 0;
        while (running.load()) {
            Action next; bool beta; int selected;
            { std::lock_guard<std::mutex> guard(lock); next = action; action = Action::None; beta = betaChannel.load(); selected = status.build; }
            if ((next == Action::Check || next == Action::Stage) && process) {
                std::lock_guard<std::mutex> guard(lock);
                action = next; next = Action::None; // finish the previous helper before starting this request
            }
            if (next == Action::Cancel && process) {
                std::ofstream cancel(current / L"cancel.request"); cancel << "cancel\n";
                if (!cancel.good()) throw std::runtime_error("Unable to cancel scheduled update");
            } else if ((next == Action::Check || next == Action::Stage) && !process) {
                current = session / std::to_wstring(++request);
                fs::create_directories(current);
                std::wstring cmd = Quote(helper.wstring()) + (next == Action::Stage ? L" --stage " : L" --check ") +
                    Quote(Folder().wstring()) + L" " + Quote(current.wstring()) + L" " +
                    std::to_wstring(GetCurrentProcessId()) + L" " + std::to_wstring(Build) +
                    (beta ? L" beta " : L" stable ") + std::to_wstring(selected);
                std::vector<wchar_t> buffer(cmd.begin(),cmd.end()); buffer.push_back(L'\0');
                STARTUPINFOW si{}; si.cb = sizeof(si); PROCESS_INFORMATION pi{};
                if (!CreateProcessW(helper.c_str(),buffer.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW | DETACHED_PROCESS,
                                    nullptr,nullptr,&si,&pi)) throw std::runtime_error("Unable to start updater helper");
                CloseHandle(pi.hThread); process = pi.hProcess;
            }
            if (process) {
                auto fields = ReadStatus(current / L"status.txt");
                if (!fields.empty() && fields.count("phase")) {
                    const std::string phase = fields["phase"];
                    int build = 0;
                    try { if(fields.count("build")) build = std::stoi(fields["build"]); } catch (...) {}
                    std::lock_guard<std::mutex> guard(lock);
                    status.phase = phase; status.message = fields["message"];
                    status.version = fields["version"]; status.build = build;
                    status.available = phase == "available" && build > Build;
                    status.queued = phase == "staged";
                    status.busy = phase == "checking" || phase == "downloading";
                }
                if (WaitForSingleObject(process,0) == WAIT_OBJECT_0) {
                    DWORD exitCode = 0; GetExitCodeProcess(process,&exitCode);
                    CloseHandle(process); process = nullptr;
                    if (fields.empty()) Failure("Updater did not return a result.");
                    else if (exitCode != 0 && fields["phase"] != "error") Failure("Updater operation failed.");
                }
            }
            Sleep(250);
        }
    } catch (const std::exception& e) { running.store(false); Failure(e.what()); }
    if (process) CloseHandle(process); // independent helper keeps waiting for actual game exit
}
}
bool EnsureDefaultIni(const std::wstring& path) {
    if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) return true;
    try {
        auto data = Resource(102);
        HANDLE h = CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if (h == INVALID_HANDLE_VALUE) return GetLastError() == ERROR_FILE_EXISTS;
        DWORD count = 0; BOOL ok = WriteFile(h,data.data(),static_cast<DWORD>(data.size()),&count,nullptr);
        FlushFileBuffers(h); CloseHandle(h);
        if (!ok || count != data.size()) { DeleteFileW(path.c_str()); return false; }
        return true;
    } catch (...) { return false; }
}
void Initialize(const std::wstring& path) {
    ini = path;
    startup.store(GetPrivateProfileIntW(L"Updater",L"CheckAtStart",1,path.c_str()) != 0);
    betaChannel.store(GetPrivateProfileIntW(L"Updater",L"Beta",0,path.c_str()) != 0);
    HMODULE pinned = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&Initialize),&pinned)) { Failure("Updater lifetime could not be secured."); return; }
    running.store(true);
    try { std::thread(Worker).detach(); } catch (...) { running.store(false); Failure("Updater worker could not start."); return; }
    if (startup.load()) Check();
}
void Shutdown() { running.store(false); }
Status Snapshot() { std::lock_guard<std::mutex> guard(lock); return status; }
bool CheckAtStart() { return startup.load(); }
bool Beta() { return betaChannel.load(); }
void SetOptions(bool checkAtStart,bool beta) {
    { std::lock_guard<std::mutex> guard(lock);
      if (status.busy || status.queued || action != Action::None) return;
      startup.store(checkAtStart); betaChannel.store(beta); status = Status{}; status.checkedBeta = beta; }
    WritePrivateProfileStringW(L"Updater",L"CheckAtStart",checkAtStart?L"1":L"0",ini.c_str());
    WritePrivateProfileStringW(L"Updater",L"Beta",beta?L"1":L"0",ini.c_str());
}
void Check() {
    std::lock_guard<std::mutex> guard(lock);
    if (!running.load() || status.busy || status.queued || action != Action::None) return;
    status = Status{}; status.busy = true; status.checkedBeta = betaChannel.load();
    status.message = "Checking GitHub..."; action = Action::Check;
}
void InstallOnExit() {
    std::lock_guard<std::mutex> guard(lock);
    if (status.busy || status.queued || !status.available || status.checkedBeta != betaChannel.load() || action != Action::None) return;
    status.busy = true; status.available = false; status.message = "Preparing update..."; action = Action::Stage;
}
void Cancel() {
    std::lock_guard<std::mutex> guard(lock);
    if (status.queued && action == Action::None) action = Action::Cancel;
}
}
