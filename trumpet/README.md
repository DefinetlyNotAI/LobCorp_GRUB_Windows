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

- `build/generated`

## Preset-based build (recommended)

Use the repository presets in `CMakePresets.json`:

```powershell
cmake --preset msys2-release
cmake --build --preset build-all-release
```

This gives you the ordered build pipeline:

`Trumpet -> TrumpetUninstaller -> Installer`

## Custom media embedding mode

`TRUMPET_EMBED_CUSTOM_TRUMPETS` controls how `.customTrumpets` is handled:

- `OFF` (default): build packages `customTrumpets.zip` and copies it next to `Installer.exe`.
- `ON`: build converts zip bytes into `CustomTrumpetsZip.h` and embeds it in `Installer.exe`.

For large media packs, keep this `OFF` to avoid very slow header generation.

## Additional docs

- Detailed internals: `docs/Trumpet - In detail.md`
- High-level overview: `docs/Trumpet Info.md`
