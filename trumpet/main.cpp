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
#include <cmath>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;
using namespace std::chrono_literals;

std::atomic currentTrumpet{0};
std::atomic<BYTE> overlayAlpha{255};
HWND overlayWnd = nullptr;
ULONG_PTR gdiplusToken;

static std::wstring overlayFilesStorage[4];
static std::wstring audioFilesStorage[4];
const wchar_t *overlayFiles[4];
const wchar_t *audioFiles[4];

std::unique_ptr<Image> currentImage;

std::mt19937 rng{std::random_device{}()};
std::uniform_int_distribution decayInterval{1000, 5000};
std::uniform_int_distribution decayChance{0, 1};

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

bool fileExists(const wchar_t *path) {
    const DWORD attr = GetFileAttributesW(path);
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

// Read User-scoped DANGER_LEVEL from registry
int readDangerLevelUser() {
    DWORD type = 0, size = 0;
    LONG res = RegGetValueW(HKEY_CURRENT_USER,
                            L"Environment",
                            L"DANGER_LEVEL",
                            RRF_RT_REG_SZ,
                            &type,
                            nullptr,
                            &size);
    if (res != ERROR_SUCCESS) return 0;
    std::wstring buf(size / sizeof(wchar_t), L'\0');
    res = RegGetValueW(HKEY_CURRENT_USER,
                       L"Environment",
                       L"DANGER_LEVEL",
                       RRF_RT_REG_SZ,
                       nullptr,
                       &buf[0],
                       &size);
    if (res != ERROR_SUCCESS) return 0;
    int val = _wtoi(buf.c_str());
    if (val < 0) val = 0;
    if (val > 100) val = 100;
    return val;
}

// Write both process and user-scoped DANGER_LEVEL
void writeDangerLevel(int val) {
    if (val < 0) val = 0;
    if (val > 100) val = 100;

    // Process env
    wchar_t buf[16];
    swprintf(buf, 16, L"%d", val);
    SetEnvironmentVariableW(L"DANGER_LEVEL", buf);

    // User env
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Environment", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, L"DANGER_LEVEL", 0, REG_SZ, reinterpret_cast<const BYTE *>(buf),
                       (DWORD) ((wcslen(buf) + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);
    }
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
    bi.bmiHeader.biHeight = -h;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void *bits = nullptr;
    HBITMAP bmp = CreateDIBSection(memDC, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!bmp) {
        DeleteDC(memDC);
        return;
    }

    HGDIOBJ oldBmp = SelectObject(memDC, bmp);

    Graphics g(memDC);
    g.SetCompositingMode(CompositingModeSourceOver);
    g.SetCompositingQuality(CompositingQualityHighQuality);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    g.SetSmoothingMode(SmoothingModeHighQuality);

    g.Clear(Color(0, 0, 0, 0));

    if (currentImage) {
        const auto imgW = static_cast<float>(currentImage->GetWidth());
        const auto imgH = static_cast<float>(currentImage->GetHeight());
        const float scale = std::min(static_cast<float>(w) / imgW, static_cast<float>(h) / imgH);
        const int drawW = static_cast<int>(imgW * scale);
        const int drawH = static_cast<int>(imgH * scale);
        const int offsetX = (w - drawW) / 2;
        const int offsetY = (h - drawH) / 2;
        g.DrawImage(currentImage.get(), offsetX, offsetY, drawW, drawH);
    }

    POINT ptSrc{0, 0};
    SIZE size{w, h};
    POINT ptDest{0, 0};
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

// Set trumpet with hotkey override option
void SetTrumpet(int t, const bool hotkeyOverride = false) {
    if (const int prev = currentTrumpet; hotkeyOverride && t == prev) t = 0; // reset if same trumpet hotkey pressed
    currentTrumpet = t;

    if (hotkeyOverride) {
        int val = 0;
        if (t == 1) val = 49;
        if (t == 2) val = 79;
        if (t == 3) val = 100;
        writeDangerLevel(val);
    }

    currentImage.reset();
    if (t > 0) {
        if (auto img = std::make_unique<Image>(overlayFiles[t]); img->GetLastStatus() == Ok)
            currentImage = std::move(img);
        else std::wcerr << L"[ERROR] Failed to load overlay: " << overlayFiles[t] << std::endl;
    }

    overlayAlpha = 255;
    UpdateOverlay();
}

[[noreturn]] void BlinkLoop() {
    using clock = std::chrono::steady_clock;
    while (true) {
        if (currentTrumpet > 0) {
            auto now = clock::now();
            const double millis = static_cast<double>(
                std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
            const double sine = sin(millis * 0.005);
            overlayAlpha = static_cast<BYTE>((sine * 0.5 + 0.5) * 255);
            UpdateOverlay();
        }
        std::this_thread::sleep_for(16ms);
    }
}

[[noreturn]] void DecayLoop() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(decayInterval(rng)));
        if (decayChance(rng)) {
            if (const int val = readDangerLevelUser(); val > 0) writeDangerLevel(val - 1);
        }
    }
}

[[noreturn]] void AudioLoop() {
    int lastTrumpet = 0;
    while (true) {
        if (const int trumpet = currentTrumpet.load(); trumpet != lastTrumpet) {
            // Purge any previous loop before starting a new one.
            PlaySoundW(nullptr, nullptr, SND_PURGE);
            std::this_thread::sleep_for(10ms);

            if (trumpet > 0) {
                const auto &path = audioFilesStorage[trumpet];
                if (path.empty() || !fileExists(path.c_str())) {
                    std::wcerr << L"[ERROR] Audio file missing/unreadable: " << path << std::endl;
                } else {
                    if (constexpr DWORD primaryFlags = SND_ASYNC | SND_FILENAME | SND_LOOP | SND_SYSTEM; !PlaySoundW(path.c_str(), nullptr, primaryFlags)) {
                        const DWORD err = GetLastError();
                        std::wcerr << L"[ERROR] PlaySound primary failed (" << err << L"): " << path << std::endl;

                        // Fallback for systems where SND_SYSTEM is not honored as expected.
                        constexpr DWORD fallbackFlags = SND_ASYNC | SND_FILENAME | SND_LOOP;
                        if (!PlaySoundW(path.c_str(), nullptr, fallbackFlags)) {
                            const DWORD fallbackErr = GetLastError();
                            std::wcerr << L"[ERROR] PlaySound fallback failed (" << fallbackErr << L"): " << path <<
                                    std::endl;

                            std::wstring dbg = L"[Trumpet] Audio playback failed for: " + path + L"\n";
                            OutputDebugStringW(dbg.c_str());
                        }
                    }
                }
            }

            lastTrumpet = trumpet;
        }

        std::this_thread::sleep_for(50ms);
    }
}

int trumpetFromDanger(const int val) {
    if (val == 0) return 0;
    if (val >= 20 && val <= 49) return 1;
    if (val >= 50 && val <= 79) return 2;
    if (val >= 80 && val <= 100) return 3;
    return 0;
}

LRESULT CALLBACK MainWndProc(HWND hwnd, const UINT msg, const WPARAM wParam, const LPARAM lParam) {
    if (msg == WM_HOTKEY) {
        if (wParam == 1) SetTrumpet(1, true);
        else if (wParam == 2) SetTrumpet(2, true);
        else if (wParam == 3) SetTrumpet(3, true);
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
        if (!fileExists(overlayFiles[i])) {
            std::wcerr << L"Overlay missing: " << overlayFiles[i] << std::endl;
            return 1;
        }
        if (!fileExists(audioFiles[i])) {
            std::wcerr << L"Audio missing: " << audioFiles[i] << std::endl;
            return 1;
        }
    }

    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    WNDCLASSW wcOverlay{};
    wcOverlay.lpfnWndProc = OverlayWndProc;
    wcOverlay.hInstance = hInstance;
    wcOverlay.lpszClassName = L"OverlayWnd";
    RegisterClassW(&wcOverlay);

    overlayWnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"OverlayWnd", L"", WS_POPUP,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        nullptr, nullptr, hInstance, nullptr
    );
    ShowWindow(overlayWnd,SW_SHOW);

    WNDCLASSW wcMain{};
    wcMain.lpfnWndProc = MainWndProc;
    wcMain.hInstance = hInstance;
    wcMain.lpszClassName = L"MainWnd";
    RegisterClassW(&wcMain);

    HWND hwndMain = CreateWindowExW(0, L"MainWnd", L"", 0, 0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);
    RegisterHotKey(hwndMain, 1, 0,VK_F6);
    RegisterHotKey(hwndMain, 2, 0,VK_F7);
    RegisterHotKey(hwndMain, 3, 0,VK_F8);

    std::thread(BlinkLoop).detach();
    std::thread(DecayLoop).detach();
    std::thread(AudioLoop).detach();
    for (int i = 1; i <= 3; i++) std::wcout << L"Audio path " << i << L": " << audioFilesStorage[i] << std::endl;

    MSG msg{};
    // ReSharper disable once CppDFAEndlessLoop
    while (true) {
        const int danger = readDangerLevelUser();
        const int targetTrumpet = trumpetFromDanger(danger);

        if (const int current = currentTrumpet.load(); targetTrumpet > current) {
            // Only upgrade automatically when danger crosses into a higher band.
            SetTrumpet(targetTrumpet);
        } else if (danger == 0 && current > 0) {
            // Sticky mode: keep current trumpet until danger fully reaches zero.
            SetTrumpet(0);
        }

        while (PeekMessageW(&msg, nullptr, 0, 0,PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        std::this_thread::sleep_for(200ms);
    }
}
