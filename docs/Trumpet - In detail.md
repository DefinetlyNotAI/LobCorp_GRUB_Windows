# Trumpet - In Detail

This guide covers architecture, build pipeline, installer assets, and troubleshooting.

## 1) What Trumpet does

`Trumpet.exe` is a Windows background app that:

- draws a full-screen transparent PNG overlay,
- loops tiered WAV audio,
- reacts to user-level `DANGER_LEVEL` in `HKCU\Environment`,
- supports hotkeys `F6`, `F7`, `F8`.

Entry point: `trumpet/main.cpp` (`wWinMain`).

## 2) Runtime behavior

### Asset locations

At startup, `initPaths()` builds these paths under `%USERPROFILE%\.customTrumpets`:

- `overlay\trumpet1.png`, `overlay\trumpet2.png`, `overlay\trumpet3.png`
- `audio\trumpet1.wav`, `audio\trumpet2.wav`, `audio\trumpet3.wav`

If any required file is missing, startup aborts.

### Danger mapping

- `0` => off
- `20-49` => trumpet 1
- `50-79` => trumpet 2
- `80-100` => trumpet 3

### Control model

- `F6/F7/F8` set tier 1/2/3.
- Pressing the same active hotkey toggles off.
- Hotkey changes also write canonical values (`49`, `79`, `100`) to `DANGER_LEVEL`.
- Auto-upgrade only: app escalates to higher tiers automatically, but only resets to off when danger hits `0`.

## 3) Build targets

`CMakeLists.txt` defines:

- `Trumpet` => `build/Trumpet.exe`
- `TrumpetUninstaller` => `build/TrumpetUninstaller.exe`
- `Installer` => `build/Installer.exe`
- `GenerateInstallerAssets` => prepares generated headers + zip payload
- `build_all` => ordered flow: `Trumpet -> TrumpetUninstaller -> Installer`
- `autotest` => non-interactive smoke checks via CTest

## 4) Installer asset generation

Installer depends on generated files in `trumpet/generated`:

- `TrumpetExe.h`
- `UninstallerExe.h`
- `LicenseTxt.h`
- `ReadmeMd.h`
- `CustomTrumpetsZip.h`

`customTrumpets.zip` is generated in the active CMake build directory (for example `cmake/build`) and then embedded into `CustomTrumpetsZip.h`.

If your build appears "stuck" at:

`Embedding customTrumpets.zip -> CustomTrumpetsZip.h`

it is usually heavy processing for a large zip (not a deadlock).

## 5) Build directories inside `cmake/`

Yes, this is supported and now documented through `CMakePresets.json`.

Presets use:

- `cmake/debug`
- `cmake/build`

Compiler path in presets is pinned to:

- `C:/msys64/mingw64/bin/g++.exe`

## 6) Recommended commands

From repository root:

```powershell
cmake --preset msys2-release
cmake --build --preset build-all-release
cmake --build --preset autotest-release
```

Equivalent explicit build-dir commands:

```powershell
cmake -S . -B cmake/build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER="C:/msys64/mingw64/bin/g++.exe"
cmake --build cmake/build --target build_all
cmake --build cmake/build --target autotest
```

Debug configure/build:

```powershell
cmake -S . -B cmake/debug -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER="C:/msys64/mingw64/bin/g++.exe"
cmake --build cmake/debug
```

## 7) Installer behavior

`trumpet/installer.cpp` always extracts `.customTrumpets` from embedded `CustomTrumpetsZip.h` bytes.

## 8) Troubleshooting

- **Error 130 / `Interrupt` during embed**: build was interrupted (`Ctrl+C`) while generating large header output.
- **Slow build at 54% embed step**: this is usually large-zip embedding work; let it finish.
- **Installer extraction fails**: check PowerShell availability (`pwsh.exe` or `powershell.exe`).
- **Trumpet exits at startup**: missing files under `%USERPROFILE%\.customTrumpets`.
- **No overlay or audio**: verify PNG/WAV names and paths exactly match expected names.

## 9) Uninstall

Use `TrumpetUninstaller.exe` or your installer-generated uninstall workflow.

The uninstaller removes:

- `C:\Program Files\Trumpet`
- `%USERPROFILE%\.customTrumpets`
- startup and uninstall registry keys for current user.
