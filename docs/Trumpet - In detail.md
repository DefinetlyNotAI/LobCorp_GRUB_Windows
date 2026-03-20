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
- `customTrumpets.zip`
- optional: `CustomTrumpetsZip.h`

### Important option for large media packs

`TRUMPET_EMBED_CUSTOM_TRUMPETS` controls whether `customTrumpets.zip` is converted to `CustomTrumpetsZip.h`.

- `OFF` (default): fast build, zip copied next to `Installer.exe`, installer extracts from external `customTrumpets.zip`.
- `ON`: zip is embedded into C++ header. This can be very slow for large zips.

If your build appears "stuck" at:

`Embedding customTrumpets.zip -> CustomTrumpetsZip.h`

it is usually heavy processing for a large zip (not a deadlock). Use `TRUMPET_EMBED_CUSTOM_TRUMPETS=OFF` unless single-file installer embedding is required.

## 5) Build directories inside `cmake/`

Yes, this is supported and now documented through `CMakePresets.json`.

Presets use:

- `cmake/cmake-build-debug`
- `cmake/cmake-build-release`

Compiler path in presets is pinned to:

- `C:/msys64/mingw64/bin/g++.exe`

## 6) Recommended commands

From repository root:

```powershell
cmake --preset msys2-release
cmake --build --preset build-all-release
cmake --build --preset autotest-release
```

Debug configure/build:

```powershell
cmake --preset msys2-debug
cmake --build --preset build-debug
```

If you explicitly want embedded zip mode:

```powershell
cmake --preset msys2-release -DTRUMPET_EMBED_CUSTOM_TRUMPETS=ON
cmake --build --preset build-all-release
```

## 7) Installer behavior

`trumpet/installer.cpp` now supports two extraction modes:

- embedded mode (`TRUMPET_EMBED_CUSTOM_TRUMPETS=ON`): extract from `CustomTrumpetsZip.h` bytes,
- external mode (default): extract from `customTrumpets.zip` next to `Installer.exe`.

The build copies `customTrumpets.zip` next to `Installer.exe` automatically.

## 8) Troubleshooting

- **Error 130 / `Interrupt` during embed**: build was interrupted (`Ctrl+C`) while generating large header output.
- **Slow build at 54% embed step**: disable embedded zip mode (`TRUMPET_EMBED_CUSTOM_TRUMPETS=OFF`).
- **Installer extraction fails**: check PowerShell availability (`pwsh.exe` or `powershell.exe`) and verify `customTrumpets.zip` exists beside `Installer.exe`.
- **Trumpet exits at startup**: missing files under `%USERPROFILE%\.customTrumpets`.
- **No overlay or audio**: verify PNG/WAV names and paths exactly match expected names.

## 9) Uninstall

Use `TrumpetUninstaller.exe` or your installer-generated uninstall workflow.

The uninstaller removes:

- `C:\Program Files\Trumpet`
- `%USERPROFILE%\.customTrumpets`
- startup and uninstall registry keys for current user.
