#include <windows.h>
#include <shlobj.h>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <iostream>
#include <zip.h>

namespace fs = std::filesystem;

// Include embedded headers
#include "TrumpetExe.h"
#include "UninstallerExe.h"
#include "LicenseTxt.h"
#include "ReadmeMd.h"
#include "CustomTrumpetsZip.h" // Embed your .customTrumpets.zip as bytes

bool WriteFile(const fs::path& path, const unsigned char* data, const size_t len) {
    try {
        fs::create_directories(path.parent_path());
        std::ofstream out(path, std::ios::binary);
        if (!out) return false;
        out.write(reinterpret_cast<const char*>(data), len);
        return true;
    } catch (...) { return false; }
}

// Simple extraction of ZIP using minizip / libzip
bool ExtractZip(const unsigned char* data, const size_t len, const fs::path& outDir) {
    // Write temp zip file
    const fs::path tempZip = fs::temp_directory_path() / "customTrumpets.zip";
    std::ofstream tmp(tempZip, std::ios::binary);
    tmp.write(reinterpret_cast<const char*>(data), len);
    tmp.close();

    // Use Windows Shell application to unzip (simpler than libzip if no extra deps)
    std::wstring cmd = L"powershell -Command \"Expand-Archive -Force '" +
                       tempZip.wstring() + L"' '" + outDir.wstring() + L"'\"";
    system(std::string(cmd.begin(), cmd.end()).c_str());

    fs::remove(tempZip);
    return true;
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
    fs::path userCustom = fs::path(getenv("USERPROFILE")) / ".customTrumpets";
    if (!ExtractZip(CustomTrumpetsZip, CustomTrumpetsZip_len, userCustom)) {
        MessageBoxW(nullptr, L"Failed to extract .customTrumpets", L"Installer", MB_OK | MB_ICONERROR);
        return 1;
    }
    MessageBoxW(nullptr, L"Installation complete.", L"Installer", MB_OK | MB_ICONINFORMATION);
    return 0;
}