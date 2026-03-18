# Boot Process Documentation - In detail

**Configuration:** Secure Boot Disabled
**Boot Chain:** UEFI Firmware → GRUB → SHIM (surface-level) → HackBGRT Loader → Windows Boot Manager

---

## Index
Boot Process Documentation:
- [Overview](#overview)
- [Stage 1. UEFI Firmware](#stage-1-uefi-firmware)
- [Stage 2. GRUB Initialization](#stage-2-grub-initialization)
- [Stage 3. HackBGRT Loader](#stage-3-hackbgrt-loader)
- [Stage 4. Boot Graphics Resource Table Modification](#stage-4-boot-graphics-resource-table-modification)
- [Stage 5. Windows Boot Manager](#stage-5-windows-boot-manager)
- [Stage 6. Windows OS Loader](#stage-6-windows-os-loader)
- [Alternative GRUB Entry](#alternative-grub-entry)
- [Complete Boot Sequence](#complete-boot-sequence)
- [Notes on Shim](#notes-on-shim)
- [Practical Behavior of This Configuration](#practical-behavior-of-this-configuration)

---

## Overview

This document describes the boot sequence implemented using **GRUB2Win** and **HackBGRT** on a UEFI-based Windows system.

The system boots using GRUB as the primary boot manager. GRUB then chainloads HackBGRT, 
which modifies the boot logo displayed during Windows startup by patching the **Boot Graphics Resource Table (BGRT)** in memory.
After applying the modification, HackBGRT launches the Windows Boot Manager.

Although HackBGRT ships with **shim** for Secure Boot compatibility, 
shim is not used in this configuration because Secure Boot is disabled and GRUB directly loads HackBGRT.

Relevant EFI executables:

```
/EFI/HackBGRT/loader.efi
/EFI/Microsoft/Boot/bootmgfw.efi
```

---

## Stage 1. UEFI Firmware

When the system powers on, UEFI firmware initializes hardware and performs POST checks.

After initialization:

1. The firmware reads the **BootOrder** variable stored in NVRAM.
2. The first valid EFI boot entry is selected.
3. The firmware loads the EFI executable associated with that entry.

In this configuration, the firmware loads the GRUB2Win EFI loader from the EFI System Partition.

Control then transfers from firmware to the GRUB bootloader.

---

## Stage 2. GRUB Initialization

GRUB begins execution and initializes its environment.

During this stage GRUB performs the following operations:

- Loads internal modules
- Reads the `grub.cfg` configuration file
- Initializes the graphics mode
- Builds the boot menu entries

The Windows entry used to start HackBGRT is defined in the configuration:

```bash
menuentry 'Windows (HackBGRT)' --hotkey=h --class windows --class icon-windows {
    set reviewpause=5
    set pager=0
    set efibootmgr=/EFI/HackBGRT/loader.efi
    getpartition file $efibootmgr root
    if [ $? = 0 ] ; then
        echo Grub is now loading HackBGRT
        echo Boot disk address is $root
        echo The boot mode is Windows EFI (HackBGRT)
        chainloader $efibootmgr
    fi
    g2wsleep
    savelast 0 'Windows (HackBGRT)'
}
```

The configuration performs the following actions:

1. Defines the EFI executable path `/EFI/HackBGRT/loader.efi`

2. Locates the partition containing the file using:
    ```bash
    getpartition file $efibootmgr root
    ```

3. Displays diagnostic information.

4. Transfers execution using:
    ```bash
    chainloader $efibootmgr
    ```

The **chainloader** command loads the EFI executable into memory and transfers control to it.

---

## Stage 3. HackBGRT Loader

The program started by GRUB is `/EFI/HackBGRT/loader.efi`.

This executable launches the HackBGRT boot graphics modification program.

HackBGRT is designed to change the image displayed during Windows boot by modifying the Boot Graphics Resource Table.

---

## Stage 4. Boot Graphics Resource Table Modification

HackBGRT loads its configuration from `/EFI/HackBGRT/config.txt`

It then loads the configured image file, usually `/EFI/HackBGRT/splash.bmp` which is located in `C:\HackBGRT-2.5.2\splash.bmp` in the Windows file system.

The image must be:

- 24-bit BMP format
- 54 byte header

HackBGRT performs several operations:

1. Detects the current framebuffer and screen resolution.
2. Loads the configured image.
3. Locates the **Boot Graphics Resource Table (BGRT)** in system memory.
4. Replaces the firmware logo pointer with the custom image.
5. Adjusts logo positioning according to configuration values.

The modification occurs entirely in memory. Firmware storage and Windows system files remain unchanged.

---

## Stage 5. Windows Boot Manager

After patching BGRT, HackBGRT launches the Windows Boot Manager.

Executable launched `/EFI/Microsoft/Boot/bootmgfw.efi`

Windows Boot Manager performs the following operations:

- Reads the **Boot Configuration Data (BCD)** store
- Determines the selected Windows installation
- Launches the Windows OS loader

---

## Stage 6. Windows OS Loader

The Windows OS loader initializes the Windows operating system.

During this stage the following components are loaded:

- Windows kernel
- Boot critical drivers
- Hardware abstraction layer
- System memory structures

Once initialization completes, the Windows kernel begins execution and normal operating system startup continues.

---

## Alternative GRUB Entry

The configuration also includes a direct Windows boot option that bypasses HackBGRT:

```bash
menuentry 'Windows EFI Boot Manager' --hotkey=w --class windows --class icon-windows {
    set reviewpause=5
    set pager=0
    set efibootmgr=/efi/Microsoft/Boot/bootmgfw.efi
    getpartition file $efibootmgr root
    if [ $? = 0 ] ; then
        echo Grub is now loading the Windows EFI Boot Manager
        echo Boot disk address is $root
        echo The boot mode is Windows EFI
        chainloader $efibootmgr
    fi
    g2wsleep
    savelast 0 'Windows EFI Boot Manager'
}
```

This entry launches Windows directly without modifying the boot logo.

This is a useful fallback option if there are issues with HackBGRT or if the user prefers the default boot experience.

---

## Complete Boot Sequence

The full execution path when selecting **Windows (HackBGRT)** is:

1. UEFI firmware initializes hardware
2. Firmware loads the GRUB EFI bootloader
3. GRUB reads `grub.cfg` and displays the boot menu
4. GRUB chainloads `/EFI/HackBGRT/loader.efi`
5. HackBGRT patches the Boot Graphics Resource Table
6. HackBGRT launches `/EFI/Microsoft/Boot/bootmgfw.efi`
7. Windows Boot Manager loads the Windows OS loader
8. Windows kernel initialization begins

---

## Notes on Shim

HackBGRT includes the **shim** bootloader to support Secure Boot environments.

Shim normally performs cryptographic verification of bootloaders before allowing them to execute.

Because Secure Boot is disabled in this system:

- shim is not required
- GRUB directly loads HackBGRT
- signature verification is not performed

If Secure Boot were enabled, shim would normally load HackBGRT after verifying their signatures.

Usually HackBGRT supports SHIM as there is proper documentation [here](https://github.com/Metabolix/HackBGRT/blob/main/shim.md)
on the first bootup "Verification failed" error and how to set up custom signed HackBGRT bootloader

But GRUB doesn't support the HackBGRT supplied SHIM, so Secure Boot is disabled, 
so we skip the SHIM bootloader and directly load HackBGRT without any issues.

---

## Practical Behavior of This Configuration

- GRUB acts as the primary boot manager with custom `LoboCorp` Theme.
- HackBGRT modifies the Windows boot logo by patching the BGRT structure in memory.
- Windows files remain unchanged.
- Windows Boot Manager begins the actual operating system boot process.

All stages before Windows Boot Manager run entirely inside the UEFI pre-boot environment.
