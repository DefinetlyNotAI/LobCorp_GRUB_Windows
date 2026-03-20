# Trumpet - In Detail

This document is the developer guide for build flow, architecture, generated assets, and troubleshooting.

## 1. Project layout

- `trumpet/main.cpp`: runtime app (overlay, audio, hotkeys, danger logic).
- `trumpet/installer.cpp`: self-contained installer that writes binaries/docs and extracts media.
- `trumpet/uninstaller.cpp`: removal/cleanup app.
- `trumpet/resources/resources.rc`: shared icon resource used by all EXEs.
- `trumpet/.customTrumpets`: source media packaged for installer embedding.
- `trumpet/generated`: generated headers used by installer build.
- `cmake/zip/customTrumpets.zip`: persistent media archive used for installer resource embedding.

## 2. Runtime model (`Trumpet.exe`)

### Danger source

`DANGER_LEVEL` is read from `HKCU\Environment` and clamped to `0..100`.

### Tier mapping

- `0`: off
- `20-49`: tier 1
- `50-79`: tier 2
- `80-100`: tier 3

### Assets expected at runtime

Resolved from `%USERPROFILE%\.customTrumpets`:

- `overlay\trumpet1.png`, `overlay\trumpet2.png`, `overlay\trumpet3.png`
- `audio\trumpet1.wav`, `audio\trumpet2.wav`, `audio\trumpet3.wav`

### Hotkeys

- `F6/F7/F8` set canonical values `49/79/100`.
- Pressing an already active hotkey toggles to off.

## 3. Build outputs and target graph

Primary artifacts in `build/`:

- `Trumpet.exe`
- `TrumpetUninstaller.exe`
- `Installer.exe`

Ordered dependency chain:

`Trumpet -> TrumpetUninstaller -> GenerateInstallerAssets -> Installer`

User-facing custom targets in `CMakeLists.txt`:

- `trumpet-only`
- `full-build`
- `basic-test`

## 4. Presets (clean set)

`CMakePresets.json` now exposes four build presets:

- `trumpet-only`
- `full-build`
- `full-build-fresh`
- `basic-test`

Internal configure presets:

- `release`
- `release-fresh` (forces full regeneration of generated headers/assets)

## 5. Build commands

From repository root:

```powershell
cmake --build --preset trumpet-only
cmake --build --preset full-build
cmake --build --preset full-build-fresh
cmake --build --preset basic-test
```

If needed, configure first explicitly:

```powershell
cmake --preset release
```

## 6. Installer asset generation

Generated headers in `trumpet/generated`:

- `TrumpetExe.h`
- `UninstallerExe.h`
- `LicenseTxt.h`
- `ReadmeMd.h`

Media packaging:

- `.customTrumpets` is zipped to `cmake/zip/customTrumpets.zip`.
- ZIP is embedded in `Installer.exe` as `RCDATA` resource.
- Installer loads ZIP from its own resources at runtime and extracts to `%USERPROFILE%\.customTrumpets`.

This keeps `Installer.exe` standalone.

## 7. Resource/icon model

- Shared IDs: `trumpet/resources/resource_ids.h`
- Shared icon rc: `trumpet/resources/resources.rc`
- All EXEs compile the same icon (`peBox.ico`).

## 8. Troubleshooting

- **Extraction fail in installer**: verify installer is run elevated and PowerShell is available (`powershell.exe` or `pwsh.exe`).
- **Missing runtime media**: confirm files under `%USERPROFILE%\.customTrumpets` and exact names.
- **Unexpected stale generated files**: run `cmake --build --preset full-build-fresh`.
- **ZIP path checks**: inspect `cmake/zip/customTrumpets.zip` and list entries with `cmake -E tar tf`.

## 9. Validation checklist

```powershell
cmake --build --preset full-build
cmake --build --preset basic-test
cmake -E tar tf "cmake/zip/customTrumpets.zip"
```

