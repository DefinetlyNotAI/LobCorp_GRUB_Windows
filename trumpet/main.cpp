#include <windows.h>
#include <mmsystem.h>
#include <gdiplus.h>
#include <thread>
#include <atomic>
#include <random>
#include <memory>
#include <chrono>
#include <cwchar>
#include <iostream>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;
using namespace std::chrono_literals;

// ---------------- GLOBALS ----------------
std::atomic<int> currentTrumpet{0};
HWND overlayWnd = nullptr;
ULONG_PTR gdiplusToken;
std::atomic<BYTE> overlayAlpha{255};
std::atomic<bool> blinkState{true};

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

// ---------------- ENV FUNCTIONS ----------------
bool fileExists(const wchar_t* path) {
    const DWORD attribs = GetFileAttributesW(path);
    return attribs != INVALID_FILE_ATTRIBUTES && !(attribs & FILE_ATTRIBUTE_DIRECTORY);
}

// ---------------- AUDIO ----------------
void Play(int t) {
    if (t == 0) PlaySoundW(nullptr, nullptr, 0);
    else PlaySoundW(audioFiles[t], nullptr, SND_ASYNC | SND_FILENAME | SND_LOOP);
}

// ---------------- OVERLAY ----------------
void UpdateOverlay() {
    if (!currentImage || currentTrumpet == 0) {
        ShowWindow(overlayWnd, SW_HIDE);
        return;
    }

    ShowWindow(overlayWnd, SW_SHOW);

    const int w = GetSystemMetrics(SM_CXSCREEN);
    const int h = GetSystemMetrics(SM_CYSCREEN);

    HDC screenDC = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(screenDC);

    BITMAPINFO bminfo{};
    bminfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bminfo.bmiHeader.biWidth = w;
    bminfo.bmiHeader.biHeight = -h; // top-down
    bminfo.bmiHeader.biPlanes = 1;
    bminfo.bmiHeader.biBitCount = 32;
    bminfo.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hBmp = CreateDIBSection(memDC, &bminfo, DIB_RGB_COLORS, &bits, nullptr, 0);
    HGDIOBJ oldBmp = SelectObject(memDC, hBmp);

    Graphics g(memDC);
    g.Clear(Color(0, 0, 0, 0));

    if (currentImage) {
        const auto imgW = static_cast<float>(currentImage->GetWidth());
        const auto imgH = static_cast<float>(currentImage->GetHeight());
        const auto screenW = static_cast<float>(w);
        const auto screenH = static_cast<float>(h);

        const float scale = std::min(screenW / imgW, screenH / imgH);

        const int drawW = static_cast<int>(imgW * scale);
        const int drawH = static_cast<int>(imgH * scale);
        const int offsetX = (w - drawW) / 2;
        const int offsetY = (h - drawH) / 2;

        g.DrawImage(currentImage.get(), offsetX, offsetY, drawW, drawH);
    }

    POINT ptSrc{0,0};
    SIZE sizeWnd{w,h};
    POINT ptDest{0,0};
    BLENDFUNCTION blend{};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = overlayAlpha.load();
    blend.AlphaFormat = AC_SRC_ALPHA;

    UpdateLayeredWindow(overlayWnd, screenDC, &ptDest, &sizeWnd, memDC, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(memDC, oldBmp);
    DeleteObject(hBmp);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screenDC);
}

// ---------------- TRUMPET ----------------
void SetTrumpet(int t) {
    if (t == currentTrumpet) t = 0; // toggle off if already active
    currentTrumpet = t;
    Play(t);

    currentImage.reset();
    if (t > 0) {
        auto img = std::make_unique<Image>(overlayFiles[t]);
        if (img->GetLastStatus() == Ok) currentImage = std::move(img);
        else std::wcerr << L"Failed to load overlay: " << overlayFiles[t] << std::endl;
    }

    overlayAlpha = 255;
    UpdateOverlay();
}

// ---------------- BLINK LOOP ----------------
[[noreturn]] void BlinkLoop() {
    while (true) {
        if (currentTrumpet > 0) {
            blinkState = !blinkState;
            overlayAlpha = blinkState ? 255 : 10;
            UpdateOverlay();
        }
        std::this_thread::sleep_for(500ms);
    }
}

// ---------------- HOTKEYS ----------------
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_HOTKEY) {
        int id = static_cast<int>(wParam);
        if (id == 1) SetTrumpet(1); // F6
        else if (id == 2) SetTrumpet(2); // F7
        else if (id == 3) SetTrumpet(3); // F8
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCHITTEST) return HTTRANSPARENT; // click-through
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ---------------- MAIN ----------------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
    for (int i = 1; i <= 3; i++) {
        if (!fileExists(overlayFiles[i])) { std::wcerr << L"Overlay missing: " << overlayFiles[i] << std::endl; return 1; }
        if (!fileExists(audioFiles[i])) { std::wcerr << L"Audio missing: " << audioFiles[i] << std::endl; return 1; }
    }

    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    // Overlay window
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

    SetLayeredWindowAttributes(overlayWnd, 0, 255, LWA_ALPHA);
    ShowWindow(overlayWnd, SW_SHOW);

    // Main window for hotkeys
    WNDCLASSW wcMain{};
    wcMain.lpfnWndProc = MainWndProc;
    wcMain.hInstance = hInstance;
    wcMain.lpszClassName = L"MainWnd";
    RegisterClassW(&wcMain);

    HWND hwndMain = CreateWindowExW(0, L"MainWnd", L"", 0, 0,0,0,0,nullptr,nullptr,hInstance,nullptr);
    RegisterHotKey(hwndMain, 1, 0, VK_F6);
    RegisterHotKey(hwndMain, 2, 0, VK_F7);
    RegisterHotKey(hwndMain, 3, 0, VK_F8);

    std::thread(BlinkLoop).detach();

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    currentImage.reset();
    GdiplusShutdown(gdiplusToken);
    return 0;
}
