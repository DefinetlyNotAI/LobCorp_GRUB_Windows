# Lobotomy Corp Themed Windows Bootloader

## Index
Setup Instructions:
- [Preset-up Requirements](#preset-up-requirements)
    - [Required UEMI BIOS Settings](#required-uemi-bios-settings)
    - [Required Files/Software](#required-filessoftware)
    - [Important Note](#important-note)
- [Folder setup](#folder-setup)
- [GRUB Setup](#grub-setup)
- [HackBGRT Setup](#hackbgrt-setup)
- [Re-initialize Bootloader order](#re-initialize-bootloader-order)
- [Credits](#credit)
    - [Extra](#extra)

## Preset-up Requirements

### Required UEMI BIOS Settings:

- You must be running on a UEMI Based system (not Legacy).
- 1920x1080 Aspect Ratio support from your Windows device.
- Disable [Secure Boot](https://learn.microsoft.com/en-us/windows-hardware/manufacture/desktop/disabling-secure-boot?view=windows-11) (To allow non-Microsoft software to run):
- Disable BitLocker (To be able to install Grub on the disk and allow it to be readable correctly)
    - To disable BitLocker run the following PowerShell command and wait until it completes \[must be in admin]:
  ```bash
  # Replace C: with your windows drive
  manage-bde -off C:
  
  # To track the progress dynamically (Ctrl+C once it says 0%)
  while ($true) { Clear-Host; manage-bde -status C:; Start-Sleep 1 }
  ```

If you want to know if your system can support this setup, run the following command in PowerShell (must be in admin):

```bash
powershell -ExecutionPolicy Bypass `
-Command "irm https://raw.githubusercontent.com/DefinetlyNotAI/LobCorp_GRUB_Windows/main/scripts/checker.ps1 | iex"

```

> The script is in the GitHub repo, and you can view its code directly [here](../scripts/checker.ps1).

### Required Files/Software:

- [HackBGRT](https://github.com/Metabolix/HackBGRT) for custom logo to replace windows boot logo
    - Download the latest release (I used 2.5.2) from [here](https://github.com/Metabolix/HackBGRT/releases)

- [Grub2Win](https://sourceforge.net/projects/grub2win/) to install the Grub bootloader and set it up (just install the exe)

- [LoboGrub Theme](https://github.com/rats-scamper/LoboGrubTheme) for the lob-corp theme
    - Download the repo itself - all you need is the `lobocorp` folder,
      for direct installation you can get the folder itself [here](https://download-directory.github.io/?url=https%3A%2F%2Fgithub.com%2Frats-scamper%2FLoboGrubTheme%2Ftree%2Fmain%2Flobocorp)

- An image that is 254x127 (note the software will auto rescale if the size is diff, but it is recommended to try and maintain a similar size ratio)

- Custom [`grub.cfg`](../grub/grub.cfg) and [`grubenv`](../grub/grubenv) files to replace the default ones created by the installer.

### Important Note

Make a [recovery drive](https://support.microsoft.com/en-us/windows/recovery-drive-abb4691b-5324-6d4a-8766-73fab304c246) AND a [system restore point](https://support.microsoft.com/en-us/windows/system-restore-a5ae3ed9-07c4-fd56-45ee-096777ecd14e) in case anything goes wrong.

Remember you are modifying the bootloader of your system, so if you make a mistake,
you might end up in a situation where your system is unbootable, and you will need to use the recovery drive to fix it.

Let's not BRICK our devices, be careful and follow the instructions carefully,
and if you are not sure about something then stop and ask for help in the [community tab](https://github.com/DefinetlyNotAI/LobCorp_GRUB_Windows/discussions/categories/help).

## Folder setup

First extract the `grub2win.zip` which should include an EXE. Run the EXE and follow the instructions,
ensuring that the location saved is in `C:\` (or change the drive letter to whatever the windows installation is based in)
so that it becomes `C:\grub2`. When it finishes install and asks to open the file, refuse/don't open it yet.

Once completed put the `lobcorp` folder installed previously INSIDE `C:\grub2\themes` so that it becomes `C:\grub2\themes\lobocorp`

Now extract the downloaded `HackBGRT-2.5.2.zip` (of whatever version number) and put the folder inside your
`C:\` (or change the drive letter to whatever the windows installation is based in) so that it becomes `C:\HackBGRT-2.5.2`

## GRUB Setup

> We will set up GRUB first before HackBGRT. We will later need to change the boot order once we set up both.
> The only reason we do this "hassle" is so we can ensure nothing breaks in the middle as this is a safer approach.

We already executed the installer script, assuming we already did the setup and removed secure boot and BitLocker,
the installer should have not produced any errors/issues

Open the EXE which can be found in `C:\grub2\grub2win.exe`, then close it, this is to make sure all files are correctly setup,
however if you see a warning or error, ensure to follow the instructions of the error.

You should have already downloaded the [`grub.cfg`](../grub/grub.cfg) and [`grubenv`](../grub/grubenv) files.
Put both files in `C:\grub2\`, and once you get prompted to replace the files, click accept.

<details>
  <summary>Modification Details</summary>

> The modification will include the following:
>
> - `grubenv` will include the following extra/replacement settings to ensure proper access, and saving, as well as correct aspect ratio
    >
    >   ```env
>   grub2win_gfxmode=1920x1080,auto
>   GRUB_THEME="/boot/grub/themes/lobocorp/theme.txt"
>   grub_theme="/boot/grub/themes/lobocorp/theme.txt"
>   GRUB_DEFAULT=saved
>   GRUB_SAVEDEFAULT=true
>   grub_default=saved
>   grub_savedefault=true
>   ```
>
> - `grub.cfg` includes the following changes:
    >
    >   - Replaces icon dir path: `set icondir=$prefix/themes/lobocorp/icons ; $prefix/themes/icons is the default`
>
>   - Replaces theme config path: `set theme=$prefix/themes/lobocorp/theme.txt ; $prefix/themes/custom.config.txt is the default`
>
>   - Replaces all `class` and `class-icon` flags of all 5 menu items to the theme supported items
>
>   - Replaces the windows bootloader path with 2 windows bootloaders (1 with and 1 without HackBGRT):
      >
      >     ```cfg
>     menuentry   'Windows (HackBGRT)                            Hotkey=h               '   --hotkey=h    --class windows   --class icon-windows  {
>          set reviewpause=5
>          set pager=0
>          set efibootmgr=/EFI/HackBGRT/loader.efi
>          getpartition  file  $efibootmgr  root
>          if [ $? = 0 ] ; then
>              echo Grub is now loading HackBGRT
>              echo Boot disk address is    $root
>              echo The boot mode is        Windows EFI (HackBGRT)
>              chainloader $efibootmgr
>          fi
>          g2wsleep
>          savelast 0 'Windows (HackBGRT)'
>     }
>             
>     menuentry   'Windows EFI Boot Manager                            Hotkey=w               '   --hotkey=w    --class windows   --class icon-windows  {
>          set reviewpause=5
>          set pager=0
>          set efibootmgr=/efi/Microsoft/Boot/bootmgfw.efi
>          getpartition  file  $efibootmgr  root
>          if [ $? = 0 ] ; then
>              echo Grub is now loading the Windows EFI Boot Manager
>              echo Boot disk address is    $root
>              echo The boot mode is        Windows EFI
>              chainloader $efibootmgr
>          fi
>          g2wsleep
>          savelast 0 'Windows EFI Boot Manager'
>     }
>     ```

</details>

Finally, restart your computer. When it restarts it should automatically boot into the GRUB menu.

> If it doesn't, you may need to change the boot order in the BIOS settings to make sure `grub2win` is the first boot option,
> by clicking (F6) or (+) to move it to the top of the list, then save and exit.

Once you do that, you should be able to see the GRUB menu with the new lobcorp theme.

We will now proceed to set up HackBGRT, in the boot window select the SECOND menu option (Windows EFI Boot Manager) which is hotkey `w`.
DO NOT SELECT THE FIRST OPTION (Windows (HackBGRT)) as it will not work until we set up HackBGRT.

## HackBGRT Setup

Returning back to the Windows desktop, open the folder `C:\HackBGRT-2.5.2`, in there will be `setup.exe`, run it as administrator

It will ask you to select an option, type `I` to install.

> If `I` doesn't work, try typing `J` instead,
> note that I have not tested the alternative install method, so this is at your own risk.

Next it will open a `Paint` window with a default image, you can edit this image as you like,
but make sure to maintain the same aspect ratio (254x127) for best results.
You can use the already prepared image, copy it, and in the paint window paste it, then save and exit the paint window.

Once completed it should properly have installed HackBGRT and you should be able to see the new boot logo.

> Note that HackBGRT uses the SHIM bootloader, but the implementation skips the SHIM bootloader and directly boots the HackBGRT `loader.efi`.
> any issues that occur you may check the documentation of [HackBGRT Troubleshooting](https://github.com/Metabolix/HackBGRT#troubleshooting), but ignore SHIM based issues/solutions.

Restart your computer, and you should see the newly custom modified boot logo, but no GRUB menu.

> If you ever want to change the logo, run the `setup.exe` again but type `F` to install config files only,
> which will reprompt the paint window again, allowing you to change the logo without having to reinstall the whole bootloader.

## Re-initialize Bootloader order

Now to be able to see the GRUB menu with the new logo, you need to change the boot order again in the BIOS settings,

Go to the boot order options, you should see 3 options now in this order [Different names could be possible]:
- `HackBGRT` (SHIM Bootloader)
- `grub2win` (GRUB Bootloader)
- `Windows Boot Manager` (Default Windows Bootloader)

You will need to move `grub2win` to the top of the list, then save and exit.
More info can be found [here](https://support.lenovo.com/us/en/solutions/ht117661-how-to-change-the-boot-order-in-bios-for-windows-7-8-81-10)

Once completed, save and exit, and you should be met with the custom-themed GRUB menu.
Selecting the first option (Windows (HackBGRT)) will boot you into the windows bootloader with the custom logo.

> Selecting the second option (Windows EFI Boot Manager) will boot you into the default windows bootloader without the custom logo, allowing you to have both options available.

Congrats, you have successfully set up a custom Lobotomy Corp themed Windows bootloader using GRUB and HackBGRT!
You can now enjoy your new boot experience every time you start your computer.

## Credit

- [Metabolix](https://github.com/Metabolix) for creating [HackBGRT](https://github.com/Metabolix/HackBGRT)
  and providing detailed documentation on how to set up and troubleshoot the bootloader.
- [Drummer](https://sourceforge.net/u/drummerdp/profile/) for creating [Grub2Win](https://sourceforge.net/projects/grub2win/), you can support them [here on PayPal](https://www.paypal.com/donate/?cmd=_s-xclick&hosted_button_id=UE5UCE9FF5YNC&ssrt=1773597099831)
- [simon](https://github.com/rats-scamper) for creating the [LoboGrub Theme](https://github.com/rats-scamper/LoboGrubTheme), do give them a star!

###  Extra

If you need help or support go to the [community tab](https://github.com/DefinetlyNotAI/LobCorp_GRUB_Windows/discussions/categories/help)

---
