# Trumpet Info

Trumpet is a Windows overlay + audio alert app controlled by user-level `DANGER_LEVEL`.

## What it does

- Shows a transparent full-screen overlay image.
- Plays looping tiered audio.
- Maps `DANGER_LEVEL` to 3 trumpet tiers.
- Supports runtime hotkeys (`F6`, `F7`, `F8`).

Tier mapping:

- `0` => off
- `20-49` => tier 1
- `50-79` => tier 2
- `80-100` => tier 3

## Required media

Trumpet reads media from `%USERPROFILE%\.customTrumpets`:

- `overlay\trumpet1.png`, `overlay\trumpet2.png`, `overlay\trumpet3.png`
- `audio\trumpet1.wav`, `audio\trumpet2.wav`, `audio\trumpet3.wav`

If any of these files is missing, `Trumpet.exe` exits at startup.

## Build outputs

- `build/Trumpet.exe`
- `build/TrumpetUninstaller.exe`
- `build/Installer.exe`

Ordered all-at-once build target:

- `build_all` (`Trumpet -> TrumpetUninstaller -> Installer`)

## CMake presets

This repository includes `CMakePresets.json` with MSYS2 presets using:

- `C:/msys64/mingw64/bin/g++.exe`
- build dirs under `cmake/`:
  - `cmake/cmake-build-debug`
  - `cmake/cmake-build-release`

Recommended commands:

```powershell
cmake --preset msys2-release
cmake --build --preset build-all-release
cmake --build --preset autotest-release
```

## Custom media ZIP packaging mode

`TRUMPET_EMBED_CUSTOM_TRUMPETS`:

- `OFF` (default): uses external `customTrumpets.zip` next to `Installer.exe` (fast).
- `ON`: embeds ZIP into `CustomTrumpetsZip.h` (can be very slow with large ZIP files).

## Runtime controls

- `F6` => set tier 1 (`DANGER_LEVEL=49`)
- `F7` => set tier 2 (`DANGER_LEVEL=79`)
- `F8` => set tier 3 (`DANGER_LEVEL=100`)
- pressing same hotkey again toggles off

Set `DANGER_LEVEL` manually:

```powershell
[Environment]::SetEnvironmentVariable("DANGER_LEVEL", "79", "User")
```

## Uninstall

Run `TrumpetUninstaller.exe` or your generated uninstall workflow.

For full implementation details, see `docs/Trumpet - In detail.md`.

