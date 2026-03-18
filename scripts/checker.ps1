# System Requirement Checker for Windows 11 Installation of GRUB and HackBGRT

# ---- Admin Check ----
$admin = ([Security.Principal.WindowsPrincipal] `
    [Security.Principal.WindowsIdentity]::GetCurrent()
).IsInRole([Security.Principal.WindowsBuiltinRole]::Administrator)

if (-not $admin) {
    Write-Host "[X] This script must be run as Administrator." -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "==== System Requirement Check ====" -ForegroundColor Cyan
Write-Host ""

function Pass($msg) {
    Write-Host "[✓] $msg" -ForegroundColor Green
}

function Fail($msg) {
    Write-Host "[X] $msg" -ForegroundColor Red
}

function SoftFail($msg) {
    Write-Host "[!] $msg" -ForegroundColor Yellow
}

# ---- UEFI Check ----
$firmware = (Get-ComputerInfo).BiosFirmwareType
if ($firmware -eq "UEFI") {
    Pass "UEFI Based System"
} else {
    Fail "System is not UEFI (Detected: $firmware)"
}

# ---- 1920x1080 Support Check ----
$found1080 = $false
$resolutions = (Get-CimInstance -Namespace root\wmi -ClassName WmiMonitorListedSupportedSourceModes).MonitorSourceModes

foreach ($mode in $resolutions) {
    if ($mode.HorizontalActivePixels -eq 1920 -and $mode.VerticalActivePixels -eq 1080) {
        $found1080 = $true
        break
    }
}

if ($found1080) {
    Pass "1920x1080 Display Mode Supported"
} else {
    Fail "1920x1080 Display Mode Not Detected - You can still try with the setup but may encounter issues"
}

# ---- Secure Boot Check ----
try {
    $secure = Confirm-SecureBootUEFI
    if ($secure) {
        SoftFail "Secure Boot Enabled [You can disable this]"
    } else {
        Pass "Secure Boot Disabled"
    }
} catch {
    Fail "Secure Boot Status Could Not Be Determined"
}

# ---- BitLocker Check ----
$bitlocker = (Get-BitLockerVolume -MountPoint "C:").ProtectionStatus

if ($bitlocker -eq "Off") {
    Pass "BitLocker Disabled"
} else {
    SoftFail "BitLocker Enabled [You can disable this]"
}

Write-Host ""
Write-Host "==== Check Complete ====" -ForegroundColor Cyan