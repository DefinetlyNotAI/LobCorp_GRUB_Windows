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
cmake --build --preset trumpet-only
cmake --build --preset full-build
cmake --build --preset full-build-fresh
cmake --build --preset basic-test
```

Equivalent explicit build-dir commands:

```powershell
cmake --preset release
cmake --build cmake/build --target full-build
```

This gives you the ordered build pipeline:

`Trumpet -> TrumpetUninstaller -> Installer`

## Custom media embedding

`.customTrumpets` is always packaged and embedded into the installer binary as a resource.

- `trumpet/generated` stays header-only (`*.h`).
- `customTrumpets.zip` is staged at `cmake/zip/customTrumpets.zip` and then embedded.
- `Installer.exe` extracts from its own embedded ZIP resource at runtime.

## Additional docs

- Detailed internals: `docs/Trumpet - In detail.md`
- High-level overview: `docs/Trumpet Info.md`
