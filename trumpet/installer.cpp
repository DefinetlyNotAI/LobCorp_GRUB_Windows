#include <windows.h>
#include <shlobj.h>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <iostream>
#include <limits>

namespace fs = std::filesystem;

// Include embedded headers
#include "TrumpetExe.h"
#include "UninstallerExe.h"
#include "LicenseTxt.h"
#include "ReadmeMd.h"
#include "CustomTrumpetsZip.h"

bool WriteFile(const fs::path& path, const unsigned char* data, const size_t len) {
    try {
        if (len > static_cast<size_t>(std::numeric_limits<std::streamsize>::max())) return false;
        fs::create_directories(path.parent_path());
        std::ofstream out(path, std::ios::binary);
        if (!out) return false;
        out.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(len));
        return true;
    } catch (...) { return false; }
}

std::wstring EscapePsSingleQuoted(const std::wstring& value) {
    std::wstring escaped;
    escaped.reserve(value.size());
    for (const wchar_t ch : value) {
        if (ch == L'\'') {
            escaped += L"''";
        } else {
            escaped += ch;
        }
    }
    return escaped;
}

bool ExtractZip(const unsigned char* data, const size_t len, const fs::path& outDir) {
    if (len > static_cast<size_t>(std::numeric_limits<std::streamsize>::max())) return false;

    // Write temp zip file
    const fs::path tempZip = fs::temp_directory_path() / "customTrumpets.zip";
    std::ofstream tmp(tempZip, std::ios::binary);
    if (!tmp) return false;
    tmp.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(len));
    if (!tmp.good()) return false;
    tmp.close();

    const std::wstring zipArg = EscapePsSingleQuoted(tempZip.wstring());
    const std::wstring outArg = EscapePsSingleQuoted(outDir.wstring());

#ifdef TRUMPET_POWERSHELL_EXE
    const std::wstring psExe = TRUMPET_POWERSHELL_EXE;
#else
    const std::wstring psExe = L"powershell.exe";
#endif

    const std::wstring cmd = L"\"" + psExe +
                             L"\" -NoProfile -ExecutionPolicy Bypass -Command \"Expand-Archive -LiteralPath '" +
                             zipArg + L"' -DestinationPath '" + outArg + L"' -Force\"";
    const int exitCode = _wsystem(cmd.c_str());

    fs::remove(tempZip);
    return exitCode == 0;
}


// Request admin rights via manifest: simpler if exe is built with requireAdministrator in manifest
bool IsAdmin() {
    BOOL fAdmin = FALSE;
    PSID pAdminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0,0,0,0,0,0, &pAdminGroup))
    {
        CheckTokenMembership(nullptr, pAdminGroup, &fAdmin);
        FreeSid(pAdminGroup);
    }
    return fAdmin;
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {

    if (!IsAdmin()) {
        MessageBoxW(nullptr, L"Please run as administrator.", L"Installer", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Program files path
    WCHAR programDir[MAX_PATH];
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_PROGRAM_FILES, nullptr, 0, programDir))) {
        MessageBoxW(nullptr, L"Failed to get Program Files path.", L"Installer", MB_OK | MB_ICONERROR);
        return 1;
    }
    fs::path targetDir = fs::path(programDir) / "Trumpet";

    // Write embedded files
    if (!WriteFile(targetDir / "Trumpet.exe", TrumpetExe, TrumpetExe_len)) {
        MessageBoxW(nullptr, L"Failed to write Trumpet.exe", L"Installer", MB_OK | MB_ICONERROR);
        return 1;
    }
    if (!WriteFile(targetDir / "UninstallTrumpet.exe", UninstallerExe, UninstallerExe_len)) {
        MessageBoxW(nullptr, L"Failed to write Uninstaller.exe", L"Installer", MB_OK | MB_ICONERROR);
        return 1;
    }
    if (!WriteFile(targetDir / "LICENSE", LicenseTxt, LicenseTxt_len)) {
        MessageBoxW(nullptr, L"Failed to write LICENSE", L"Installer", MB_OK | MB_ICONERROR);
        return 1;
    }
    if (!WriteFile(targetDir / "README.md", ReadmeMd, ReadmeMd_len)) {
        MessageBoxW(nullptr, L"Failed to write README.md", L"Installer", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Extract .customTrumpets folder
    wchar_t userProfile[MAX_PATH];
    if (!GetEnvironmentVariableW(L"USERPROFILE", userProfile, MAX_PATH)) {
        MessageBoxW(nullptr, L"Failed to resolve USERPROFILE", L"Installer", MB_OK | MB_ICONERROR);
        return 1;
    }
    fs::path userCustom = fs::path(userProfile) / ".customTrumpets";
    if (!ExtractZip(CustomTrumpetsZip, CustomTrumpetsZip_len, userCustom)) {
        MessageBoxW(nullptr, L"Failed to extract .customTrumpets", L"Installer", MB_OK | MB_ICONERROR);
        return 1;
    }
    MessageBoxW(nullptr, L"Installation complete.", L"Installer", MB_OK | MB_ICONINFORMATION);
    return 0;
}