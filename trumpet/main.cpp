#include <windows.h>
#include <mmsystem.h>
#include <gdiplus.h>
#include <thread>
#include <atomic>
#include <random>
#include <memory>
#include <chrono>
#include <cwchar>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;
using namespace std::chrono_literals;

// ---------------- GLOBALS ----------------
std::atomic currentTrumpet{0};
std::atomic lastSeenEnv{-1};
HWND overlayWnd = nullptr;
ULONG_PTR gdiplusToken;
std::atomic<BYTE> overlayAlpha{255};

const wchar_t* overlayFiles[] = {
    L"",
    L"C:\\Users\\Hp\\.customTrumpets\\overlay\\trumpet1.png",
    L"C:\\Users\\Hp\\.customTrumpets\\overlay\\trumpet2.png",
    L"C:\\Users\\Hp\\.customTrumpets\\overlay\\trumpet3.png"
};

const wchar_t* audioFiles[] = {
    L"",
    L"C:\\Users\\Hp\\.customTrumpets\\audio\\trumpet1.wav",
    L"C:\\Users\\Hp\\.customTrumpets\\audio\\trumpet2.wav",
    L"C:\\Users\\Hp\\.customTrumpets\\audio\\trumpet3.wav"
};

std::unique_ptr<Image> currentImage;
std::atomic blinkState{true};

// ---------------- ENV FUNCTIONS ----------------
int ReadEnv() {
    // ReSharper disable once CppLocalVariableMayBeConst
    wchar_t buf[16]{};
    DWORD size = sizeof(buf);
    HKEY hKey{};
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Environment", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExW(hKey, L"DANGER_LEVEL", nullptr, nullptr, reinterpret_cast<LPBYTE>(buf), &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return _wtoi(buf);
        }
        RegCloseKey(hKey);
    }
    return 0;
}

void WriteEnv(int val) {
    val = std::clamp(val, 0, 100);
    wchar_t buf[16];
    swprintf(buf, 16, L"%d", val);

    HKEY hKey{};
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Environment", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, L"DANGER_LEVEL", 0, REG_SZ,
            reinterpret_cast<BYTE*>(buf), (wcslen(buf)+1)*sizeof(wchar_t));
        RegCloseKey(hKey);
    }

    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>(L"Environment"),
        SMTO_ABORTIFHUNG, 100, nullptr);
}

// ---------------- AUDIO ----------------
void Play(int t) {
    if (t == 0) PlaySoundW(nullptr, nullptr, 0);
    else PlaySoundW(audioFiles[t], nullptr, SND_ASYNC | SND_FILENAME | SND_LOOP);
}

// ---------------- OVERLAY ----------------
void CreateOverlay() {
    WNDCLASSW wc{};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"OverlayWnd";
    RegisterClassW(&wc);

    overlayWnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"OverlayWnd", L"",
        WS_POPUP,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        nullptr, nullptr, wc.hInstance, nullptr
    );

    SetLayeredWindowAttributes(overlayWnd, 0, 255, LWA_ALPHA);
    ShowWindow(overlayWnd, SW_SHOW);
}

void UpdateOverlay() {
    if (!currentImage || currentTrumpet == 0) {
        ShowWindow(overlayWnd, SW_HIDE);
        return;
    }

    ShowWindow(overlayWnd, SW_SHOW);

    HDC screenDC = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(screenDC);

    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);

    BITMAPINFO bminfo{};
    bminfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bminfo.bmiHeader.biWidth = w;
    bminfo.bmiHeader.biHeight = -h; // top-down
    bminfo.bmiHeader.biPlanes = 1;
    bminfo.bmiHeader.biBitCount = 32;
    bminfo.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    // ReSharper disable once CppLocalVariableMayBeConst
    HBITMAP hBmp = CreateDIBSection(memDC, &bminfo, DIB_RGB_COLORS, &bits, nullptr, 0);
    // ReSharper disable once CppLocalVariableMayBeConst
    HGDIOBJ oldBmp = SelectObject(memDC, hBmp);

    Graphics g(memDC);
    g.Clear(Color(0, 0, 0, 0));
    g.DrawImage(currentImage.get(), 0, 0, w, h);

    POINT ptSrc{0,0};
    SIZE sizeWnd{w,h};
    POINT ptDest{0,0};
    BLENDFUNCTION blend{};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = overlayAlpha;
    blend.AlphaFormat = AC_SRC_ALPHA;

    UpdateLayeredWindow(overlayWnd, screenDC, &ptDest, &sizeWnd, memDC, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(memDC, oldBmp);
    DeleteObject(hBmp);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screenDC);
}

// ---------------- TRUMPET ----------------
void SetTrumpet(int t) {
    if (t == currentTrumpet) return;

    currentTrumpet = t;
    Play(t);

    currentImage.reset();
    if (t > 0) currentImage = std::make_unique<Image>(overlayFiles[t]);

    if (t == 0) ShowWindow(overlayWnd, SW_HIDE);
    else ShowWindow(overlayWnd, SW_SHOW);

    overlayAlpha = 255;
    UpdateOverlay();
}

void Evaluate(int v) {
    v = std::clamp(v, 0, 100);

    int target = 0;
    if (v > 80) target = 3;
    else if (v > 50) target = 2;
    else if (v > 10) target = 1;

    int cur = currentTrumpet;
    if (target > cur) SetTrumpet(target);
    else if (v == 0 && cur != 0) SetTrumpet(0);
}

// ---------------- LOOPS ----------------
[[noreturn]] void EnvLoop() {
    while (true) {
        if (const int v = ReadEnv(); v != lastSeenEnv) {
            lastSeenEnv = v;
            Evaluate(v);
        }
        std::this_thread::sleep_for(300ms);
    }
}

[[noreturn]] void DecayLoop() {
    std::mt19937 rng(GetTickCount());
    std::uniform_int_distribution delay(1000,5000);
    std::uniform_int_distribution coin(0,1);

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay(rng)));
        if (coin(rng) == 0) continue;

        const int before = ReadEnv();
        if (before == 0) continue;
        if (ReadEnv() == before) WriteEnv(before - 1);
    }
}

// ---------------- BLINK LOOP ----------------
[[noreturn]] void BlinkLoop() {
    while (true) {
        if (currentTrumpet > 0) {
            blinkState = !blinkState;
            overlayAlpha = blinkState ? 255 : 0;
            UpdateOverlay();
        }
        std::this_thread::sleep_for(500ms);
    }
}

// ---------------- HOTKEYS ----------------
// ReSharper disable once CppParameterMayBeConst
LRESULT CALLBACK WndProc(HWND hwnd, const UINT msg, const WPARAM wParam, const LPARAM lParam) {
    if (msg == WM_HOTKEY) {
        const int id = static_cast<int>(wParam);
        const int cur = ReadEnv();

        if (id == 1) { // F6
            if (cur >=1 && cur <=49) WriteEnv(0);
            else WriteEnv(49);
        }
        else if (id == 2) { // F7
            if (cur >=50 && cur <=79) WriteEnv(0);
            else WriteEnv(79);
        }
        else if (id == 3) { // F8
            if (cur >=80 && cur <=100) WriteEnv(0);
            else WriteEnv(100);
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ---------------- MAIN ----------------
// ReSharper disable once CppParameterMayBeConst
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
    const GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    CreateOverlay();

    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MainWnd";
    RegisterClassW(&wc);

    // ReSharper disable once CppLocalVariableMayBeConst
    HWND hwnd = CreateWindowExW(0, L"MainWnd", L"", 0,
                                0,0,0,0, nullptr,nullptr, hInstance, nullptr);

    RegisterHotKey(hwnd, 1, 0, VK_F6);
    RegisterHotKey(hwnd, 2, 0, VK_F7);
    RegisterHotKey(hwnd, 3, 0, VK_F8);

    std::thread(EnvLoop).detach();
    std::thread(DecayLoop).detach();
    std::thread(BlinkLoop).detach();

    MSG msg{};
    while (GetMessageW(&msg,nullptr,0,0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    currentImage.reset();
    GdiplusShutdown(gdiplusToken);
    return 0;
}
