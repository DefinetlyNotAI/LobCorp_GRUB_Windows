# Trumpet

Trumpet is a lightweight Windows overlay + looping audio app driven by a user-level `DANGER_LEVEL` value.

It is designed to run in the background, react to danger changes, and display/play one of three "trumpet tiers".

> Main installer app: `scripts/installer.ps1`

## About

Trumpet has three core behaviors:

- **Visual overlay**: A transparent topmost window renders PNG overlays from `%USERPROFILE%\\.customTrumpets\\overlay`.
- **Looping audio**: Tier-specific WAV files are looped asynchronously from `%USERPROFILE%\\.customTrumpets\\audio`.
- **Danger integration**: `DANGER_LEVEL` (stored under `HKCU:\Environment`) controls which tier is active.

### Tier mapping

- `0` -> no trumpet
- `20-49` -> trumpet 1
- `50-79` -> trumpet 2
- `80-100` -> trumpet 3

## Install (recommended)

Use the attached installer.exe

## Runtime controls

- `F6`: set trumpet 1 (`DANGER_LEVEL=49`)
- `F7`: set trumpet 2 (`DANGER_LEVEL=79`)
- `F8`: set trumpet 3 (`DANGER_LEVEL=100`)

Pressing the same hotkey again toggles off (sets trumpet to `0`).

You can also set `DANGER_LEVEL` directly via PowerShell:

```powershell
[Environment]::SetEnvironmentVariable("DANGER_LEVEL", "INPUT VALUE HERE", "User")

```

This means you can later integrate Trumpet with other scripts or tools by modifying `DANGER_LEVEL` as needed.

## Notes

- Trumpet expects these files to exist at runtime:
  - `%USERPROFILE%\\.customTrumpets\\overlay\\trumpet1.png`
  - `%USERPROFILE%\\.customTrumpets\\overlay\\trumpet2.png`
  - `%USERPROFILE%\\.customTrumpets\\overlay\\trumpet3.png`
  - `%USERPROFILE%\\.customTrumpets\\audio\\trumpet1.wav`
  - `%USERPROFILE%\\.customTrumpets\\audio\\trumpet2.wav`
  - `%USERPROFILE%\\.customTrumpets\\audio\\trumpet3.wav`
- If any file is missing, the app exits early with an error.

## Uninstall

If installed using `scripts/installer.ps1`, run:

```powershell
powershell -ExecutionPolicy Bypass -File "C:\Program Files\Trumpet\uninstall-trumpet.ps1"
```

This removes installed files, `.customTrumpets`, and the startup entry.

For implementation and build internals, see `docs/Trumpet.md`.

