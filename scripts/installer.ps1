# Trumpet CLI Installer - Enterprise Grade (FINAL)

# Require Admin
$currentUser = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $currentUser.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host "Run as Administrator." -ForegroundColor Red
    exit 1
}

[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

# Paths
$ProgramDir = "C:\Program Files\Trumpet"
$UserDir    = "$env:USERPROFILE"
$ZipFile    = Join-Path $env:TEMP "Trumpets.media.zip"
$UninstallEXE = Join-Path $ProgramDir "UninstallTrumpet.exe"

# URLs
$Urls = @{
    'Trumpet.exe'     = 'https://github.com/DefinetlyNotAI/LobCorp_GRUB_Windows/releases/download/1.0.0-trumpet/Trumpet.exe'
    'LICENSE'         = 'https://raw.githubusercontent.com/DefinetlyNotAI/LobCorp_GRUB_Windows/main/LICENSE'
    'Trumpet Info.md' = 'https://raw.githubusercontent.com/DefinetlyNotAI/LobCorp_GRUB_Windows/main/docs/Trumpet%20Info.md'
    'Zip'             = 'https://github.com/DefinetlyNotAI/LobCorp_GRUB_Windows/releases/download/1.0.0-trumpet/Trumpets.media.zip'
}

# Logging
function Log($msg) {
    Write-Host ( "[{0}] {1}" -f (Get-Date -Format 'HH:mm:ss'), $msg )
}

# Progress bar
function ProgressBar($percent) {
    $width = 40
    $filled = [int]($width * ($percent/100))
    $bar = ('#'*$filled) + ('-'*($width-$filled))
    Write-Host -NoNewline "`r[$bar] $([math]::Round($percent,1))%"
}

# Close running process
function Close-TrumpetIfRunning {
    $proc = Get-Process -Name Trumpet -ErrorAction SilentlyContinue
    if ($proc) {
        Log "Closing running Trumpet..."
        $proc | Stop-Process -Force
    }
}

# Download with real progress
function DownloadFile($url, $dest) {
    try {
        $head = [System.Net.WebRequest]::Create($url)
        $head.Method = "HEAD"
        $resp = $head.GetResponse()
        $total = [int64]$resp.Headers["Content-Length"]
        $resp.Close()
    } catch { $total = 0 }

    if ($total -gt 0) {
        Log "Downloading $url ($([math]::Round($total/1MB,2)) MB)"
    } else {
        Log "Downloading $url"
    }

    $req = [System.Net.WebRequest]::Create($url)
    $resp = $req.GetResponse()
    $stream = $resp.GetResponseStream()
    $fs = [System.IO.File]::Open($dest,[System.IO.FileMode]::Create)

    $buffer = New-Object byte[] 8192
    $read = 0
    $done = 0

    while (($read = $stream.Read($buffer,0,$buffer.Length)) -gt 0) {
        $fs.Write($buffer,0,$read)
        $done += $read
        if ($total -gt 0) {
            ProgressBar (($done / $total) * 100)
        }
    }

    $fs.Close()
    $stream.Close()
    $resp.Close()

    if ($total -gt 0) { ProgressBar 100; Write-Host "" }
    Log "Saved -> $dest"
}

# Create native EXE uninstaller
function Create-Uninstaller {
    $code = @"
using System;
using System.Diagnostics;
using System.IO;
using Microsoft.Win32;
using System.Windows.Forms;

class Uninstall {
    [STAThread]
    static void Main() {
        try {
            foreach (var p in Process.GetProcessesByName("Trumpet")) { p.Kill(); }

            string prog = @"$ProgramDir";
            if (Directory.Exists(prog)) Directory.Delete(prog, true);

            string media = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), ".customTrumpets");
            if (Directory.Exists(media)) Directory.Delete(media, true);

            var run = Registry.CurrentUser.OpenSubKey(@"Software\Microsoft\Windows\CurrentVersion\Run", true);
            if (run != null) run.DeleteValue("Trumpet", false);

            var uninst = Registry.CurrentUser.OpenSubKey(@"Software\Microsoft\Windows\CurrentVersion\Uninstall", true);
            if (uninst != null) uninst.DeleteSubKeyTree("Trumpet", false);

            MessageBox.Show("Trumpet removed.", "Done");
        } catch (Exception ex) {
            MessageBox.Show(ex.Message, "Error");
        }
    }
}
"@

    Add-Type -TypeDefinition $code -Language CSharp -OutputAssembly $UninstallEXE -OutputType WindowsApplication
    Log "Uninstaller EXE created"
}

# Install
function Install-Trumpet {
    Close-TrumpetIfRunning

    if (!(Test-Path $ProgramDir)) {
        New-Item -ItemType Directory -Path $ProgramDir | Out-Null
        Log "Created $ProgramDir"
    }

    foreach ($f in @('Trumpet.exe','LICENSE','Trumpet Info.md')) {
        $dest = Join-Path $ProgramDir $f
        if (Test-Path $dest) { Log "$f exists, overwriting..." }
        DownloadFile $Urls[$f] $dest
    }

    DownloadFile $Urls['Zip'] $ZipFile
    Log "Extracting..."
    Expand-Archive $ZipFile -DestinationPath $UserDir -Force
    Remove-Item $ZipFile -Force
    Log "Media ready"

    # Startup prompt
    $ans = Read-Host "Run on startup? (Y/N)"
    if ($ans -match '^[Yy]') {
        New-ItemProperty -Path "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run" `
            -Name "Trumpet" `
            -Value "`"$ProgramDir\Trumpet.exe`"" `
            -Force | Out-Null
        Log "Startup enabled"
    }

    # Uninstaller EXE
    Create-Uninstaller

    # Registry uninstall entry
    $key = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\Trumpet"
    New-Item $key -Force | Out-Null
    Set-ItemProperty $key DisplayName "Trumpet"
    Set-ItemProperty $key UninstallString "`"$UninstallEXE`""
    Set-ItemProperty $key Publisher "LobCorp"
    Set-ItemProperty $key DisplayVersion "1.0.0"
    Log "Registered in Apps & Features"

    Log "Launching Trumpet..."
    Start-Process (Join-Path $ProgramDir "Trumpet.exe")

    Log "Done."
}

Install-Trumpet