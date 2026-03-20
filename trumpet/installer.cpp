#include <windows.h>
#include <shlobj.h>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <iostream>
#include <limits>
#include "resources/resource_ids.h"

namespace fs = std::filesystem;

// Include embedded headers
#include "TrumpetExe.h"
#include "UninstallerExe.h"
#include "LicenseTxt.h"
#include "ReadmeMd.h"

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

    std::error_code dirEc;
    fs::create_directories(outDir, dirEc);
    if (dirEc) {
        fs::remove(tempZip);
        return false;
    }

    const std::wstring zipArg = EscapePsSingleQuoted(tempZip.wstring());
    const std::wstring outArg = EscapePsSingleQuoted(outDir.wstring());

    auto runExtractWithPs = [&](const std::wstring& psExe) -> bool {
        std::wstring script =
                L"try { Expand-Archive -LiteralPath '" + zipArg + L"' -DestinationPath '" + outArg +
                L"' -Force; exit 0 } catch { exit 1 }";
        std::wstring args = L" -NoProfile -NonInteractive -ExecutionPolicy Bypass -Command \"" + script + L"\"";

        STARTUPINFOW si{};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};

        std::vector<wchar_t> cmdLine(args.begin(), args.end());
        cmdLine.push_back(L'\0');

        if (!CreateProcessW(psExe.c_str(), cmdLine.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
            return false;
        }

        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exitCode = 1;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return exitCode == 0;
    };

    bool extracted = false;

#ifdef TRUMPET_POWERSHELL_EXE
    extracted = runExtractWithPs(TRUMPET_POWERSHELL_EXE);
#endif
    if (!extracted) extracted = runExtractWithPs(L"powershell.exe");
    if (!extracted) extracted = runExtractWithPs(L"pwsh.exe");

    fs::remove(tempZip);
    return extracted;
}

bool LoadCustomTrumpetsZip(std::vector<unsigned char>& outData) {
    HRSRC resourceInfo = FindResourceW(nullptr, MAKEINTRESOURCEW(IDR_CUSTOM_TRUMPETS_ZIP), RT_RCDATA);
    if (!resourceInfo) return false;

    const DWORD resourceSize = SizeofResource(nullptr, resourceInfo);
    if (resourceSize == 0) return false;

    HGLOBAL loadedResource = LoadResource(nullptr, resourceInfo);
    if (!loadedResource) return false;

    const void* resourceData = LockResource(loadedResource);
    if (!resourceData) return false;

    outData.assign(static_cast<const unsigned char*>(resourceData),
                   static_cast<const unsigned char*>(resourceData) + resourceSize);
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
    wchar_t userProfile[MAX_PATH];
    if (!GetEnvironmentVariableW(L"USERPROFILE", userProfile, MAX_PATH)) {
        MessageBoxW(nullptr, L"Failed to resolve USERPROFILE", L"Installer", MB_OK | MB_ICONERROR);
        return 1;
    }
    fs::path userCustom = fs::path(userProfile) / ".customTrumpets";
    std::vector<unsigned char> customTrumpetsZip;
    if (!LoadCustomTrumpetsZip(customTrumpetsZip)) {
        MessageBoxW(nullptr, L"Failed to load .customTrumpets archive resource", L"Installer", MB_OK | MB_ICONERROR);
        return 1;
    }

    if (!ExtractZip(customTrumpetsZip.data(), customTrumpetsZip.size(), userCustom)) {
        MessageBoxW(nullptr, L"Failed to extract .customTrumpets", L"Installer", MB_OK | MB_ICONERROR);
        return 1;
    }
    MessageBoxW(nullptr, L"Installation complete.", L"Installer", MB_OK | MB_ICONINFORMATION);
    return 0;
}