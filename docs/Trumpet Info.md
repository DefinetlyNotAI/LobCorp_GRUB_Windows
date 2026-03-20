# Trumpet Info

`Trumpet` is a Windows app that shows a visual warning overlay and plays warning audio based on your danger level.

> Trumpet is 1.23 MB in size + 101 MB in media size
> 
> The standalone installer is 98.9MB in size, which includes all required files.

## What Trumpet does

- Watches your user `DANGER_LEVEL` value.
- Shows a full-screen overlay image when danger is active.
- Plays looping trumpet audio for the active danger tier.
- Lets you quickly switch tiers with hotkeys.

### Danger tiers

- `0`: off
- `20-49`: tier 1
- `50-79`: tier 2
- `80-100`: tier 3

## Quick install (normal users)

1. Run `Installer.exe` **as Administrator**.
2. Wait for the `Installation complete` message.
3. Start `Trumpet.exe` from `C:\Program Files\Trumpet`.

The installer is self-contained and carries everything it needs internally.

## First run and media files

Trumpet uses files under:

`%USERPROFILE%\.customTrumpets`

Expected structure:

- `overlay\trumpet1.png`
- `overlay\trumpet2.png`
- `overlay\trumpet3.png`
- `audio\trumpet1.wav`
- `audio\trumpet2.wav`
- `audio\trumpet3.wav`

If these files are missing/corrupt, Trumpet may fail to run correctly.

## How to use Trumpet

- `F6`: set tier 1 (`DANGER_LEVEL=49`)
- `F7`: set tier 2 (`DANGER_LEVEL=79`)
- `F8`: set tier 3 (`DANGER_LEVEL=100`)
- Pressing the same hotkey again toggles off.

Optional manual control:

```powershell
[Environment]::SetEnvironmentVariable("DANGER_LEVEL", "79", "User")
```

## Troubleshooting

- **Installer says extraction failed**: run `Installer.exe` as Administrator and try again.
- **No overlay or sound**: verify all files exist in `%USERPROFILE%\.customTrumpets` with exact names.
- **App closes immediately**: usually means required media files are missing or unreadable.

## Uninstall

- Run `UninstallTrumpet.exe` from `C:\Program Files\Trumpet`, or
- Run `TrumpetUninstaller.exe` from your build output if you are testing builds.

For development/build internals, see `docs/Trumpet - In detail.md`.

