# `trumpet/` module

This folder contains the Windows C++ sources for:

- `Trumpet.exe` (`main.cpp`)
- `TrumpetUninstaller.exe` (`uninstaller.cpp`)
- `Installer.exe` (`installer.cpp`)

## File overview

- `main.cpp`: overlay window, audio loop, hotkeys, danger-level logic.
- `uninstaller.cpp`: process stop + file/registry cleanup.
- `installer.cpp`: writes embedded binaries/docs and extracts custom media.
- `resources/resources.rc`: executable resources.
- `.customTrumpets/`: source media pack used for installer packaging.

## Build notes

Top-level `CMakeLists.txt` defines build targets and generated installer assets.

Outputs go to top-level `build/`:

- `build/Trumpet.exe`
- `build/TrumpetUninstaller.exe`
- `build/Installer.exe`

Generated installer assets go to:

- `trumpet/generated`

## Preset-based build (recommended)

Use the repository presets in `CMakePresets.json`:

```powershell
cmake --preset msys2-release
cmake --build --preset build-all-release
```

Equivalent explicit build-dir commands:

```powershell
cmake -S . -B cmake/build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER="C:/msys64/mingw64/bin/g++.exe"
cmake --build cmake/build --target build_all
```

Debug build-dir command:

```powershell
cmake -S . -B cmake/debug -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER="C:/msys64/mingw64/bin/g++.exe"
cmake --build cmake/debug
```

This gives you the ordered build pipeline:

`Trumpet -> TrumpetUninstaller -> Installer`

## Custom media embedding

`.customTrumpets` is always packaged and embedded into `CustomTrumpetsZip.h`.

- `trumpet/generated` stays header-only (`*.h`).
- `customTrumpets.zip` is staged in the active build directory (`cmake/build` or `cmake/debug`) and then embedded.
- `Installer.exe` always extracts from embedded bytes in `CustomTrumpetsZip.h`.

## Additional docs

- Detailed internals: `docs/Trumpet - In detail.md`
- High-level overview: `docs/Trumpet Info.md`
