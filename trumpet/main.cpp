#include <windows.h>
#include <mmsystem.h>
#include <gdiplus.h>
#include <thread>
#include <atomic>
#include <memory>
#include <chrono>
#include <cwchar>
#include <iostream>
#include <random>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;
using namespace std::chrono_literals;

std::atomic<int> currentTrumpet{0};
HWND overlayWnd = nullptr;
ULONG_PTR gdiplusToken;
std::atomic<BYTE> overlayAlpha{255};

static std::wstring overlayFilesStorage[4];
static std::wstring audioFilesStorage[4];

const wchar_t* overlayFiles[4];
const wchar_t* audioFiles[4];

std::unique_ptr<Image> currentImage;

std::atomic dangerLevel{0};
std::mutex audioMutex;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution decayInterval(1000, 5000); // 1-5s

void clampDangerLevel() {
    if (dangerLevel < 0) dangerLevel = 0;
    if (dangerLevel > 100) dangerLevel = 100;
}

int getTrumpetFromDanger(int level) {
    if (level == 0) return 0;
    if (level >= 20 && level <= 49) return 1;
    if (level >= 50 && level <= 79) return 2;
    if (level >= 80 && level <= 100) return 3;
    return 0;
}

bool fileExists(const wchar_t* path) {
    const DWORD attr = GetFileAttributesW(path);
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

void initPaths() {
    wchar_t buffer[MAX_PATH];
    if (const DWORD ret = GetEnvironmentVariableW(L"USERPROFILE", buffer, MAX_PATH); ret == 0 || ret >= MAX_PATH) {
        std::wcerr << L"Failed to get USERPROFILE" << std::endl;
        return;
    }
    const std::wstring userProfile(buffer);

    overlayFilesStorage[0] = L"";
    overlayFilesStorage[1] = userProfile + L"\\.customTrumpets\\overlay\\trumpet1.png";
    overlayFilesStorage[2] = userProfile + L"\\.customTrumpets\\overlay\\trumpet2.png";
    overlayFilesStorage[3] = userProfile + L"\\.customTrumpets\\overlay\\trumpet3.png";

    audioFilesStorage[0] = L"";
    audioFilesStorage[1] = userProfile + L"\\.customTrumpets\\audio\\trumpet1.wav";
    audioFilesStorage[2] = userProfile + L"\\.customTrumpets\\audio\\trumpet2.wav";
    audioFilesStorage[3] = userProfile + L"\\.customTrumpets\\audio\\trumpet3.wav";

    for (int i = 0; i < 4; i++) {
        overlayFiles[i] = overlayFilesStorage[i].c_str();
        audioFiles[i] = audioFilesStorage[i].c_str();
    }
}

void fadeOutAudio() {
    // simple fade out using volume change
    for (int vol = 100; vol >= 0; vol -= 5) {
        waveOutSetVolume(nullptr, vol * 0xFFFF / 100);
        std::this_thread::sleep_for(10ms);
    }
    PlaySoundW(nullptr, nullptr, 0);
}

void PlayTrumpet(const int t) {
    std::lock_guard lock(audioMutex);
    if (t == 0) {
        fadeOutAudio();
        return;
    }
    PlaySoundW(audioFiles[t], nullptr, SND_ASYNC | SND_FILENAME | SND_LOOP);
}

void UpdateOverlay() {
    if (!overlayWnd) return;
    if (!currentImage || currentTrumpet == 0) {
        ShowWindow(overlayWnd, SW_HIDE);
        return;
    }

    ShowWindow(overlayWnd, SW_SHOW);

    const int w = GetSystemMetrics(SM_CXSCREEN);
    const int h = GetSystemMetrics(SM_CYSCREEN);

    HDC screenDC = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(screenDC);
    if (!memDC) return;

    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h; // top-down
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP bmp = CreateDIBSection(memDC, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!bmp) { DeleteDC(memDC); return; }

    HGDIOBJ oldBmp = SelectObject(memDC, bmp);

    Graphics g(memDC);
    g.SetCompositingMode(CompositingModeSourceOver);
    g.SetCompositingQuality(CompositingQualityHighQuality);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    g.SetSmoothingMode(SmoothingModeHighQuality);

    g.Clear(Color(0, 0, 0, 0));

    const auto imgW = static_cast<float>(currentImage->GetWidth());
    const auto imgH = static_cast<float>(currentImage->GetHeight());
    const float scale = std::min(static_cast<float>(w) / imgW, static_cast<float>(h) / imgH);
    const int drawW = static_cast<int>(imgW * scale);
    const int drawH = static_cast<int>(imgH * scale);
    const int offsetX = (w - drawW) / 2;
    const int offsetY = (h - drawH) / 2;

    g.DrawImage(currentImage.get(), offsetX, offsetY, drawW, drawH);

    POINT ptSrc{ 0,0 };
    SIZE size{ w,h };
    POINT ptDest{ 0,0 };
    BLENDFUNCTION blend{};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = overlayAlpha.load();
    blend.AlphaFormat = AC_SRC_ALPHA;

    UpdateLayeredWindow(overlayWnd, screenDC, &ptDest, &size, memDC, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(memDC, oldBmp);
    DeleteObject(bmp);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screenDC);
}

void SetTrumpet(int t) {
    if (t == currentTrumpet) t = 0; // reset if same

    currentTrumpet = t;

    if (t > 0) {
        if (auto img = std::make_unique<Image>(overlayFiles[t]); img->GetLastStatus() == Ok) currentImage = std::move(img);
        else std::wcerr << L"[ERROR] Failed to load overlay: " << overlayFiles[t] << std::endl;
    } else {
        currentImage.reset();
    }

    PlayTrumpet(t);
    overlayAlpha = 255;
    UpdateOverlay();
}

[[noreturn]] void BlinkLoop() {
    using clock = std::chrono::steady_clock;
    const auto startTime = clock::now();

    while (true) {
        if (currentTrumpet > 0) {
            auto now = clock::now();
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
            const double sine = sin(ms * 0.005);
            overlayAlpha = static_cast<BYTE>((sine * 0.5 + 0.5) * 255);
            UpdateOverlay();
        }
        std::this_thread::sleep_for(16ms);
    }
}

[[noreturn]] void DecayLoop() {
    std::uniform_int_distribution chance(0, 1);
    while (true) {
        int waitMs = decayInterval(gen);
        std::this_thread::sleep_for(std::chrono::milliseconds(waitMs));
        if (chance(gen) == 1) {
            --dangerLevel;
            clampDangerLevel();
        }
    }
}

void ReadEnvDanger() {
    wchar_t buffer[16];
    if (GetEnvironmentVariableW(L"DANGER_LEVEL", buffer, 16)) {
        const int val = _wtoi(buffer);
        dangerLevel = val;
        clampDangerLevel();
    }
}

LRESULT CALLBACK MainWndProc(HWND hwnd, const UINT msg, const WPARAM wParam, const LPARAM lParam) {
    if (msg == WM_HOTKEY) {
        if (wParam == 1) { dangerLevel = 49; clampDangerLevel(); SetTrumpet(1); }
        else if (wParam == 2) { dangerLevel = 79; clampDangerLevel(); SetTrumpet(2); }
        else if (wParam == 3) { dangerLevel = 100; clampDangerLevel(); SetTrumpet(3); }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK OverlayWndProc(HWND hwnd, const UINT msg, const WPARAM wParam, const LPARAM lParam) {
    if (msg == WM_NCHITTEST) return HTTRANSPARENT;
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
    initPaths();

    for (int i = 1; i <= 3; i++) {
        if (!fileExists(overlayFiles[i])) { std::wcerr << L"Overlay missing: " << overlayFiles[i] << std::endl; return 1; }
        if (!fileExists(audioFiles[i])) { std::wcerr << L"Audio missing: " << audioFiles[i] << std::endl; return 1; }
    }

    const GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    WNDCLASSW wcOverlay{};
    wcOverlay.lpfnWndProc = OverlayWndProc;
    wcOverlay.hInstance = hInstance;
    wcOverlay.lpszClassName = L"OverlayWnd";
    RegisterClassW(&wcOverlay);

    overlayWnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"OverlayWnd", L"",
        WS_POPUP,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        nullptr, nullptr, hInstance, nullptr
    );
    ShowWindow(overlayWnd, SW_SHOW);

    WNDCLASSW wcMain{};
    wcMain.lpfnWndProc = MainWndProc;
    wcMain.hInstance = hInstance;
    wcMain.lpszClassName = L"MainWnd";
    RegisterClassW(&wcMain);

    HWND hwndMain = CreateWindowExW(0, L"MainWnd", L"", 0,0,0,0,0,nullptr,nullptr,hInstance,nullptr);
    RegisterHotKey(hwndMain, 1, 0, VK_F6);
    RegisterHotKey(hwndMain, 2, 0, VK_F7);
    RegisterHotKey(hwndMain, 3, 0, VK_F8);

    std::thread(BlinkLoop).detach();
    std::thread(DecayLoop).detach();

    MSG msg{};
    std::atomic running{true};

    while (running) {
        ReadEnvDanger();
        if (const int targetTrumpet = getTrumpetFromDanger(dangerLevel); targetTrumpet > currentTrumpet) SetTrumpet(targetTrumpet);
        else if (currentTrumpet > 0 && dangerLevel == 0) SetTrumpet(0);

        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) running = false;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        std::this_thread::sleep_for(50ms);
    }

    currentImage.reset();
    GdiplusShutdown(gdiplusToken);
    return 0;
}