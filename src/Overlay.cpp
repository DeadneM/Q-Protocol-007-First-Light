#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "Overlay.h"

#include <algorithm>
#include <array>
#include <cwchar>
#include <string>
#include <vector>

namespace qp {
namespace {

constexpr wchar_t kOverlayClass[] = L"QProtocolOverlayA6";
constexpr int kWindowWidth = 720;
constexpr int kWindowHeight = 760;

enum ControlId : int {
    IDC_STATUS_PLAYER = 100,
    IDC_STATUS_AUTO,
    IDC_STATUS_QUEUE,
    IDC_STATUS_QPISTOL,
    IDC_AUTO_ENABLED = 110,
    IDC_F2_PROFILE,
    IDC_F3_PROFILE,

    IDC_MANUAL_QPISTOL = 200,
    IDC_MANUAL_ONEHANDED,
    IDC_MANUAL_TWOHANDED,
    IDC_MANUAL_AMMO_QPISTOL = 210,
    IDC_MANUAL_AMMO_SMG,
    IDC_MANUAL_AMMO_AR,
    IDC_MANUAL_AMMO_SHOTGUN,
    IDC_MANUAL_AMMO_SNIPER,
    IDC_MANUAL_AMMO_HEAVY,

    IDC_AUTO_QPISTOL = 300,
    IDC_AUTO_ONEHANDED,
    IDC_AUTO_TWOHANDED,
    IDC_AUTO_AMMO_QPISTOL = 310,
    IDC_AUTO_AMMO_SMG,
    IDC_AUTO_AMMO_AR,
    IDC_AUTO_AMMO_SHOTGUN,
    IDC_AUTO_AMMO_SNIPER,
    IDC_AUTO_AMMO_HEAVY,

    IDC_SAVE = 400,
    IDC_RELOAD,
    IDC_RESET
};

std::wstring g_iniPath;
HWND g_window = nullptr;
HWND g_gameWindow = nullptr;
HFONT g_font = nullptr;
HBRUSH g_backgroundBrush = nullptr;
HBRUSH g_editBrush = nullptr;
bool g_registered = false;
bool g_visible = false;
bool g_reloadRequested = false;
bool g_insertWasDown = false;
ULONGLONG g_lastStatusUpdate = 0;

HWND g_statusPlayer = nullptr;
HWND g_statusAuto = nullptr;
HWND g_statusQueue = nullptr;
HWND g_statusQPistol = nullptr;

HWND g_autoEnabled = nullptr;
HWND g_f2Profile = nullptr;
HWND g_f3Profile = nullptr;

HWND g_manualLoadout[3]{};
HWND g_autoLoadout[3]{};
HWND g_manualAmmo[6]{};
HWND g_autoAmmo[6]{};

const wchar_t* kLoadoutKeys[3] = {
    L"QPistol",
    L"OneHanded",
    L"TwoHanded"
};

const wchar_t* kAmmoKeys[6] = {
    L"QPistol",
    L"SMG",
    L"AssaultRifle",
    L"Shotgun",
    L"Sniper",
    L"HeavyPistol"
};

const wchar_t* kDefaultManualLoadout[3] = {
    L"QPistolSilenced",
    L"MachinePistolHighRecoil",
    L"ShotgunSemiAuto"
};

const wchar_t* kDefaultAutoLoadout[3] = {
    L"QPistolSilenced",
    L"HeavyPistol50Cal",
    L"ARMilitary"
};

const unsigned kDefaultAmmo[6] = {
    10, 30, 30, 8, 5, 8
};

void ApplyFont(HWND hwnd) {
    if (hwnd && g_font) {
        SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(g_font), TRUE);
    }
}

HWND AddStatic(
    HWND parent,
    const wchar_t* text,
    int x,
    int y,
    int w,
    int h,
    int id = 0) {

    HWND hwnd = CreateWindowExW(
        0,
        L"STATIC",
        text,
        WS_CHILD | WS_VISIBLE,
        x, y, w, h,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        nullptr,
        nullptr);
    ApplyFont(hwnd);
    return hwnd;
}

HWND AddButton(
    HWND parent,
    const wchar_t* text,
    int x,
    int y,
    int w,
    int h,
    int id,
    DWORD extraStyle = 0) {

    HWND hwnd = CreateWindowExW(
        0,
        L"BUTTON",
        text,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | extraStyle,
        x, y, w, h,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        nullptr,
        nullptr);
    ApplyFont(hwnd);
    return hwnd;
}

HWND AddEdit(
    HWND parent,
    int x,
    int y,
    int w,
    int h,
    int id) {

    HWND hwnd = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER,
        x, y, w, h,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        nullptr,
        nullptr);
    ApplyFont(hwnd);
    SendMessageW(hwnd, EM_SETLIMITTEXT, 6, 0);
    return hwnd;
}

HWND AddCombo(
    HWND parent,
    int x,
    int y,
    int w,
    int h,
    int id) {

    HWND hwnd = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"COMBOBOX",
        L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
        x, y, w, h,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        nullptr,
        nullptr);
    ApplyFont(hwnd);
    return hwnd;
}

std::wstring ReadIni(
    const wchar_t* section,
    const wchar_t* key,
    const wchar_t* fallback = L"") {

    wchar_t buffer[512]{};
    GetPrivateProfileStringW(
        section,
        key,
        fallback,
        buffer,
        static_cast<DWORD>(std::size(buffer)),
        g_iniPath.c_str());
    return buffer;
}

void WriteIni(
    const wchar_t* section,
    const wchar_t* key,
    const std::wstring& value) {

    WritePrivateProfileStringW(
        section,
        key,
        value.c_str(),
        g_iniPath.c_str());
}

std::vector<std::wstring> ReadWeaponAliases() {
    std::vector<wchar_t> buffer(65536);
    GetPrivateProfileSectionW(
        L"WeaponCatalog",
        buffer.data(),
        static_cast<DWORD>(buffer.size()),
        g_iniPath.c_str());

    std::vector<std::wstring> aliases;
    aliases.emplace_back(L"None");

    const wchar_t* p = buffer.data();
    while (*p) {
        std::wstring line = p;
        const std::size_t eq = line.find(L'=');
        if (eq != std::wstring::npos && eq > 0) {
            aliases.push_back(line.substr(0, eq));
        }
        p += line.size() + 1;
    }

    return aliases;
}

void FillCombo(HWND combo, const std::vector<std::wstring>& items) {
    SendMessageW(combo, CB_RESETCONTENT, 0, 0);
    for (const auto& item : items) {
        SendMessageW(
            combo,
            CB_ADDSTRING,
            0,
            reinterpret_cast<LPARAM>(item.c_str()));
    }
}

void SelectComboText(HWND combo, const std::wstring& value) {
    LRESULT index = SendMessageW(
        combo,
        CB_FINDSTRINGEXACT,
        static_cast<WPARAM>(-1),
        reinterpret_cast<LPARAM>(value.c_str()));

    if (index == CB_ERR) {
        index = SendMessageW(
            combo,
            CB_ADDSTRING,
            0,
            reinterpret_cast<LPARAM>(value.c_str()));
    }

    if (index != CB_ERR) {
        SendMessageW(combo, CB_SETCURSEL, static_cast<WPARAM>(index), 0);
    }
}

std::wstring ComboText(HWND combo) {
    const LRESULT index = SendMessageW(combo, CB_GETCURSEL, 0, 0);
    if (index == CB_ERR) {
        return L"";
    }

    wchar_t buffer[256]{};
    SendMessageW(
        combo,
        CB_GETLBTEXT,
        static_cast<WPARAM>(index),
        reinterpret_cast<LPARAM>(buffer));
    return buffer;
}

unsigned EditUInt(HWND edit) {
    wchar_t buffer[32]{};
    GetWindowTextW(edit, buffer, static_cast<int>(std::size(buffer)));
    wchar_t* end = nullptr;
    unsigned long value = wcstoul(buffer, &end, 10);
    if (!end || end == buffer) {
        return 0;
    }
    return static_cast<unsigned>(std::min<unsigned long>(value, 100000));
}

void SetEditUInt(HWND edit, unsigned value) {
    wchar_t buffer[32]{};
    swprintf_s(buffer, L"%u", value);
    SetWindowTextW(edit, buffer);
}

void LoadControlsFromIni() {
    if (!g_window) {
        return;
    }

    const auto aliases = ReadWeaponAliases();

    FillCombo(g_f2Profile, {L"Manual", L"Auto"});
    FillCombo(g_f3Profile, {L"Manual", L"Auto"});

    SelectComboText(
        g_f2Profile,
        ReadIni(L"Hotkey_F2", L"Profile", L"Manual"));
    SelectComboText(
        g_f3Profile,
        ReadIni(L"Hotkey_F3", L"Profile", L"Manual"));

    const bool autoEnabled =
        GetPrivateProfileIntW(
            L"Auto",
            L"Enabled",
            1,
            g_iniPath.c_str()) != 0;

    SendMessageW(
        g_autoEnabled,
        BM_SETCHECK,
        autoEnabled ? BST_CHECKED : BST_UNCHECKED,
        0);

    for (int i = 0; i < 3; ++i) {
        FillCombo(g_manualLoadout[i], aliases);
        FillCombo(g_autoLoadout[i], aliases);

        SelectComboText(
            g_manualLoadout[i],
            ReadIni(L"ManualLoadout", kLoadoutKeys[i], kDefaultManualLoadout[i]));
        SelectComboText(
            g_autoLoadout[i],
            ReadIni(L"AutoLoadout", kLoadoutKeys[i], kDefaultAutoLoadout[i]));
    }

    for (int i = 0; i < 6; ++i) {
        SetEditUInt(
            g_manualAmmo[i],
            GetPrivateProfileIntW(
                L"ManualAmmo",
                kAmmoKeys[i],
                kDefaultAmmo[i],
                g_iniPath.c_str()));

        SetEditUInt(
            g_autoAmmo[i],
            GetPrivateProfileIntW(
                L"AutoAmmo",
                kAmmoKeys[i],
                kDefaultAmmo[i],
                g_iniPath.c_str()));
    }
}

void SaveControlsToIni() {
    if (!g_window) {
        return;
    }

    WriteIni(L"Hotkey_F2", L"Profile", ComboText(g_f2Profile));
    WriteIni(L"Hotkey_F3", L"Profile", ComboText(g_f3Profile));

    WriteIni(
        L"Auto",
        L"Enabled",
        SendMessageW(g_autoEnabled, BM_GETCHECK, 0, 0) == BST_CHECKED
            ? L"1"
            : L"0");

    for (int i = 0; i < 3; ++i) {
        WriteIni(L"ManualLoadout", kLoadoutKeys[i], ComboText(g_manualLoadout[i]));
        WriteIni(L"AutoLoadout", kLoadoutKeys[i], ComboText(g_autoLoadout[i]));
    }

    for (int i = 0; i < 6; ++i) {
        WriteIni(
            L"ManualAmmo",
            kAmmoKeys[i],
            std::to_wstring(EditUInt(g_manualAmmo[i])));
        WriteIni(
            L"AutoAmmo",
            kAmmoKeys[i],
            std::to_wstring(EditUInt(g_autoAmmo[i])));
    }

    WritePrivateProfileStringW(nullptr, nullptr, nullptr, g_iniPath.c_str());
    g_reloadRequested = true;
}

void ResetDefaults() {
    WriteIni(L"Hotkey_F2", L"Profile", L"Manual");
    WriteIni(L"Hotkey_F3", L"Profile", L"Manual");
    WriteIni(L"Auto", L"Enabled", L"1");

    for (int i = 0; i < 3; ++i) {
        WriteIni(L"ManualLoadout", kLoadoutKeys[i], kDefaultManualLoadout[i]);
        WriteIni(L"AutoLoadout", kLoadoutKeys[i], kDefaultAutoLoadout[i]);
    }

    for (int i = 0; i < 6; ++i) {
        const std::wstring value = std::to_wstring(kDefaultAmmo[i]);
        WriteIni(L"ManualAmmo", kAmmoKeys[i], value);
        WriteIni(L"AutoAmmo", kAmmoKeys[i], value);
    }

    WritePrivateProfileStringW(nullptr, nullptr, nullptr, g_iniPath.c_str());
    LoadControlsFromIni();
    g_reloadRequested = true;
}

void BuildControls(HWND hwnd) {
    g_font = CreateFontW(
        -17, 0, 0, 0,
        FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI");

    AddStatic(hwnd, L"Q PROTOCOL  /  Fresh Core A6", 20, 14, 360, 24);

    g_statusPlayer = AddStatic(hwnd, L"Player: ...", 20, 48, 200, 22, IDC_STATUS_PLAYER);
    g_statusAuto = AddStatic(hwnd, L"AUTO: ...", 220, 48, 160, 22, IDC_STATUS_AUTO);
    g_statusQueue = AddStatic(hwnd, L"Queue: ...", 380, 48, 130, 22, IDC_STATUS_QUEUE);
    g_statusQPistol = AddStatic(hwnd, L"Q-Pistol: ...", 510, 48, 170, 22, IDC_STATUS_QPISTOL);

    g_autoEnabled = AddButton(
        hwnd,
        L"AUTO enabled",
        20, 82, 140, 28,
        IDC_AUTO_ENABLED,
        BS_AUTOCHECKBOX);

    AddStatic(hwnd, L"F2 Ammo profile", 190, 86, 120, 20);
    g_f2Profile = AddCombo(hwnd, 310, 80, 150, 180, IDC_F2_PROFILE);

    AddStatic(hwnd, L"F3 Loadout profile", 480, 86, 130, 20);
    g_f3Profile = AddCombo(hwnd, 610, 80, 80, 180, IDC_F3_PROFILE);

    AddStatic(hwnd, L"MANUAL PROFILE", 20, 130, 220, 22);
    AddStatic(hwnd, L"Loadout", 20, 160, 100, 20);
    AddStatic(hwnd, L"QPistol", 20, 190, 90, 20);
    AddStatic(hwnd, L"OneHanded", 20, 226, 90, 20);
    AddStatic(hwnd, L"TwoHanded", 20, 262, 90, 20);

    g_manualLoadout[0] = AddCombo(hwnd, 110, 184, 245, 300, IDC_MANUAL_QPISTOL);
    g_manualLoadout[1] = AddCombo(hwnd, 110, 220, 245, 300, IDC_MANUAL_ONEHANDED);
    g_manualLoadout[2] = AddCombo(hwnd, 110, 256, 245, 300, IDC_MANUAL_TWOHANDED);

    AddStatic(hwnd, L"Reserve ammo", 380, 160, 120, 20);
    const wchar_t* ammoLabels[6] = {
        L"QPistol", L"SMG", L"Assault Rifle", L"Shotgun", L"Sniper", L"Heavy Pistol"
    };
    for (int i = 0; i < 6; ++i) {
        const int y = 188 + i * 34;
        AddStatic(hwnd, ammoLabels[i], 380, y + 3, 110, 20);
        g_manualAmmo[i] = AddEdit(
            hwnd,
            500, y,
            110, 26,
            IDC_MANUAL_AMMO_QPISTOL + i);
    }

    AddStatic(hwnd, L"AUTO PROFILE", 20, 406, 220, 22);
    AddStatic(hwnd, L"Loadout", 20, 436, 100, 20);
    AddStatic(hwnd, L"QPistol", 20, 466, 90, 20);
    AddStatic(hwnd, L"OneHanded", 20, 502, 90, 20);
    AddStatic(hwnd, L"TwoHanded", 20, 538, 90, 20);

    g_autoLoadout[0] = AddCombo(hwnd, 110, 460, 245, 300, IDC_AUTO_QPISTOL);
    g_autoLoadout[1] = AddCombo(hwnd, 110, 496, 245, 300, IDC_AUTO_ONEHANDED);
    g_autoLoadout[2] = AddCombo(hwnd, 110, 532, 245, 300, IDC_AUTO_TWOHANDED);

    AddStatic(hwnd, L"Reserve ammo", 380, 436, 120, 20);
    for (int i = 0; i < 6; ++i) {
        const int y = 464 + i * 34;
        AddStatic(hwnd, ammoLabels[i], 380, y + 3, 110, 20);
        g_autoAmmo[i] = AddEdit(
            hwnd,
            500, y,
            110, 26,
            IDC_AUTO_AMMO_QPISTOL + i);
    }

    AddButton(hwnd, L"Save", 20, 686, 150, 36, IDC_SAVE, BS_PUSHBUTTON);
    AddButton(hwnd, L"Reload", 190, 686, 150, 36, IDC_RELOAD, BS_PUSHBUTTON);
    AddButton(hwnd, L"Reset Defaults", 360, 686, 170, 36, IDC_RESET, BS_PUSHBUTTON);
    AddStatic(hwnd, L"Insert = close overlay", 550, 694, 150, 20);

    LoadControlsFromIni();
}

BOOL CALLBACK FindGameWindowProc(HWND hwnd, LPARAM lParam) {
    if (hwnd == g_window || !IsWindowVisible(hwnd)) {
        return TRUE;
    }

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != GetCurrentProcessId()) {
        return TRUE;
    }

    RECT rect{};
    if (!GetClientRect(hwnd, &rect)) {
        return TRUE;
    }

    const long width = rect.right - rect.left;
    const long height = rect.bottom - rect.top;
    if (width < 640 || height < 360) {
        return TRUE;
    }

    auto* best = reinterpret_cast<std::pair<HWND, long long>*>(lParam);
    const long long area = static_cast<long long>(width) * height;
    if (area > best->second) {
        best->first = hwnd;
        best->second = area;
    }

    return TRUE;
}

HWND FindGameWindow() {
    std::pair<HWND, long long> best{nullptr, 0};
    EnumWindows(FindGameWindowProc, reinterpret_cast<LPARAM>(&best));
    return best.first;
}

void CenterOnGameWindow() {
    if (!g_window || !g_gameWindow) {
        return;
    }

    RECT r{};
    if (!GetWindowRect(g_gameWindow, &r)) {
        return;
    }

    const int gameW = r.right - r.left;
    const int gameH = r.bottom - r.top;
    const int x = r.left + std::max(0, (gameW - kWindowWidth) / 2);
    const int y = r.top + std::max(0, (gameH - kWindowHeight) / 2);

    SetWindowPos(
        g_window,
        HWND_TOPMOST,
        x, y,
        kWindowWidth,
        kWindowHeight,
        SWP_SHOWWINDOW);
}

void HideOverlay() {
    if (!g_window || !g_visible) {
        return;
    }

    ShowWindow(g_window, SW_HIDE);
    g_visible = false;
    ShowCursor(FALSE);

    if (g_gameWindow && IsWindow(g_gameWindow)) {
        SetForegroundWindow(g_gameWindow);
    }
}

void ShowOverlay() {
    if (!g_window) {
        return;
    }

    LoadControlsFromIni();
    CenterOnGameWindow();
    ShowWindow(g_window, SW_SHOW);
    SetForegroundWindow(g_window);
    SetFocus(g_window);
    ClipCursor(nullptr);
    ShowCursor(TRUE);
    g_visible = true;
}

LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_SAVE:
            SaveControlsToIni();
            return 0;
        case IDC_RELOAD:
            LoadControlsFromIni();
            g_reloadRequested = true;
            return 0;
        case IDC_RESET:
            ResetDefaults();
            return 0;
        default:
            break;
        }
        break;

    case WM_CLOSE:
        HideOverlay();
        return 0;

    case WM_CTLCOLORSTATIC: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, RGB(230, 235, 242));
        SetBkColor(dc, RGB(28, 31, 38));
        return reinterpret_cast<LRESULT>(g_backgroundBrush);
    }

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, RGB(235, 238, 244));
        SetBkColor(dc, RGB(43, 47, 57));
        return reinterpret_cast<LRESULT>(g_editBrush);
    }

    case WM_ERASEBKGND: {
        RECT rc{};
        GetClientRect(hwnd, &rc);
        FillRect(reinterpret_cast<HDC>(wParam), &rc, g_backgroundBrush);
        return 1;
    }

    case WM_DESTROY:
        g_window = nullptr;
        g_visible = false;
        return 0;

    default:
        break;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool EnsureWindow() {
    if (g_window) {
        return true;
    }

    if (!g_gameWindow || !IsWindow(g_gameWindow)) {
        g_gameWindow = FindGameWindow();
    }

    if (!g_gameWindow) {
        return false;
    }

    g_window = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        kOverlayClass,
        L"Q Protocol",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        kWindowWidth,
        kWindowHeight,
        g_gameWindow,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr);

    if (!g_window) {
        return false;
    }

    BuildControls(g_window);
    ShowWindow(g_window, SW_HIDE);
    return true;
}

void UpdateStatus(
    bool playerReady,
    bool autoDone,
    std::size_t weaponQueueCount,
    bool qpistolNextB) {

    if (!g_window) {
        return;
    }

    SetWindowTextW(
        g_statusPlayer,
        playerReady ? L"Player: READY" : L"Player: NOT READY");

    SetWindowTextW(
        g_statusAuto,
        autoDone ? L"AUTO: DONE" : L"AUTO: WAITING");

    wchar_t buffer[96]{};
    swprintf_s(buffer, L"Queue: %zu", weaponQueueCount);
    SetWindowTextW(g_statusQueue, buffer);

    SetWindowTextW(
        g_statusQPistol,
        qpistolNextB ? L"Q-Pistol next: Mode B" : L"Q-Pistol next: Mode A");
}

} // namespace

bool OverlayInitialize(const std::wstring& iniPath) {
    g_iniPath = iniPath;

    g_backgroundBrush = CreateSolidBrush(RGB(28, 31, 38));
    g_editBrush = CreateSolidBrush(RGB(43, 47, 57));

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = g_backgroundBrush;
    wc.lpszClassName = kOverlayClass;

    if (!RegisterClassExW(&wc)) {
        if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }
    }

    g_registered = true;
    return true;
}

void OverlayPump(
    bool playerReady,
    bool autoDone,
    std::size_t weaponQueueCount,
    bool qpistolNextB) {

    EnsureWindow();

    const bool insertDown =
        (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;

    if (insertDown && !g_insertWasDown && g_window) {
        if (g_visible) {
            HideOverlay();
        } else {
            ShowOverlay();
        }
    }
    g_insertWasDown = insertDown;

    MSG msg{};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (!g_visible) {
        return;
    }

    const ULONGLONG now = GetTickCount64();
    if (now - g_lastStatusUpdate >= 200) {
        UpdateStatus(playerReady, autoDone, weaponQueueCount, qpistolNextB);
        g_lastStatusUpdate = now;
    }
}

bool OverlayConsumeReloadRequest() {
    if (!g_reloadRequested) {
        return false;
    }

    g_reloadRequested = false;
    return true;
}

bool OverlayIsVisible() {
    return g_visible;
}

void OverlayShutdown() {
    HideOverlay();

    if (g_window) {
        DestroyWindow(g_window);
        g_window = nullptr;
    }

    if (g_font) {
        DeleteObject(g_font);
        g_font = nullptr;
    }

    if (g_editBrush) {
        DeleteObject(g_editBrush);
        g_editBrush = nullptr;
    }

    if (g_backgroundBrush) {
        DeleteObject(g_backgroundBrush);
        g_backgroundBrush = nullptr;
    }

    if (g_registered) {
        UnregisterClassW(kOverlayClass, GetModuleHandleW(nullptr));
        g_registered = false;
    }
}

} // namespace qp
