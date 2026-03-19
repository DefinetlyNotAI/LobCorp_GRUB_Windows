# Trumpet - Detailed Guide

This document explains how Trumpet works internally, how to build it, and how to install it safely.
> Main installer app: `scripts/installer.ps1`

## 1) Purpose and behavior

Trumpet is a native Windows background app that combines:

- a transparent fullscreen overlay (PNG)
- looped system audio playback (WAV)
- hotkey + registry-driven state changes based on `DANGER_LEVEL`
  The executable entry point is `trumpet/main.cpp` (`wWinMain`).

It takes inspiration from `Lobotomy Corp` warning systems.

## 2) Installer-first workflow

The expected user workflow is installer-first via `scripts/installer.ps1`.

### What `scripts/installer.ps1` does

1. Creates `C:\Program Files\Trumpet` if missing.
2. Downloads:
    - `Trumpet.exe`
    - `LICENSE`
    - `README.md`
3. Downloads `Trumpets.media.zip` and extracts into `%USERPROFILE%`.
4. Optionally creates startup key:
    - Path: `HKCU:\Software\Microsoft\Windows\CurrentVersion\Run`
    - Name: `Trumpet`
    - Value: `C:\Program Files\Trumpet\Trumpet.exe`
5. Writes an uninstaller script to:
    - `C:\Program Files\Trumpet\uninstall-trumpet.ps1`

### Why installer-first matters

`trumpet/main.cpp` hardcodes media lookup under `%USERPROFILE%\\.customTrumpets\\...`. The installer guarantees that
folder structure and files exist.

## 3) Runtime data model

Trumpet uses three data inputs:

- **Overlay assets** (`.png`)
- **Audio assets** (`.wav`)
- **Danger level** (`HKCU\\Environment\\DANGER_LEVEL` aka Environment variable `DANGER_LEVEL`)

### Asset paths

At startup, `initPaths()` resolves `%USERPROFILE%` and builds:

- `%USERPROFILE%\\.customTrumpets\\overlay\\trumpet1.png`
- `%USERPROFILE%\\.customTrumpets\\overlay\\trumpet2.png`
- `%USERPROFILE%\\.customTrumpets\\overlay\\trumpet3.png`
- `%USERPROFILE%\\.customTrumpets\\audio\\trumpet1.wav`
- `%USERPROFILE%\\.customTrumpets\\audio\\trumpet2.wav`
- `%USERPROFILE%\\.customTrumpets\\audio\\trumpet3.wav`
  If any of the six required files are missing, startup fails immediately.

### Danger-level mapping

`trumpetFromDanger()` maps value to active tier:

- `0` -> `0` (off)
- `20-49` -> `1`
- `50-79` -> `2`
- `80-100` -> `3`
- values outside ranges → off
  `readDangerLevelUser()` and `writeDangerLevel()` clamp values to `0-100`.

## 4) State transitions and controls

### Global hotkeys

In `wWinMain`, Trumpet registers:

- `F6` -> trumpet 1
- `F7` -> trumpet 2
- `F8` -> trumpet 3
  When a hotkey is pressed, `SetTrumpet(t, true)` is used:
- pressing the same active hotkey toggles off
- writes a canonical danger value:
    - tier 1 -> `49`
    - tier 2 -> `79`
    - tier 3 -> `100`

This over-rides any existing `DANGER_LEVEL` value, 
but the main loop will still read and react to changes from other sources.

This also over-rides the "sticky mode" behavior described below, since it sets the trumpet level directly.

### Sticky mode

Main loop behavior (every ~200ms):

- Trumpet auto-upgrades if danger enters a higher band.
- Trumpet does **not** auto-downgrade between non-zero bands.
- It resets to off only when danger reaches `0`.

## 5) Rendering and audio internals

### Overlay rendering (`UpdateOverlay`)

- Uses a layered transparent window (`WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW`).
- Draws through GDI+ into a 32-bit DIB section.
- Scales image proportionally to fit screen and centers it.
- Applies alpha via `UpdateLayeredWindow`.

### Overlay click-through

`OverlayWndProc` returns `HTTRANSPARENT` on `WM_NCHITTEST`, so the window does not block mouse interaction.

### Blink effect

`BlinkLoop` updates alpha every ~16ms using a sine wave and calls `UpdateOverlay`.

### Audio loop (`AudioLoop`)

- Detects tier changes and purges old sound (`SND_PURGE`).
- Plays wav in loop via `PlaySoundW` with:
    - primary: `SND_ASYNC | SND_FILENAME | SND_LOOP | SND_SYSTEM`
    - fallback: `SND_ASYNC | SND_FILENAME | SND_LOOP`
- Logs failures to stderr and `OutputDebugStringW`.

### Passive decay (`DecayLoop`)

Every 1-5 seconds, with 50% chance, danger is decreased by 1 if above zero.

## 6) Build and compile

Build configuration lives in `CMakeLists.txt`.

### Current build facts

- Minimum CMake: `3.25`
- Language: `C++20`
- Target: `Trumpet`
- Sources:
    - `trumpet/main.cpp`
    - `trumpet/resources/resources.rc`
- Output executable path:
    - `build/Trumpet.exe` (Debug and Release)
      For GNU toolchains only (`CMAKE_CXX_COMPILER_ID STREQUAL "GNU"`):
- compile options: `-municode -mwindows`
- link options: `-municode -mwindows`
- explicit libs: `gdiplus winmm user32 gdi32 ole32`

### Build commands (PowerShell)

From repository root:

First time only (Init Release build):
```powershell
cmake -S . -B cmake-build-release -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
```

Compile command (after initial config):
```powershell
cmake --build cmake-build-release --target Trumpet -- -j 18
```

Then run (after putting `trumpet/.customTrumpets` in place `%USERPROFILE%`):

```powershell
.\\build\\Trumpet.exe
```

## 7) Operational checklist

Before running `Trumpet.exe`, confirm:

1. `.customTrumpets` exists in `%USERPROFILE%` with both `overlay` and `audio` subfolders.
2. All 3 PNG and 3 WAV files are present and readable.
3. `DANGER_LEVEL` under `HKCU:\Environment` is numeric (or unset).
4. If startup is desired, installer-created Run key is present.

## 8) Troubleshooting

- **Startup exits immediately**: one or more required media files are missing.
- **No sound**: verify WAV files and check if `PlaySoundW` errors are printed.
- **No overlay**: verify PNG files and that GDI+ loaded the selected image.
- **Hotkeys do nothing**: another app may already own `F6/F7/F8`.
- **Unexpected tier behavior**: sticky mode only auto-resets at danger `0`.

## 9) Uninstall

Preferred uninstall (installer-generated script):

```powershell
powershell -ExecutionPolicy Bypass -File "C:\Program Files\Trumpet\uninstall-trumpet.ps1"
```

This removes program files, `%USERPROFILE%\\.customTrumpets`, and startup registration.
