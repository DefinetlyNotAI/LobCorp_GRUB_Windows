#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <shlobj.h>
#include <iostream>

void KillProcessByName(const std::wstring& processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hSnapshot, &pe)) {
        do {
            if (processName == pe.szExeFile) {
                if (HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID)) {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                }
            }
        } while (Process32Next(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);
}

bool DeleteFolder(const std::wstring& path) {
    std::wstring cmd = L"rd /s /q \"" + path + L"\"";
    return system(std::string(cmd.begin(), cmd.end()).c_str()) == 0;
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    try {
        // Kill running Trumpet.exe
        KillProcessByName(L"Trumpet.exe");

        // Delete program folder
        DeleteFolder(L"C:\\Program Files\\Trumpet");

        // Delete user media folder
        WCHAR userPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PROFILE, nullptr, 0, userPath))) {
            std::wstring mediaPath = std::wstring(userPath) + L"\\.customTrumpets";
            DeleteFolder(mediaPath);
        }

        // Delete startup registry entry
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER,
                          L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                          0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            RegDeleteValueW(hKey, L"Trumpet");
            RegCloseKey(hKey);
        }

        // Delete uninstall registry key
        if (RegOpenKeyExW(HKEY_CURRENT_USER,
                          L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
                          0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
            RegDeleteKeyW(hKey, L"Trumpet");
            RegCloseKey(hKey);
        }

        MessageBoxW(nullptr, L"Trumpet removed successfully.", L"Uninstaller", MB_OK | MB_ICONINFORMATION);
    } catch (...) {
        MessageBoxW(nullptr, L"An error occurred during uninstall.", L"Uninstaller", MB_OK | MB_ICONERROR);
    }

    return 0;
}