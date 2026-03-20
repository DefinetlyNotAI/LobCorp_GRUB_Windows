#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <shlobj.h>
#include <filesystem>

namespace fs = std::filesystem;

bool IsAdmin() {
    BOOL fAdmin = FALSE;
    PSID pAdminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pAdminGroup)) {
        CheckTokenMembership(nullptr, pAdminGroup, &fAdmin);
        FreeSid(pAdminGroup);
    }
    return fAdmin != FALSE;
}

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
    std::error_code ec;
    fs::remove_all(fs::path(path), ec);
    return !ec;
}

void RemoveUninstallRegistration() {
    RegDeleteTreeW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Trumpet");
    RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Trumpet");
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    if (!IsAdmin()) {
        MessageBoxW(nullptr, L"Please run as administrator.", L"Uninstaller", MB_OK | MB_ICONERROR);
        return 1;
    }

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

        // Delete uninstall registration shown in Windows Settings.
        RemoveUninstallRegistration();

        MessageBoxW(nullptr, L"Trumpet removed successfully.", L"Uninstaller", MB_OK | MB_ICONINFORMATION);
    } catch (...) {
        MessageBoxW(nullptr, L"An error occurred during uninstall.", L"Uninstaller", MB_OK | MB_ICONERROR);
    }

    return 0;
}