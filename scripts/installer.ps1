# Trumpet CLI Installer - Enterprise Grade
# Requires Admin

$currentUser = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $currentUser.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host "This installer must be run as administrator."
    exit 1
}

# Force TLS1.2
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

# Paths
$ProgramDir = "C:\Program Files\Trumpet"
$UserDir    = "$env:USERPROFILE"
$ZipFile    = Join-Path $env:TEMP "Trumpets.media.zip"

# URLs
$Urls = @{
    'Trumpet.exe'     = 'https://github.com/DefinetlyNotAI/LobCorp_GRUB_Windows/releases/download/1.0.0-trumpet/Trumpet.exe'
    'LICENSE'         = 'https://raw.githubusercontent.com/DefinetlyNotAI/LobCorp_GRUB_Windows/main/LICENSE'
    'Trumpet Info.md' = 'https://raw.githubusercontent.com/DefinetlyNotAI/LobCorp_GRUB_Windows/main/docs/Trumpet%20Info.md'
    'Zip'             = 'https://github.com/DefinetlyNotAI/LobCorp_GRUB_Windows/releases/download/1.0.0-trumpet/Trumpets.media.zip'
}

# Logging
function Log($msg) {
    $time = Get-Date -Format 'HH:mm:ss'
    Write-Host "[$time] $msg"
}

# Console progress bar
function ProgressBar($percent) {
    $width = 50
    $filled = [int]($width * ($percent/100))
    $empty = $width - $filled
    $bar = ('#'* $filled) + ('-'* $empty)
    Write-Host -NoNewline "`r[$bar] $([math]::Round($percent,1))%"
}

# Close running Trumpet
function Close-TrumpetIfRunning {
    $proc = Get-Process -Name Trumpet -ErrorAction SilentlyContinue
    if ($proc) {
        Log "Trumpet is running, closing..."
        $proc | Stop-Process -Force
    }
}

# Synchronous download with manual progress
function DownloadFile($url, $dest) {
    try {
        $req = [System.Net.WebRequest]::Create($url)
        $req.Method = "HEAD"
        $resp = $req.GetResponse()
        $totalBytes = [int64]$resp.Headers["Content-Length"]
        $resp.Close()
    } catch {
        $totalBytes = 0
    }

    if ($totalBytes -gt 0) {
        $sizeMB = [math]::Round($totalBytes / 1MB, 2)
        Log "Downloading $url ($sizeMB MB) -> $dest"
    } else {
        Log "Downloading $url -> $dest"
    }

    $req = [System.Net.WebRequest]::Create($url)
    $resp = $req.GetResponse()
    $stream = $resp.GetResponseStream()
    $fs = [System.IO.File]::Open($dest, [System.IO.FileMode]::Create)

    $buffer = New-Object byte[] 8192
    $read = 0
    $bytesRead = 0
    while (($read = $stream.Read($buffer,0,$buffer.Length)) -gt 0) {
        $fs.Write($buffer,0,$read)
        $bytesRead += $read
        if ($totalBytes -gt 0) {
            $percent = ($bytesRead / $totalBytes) * 100
            ProgressBar $percent
        }
    }

    $fs.Close()
    $stream.Close()
    $resp.Close()

    if ($totalBytes -gt 0) { ProgressBar 100; Write-Host "" }
    Log "Downloaded $dest"
}

# Install Trumpet
function Install-Trumpet {
    Close-TrumpetIfRunning
    $created = @()

    try {
        if (!(Test-Path $ProgramDir)) {
            New-Item -ItemType Directory -Path $ProgramDir -Force | Out-Null
            Log "Created directory $ProgramDir"
        }

        # Download main files (Trumpet.exe, LICENSE, Info.md)
        $files = @('Trumpet.exe','LICENSE','Trumpet Info.md')
        foreach ($f in $files) {
            $dest = Join-Path $ProgramDir $f
            if (Test-Path $dest) { Log "$f exists, overwriting..." }
            DownloadFile $Urls[$f] $dest
            $created += $dest
        }

        # Download and extract ZIP
        DownloadFile $Urls['Zip'] $ZipFile
        Log "Extracting ZIP..."
        Expand-Archive -Path $ZipFile -DestinationPath $UserDir -Force
        Remove-Item $ZipFile -Force -ErrorAction SilentlyContinue
        Log "Extraction complete"

        # Setup startup
        $startupChoice = Read-Host "Do you want Trumpet to run on Windows startup? (Y/N)"
        if ($startupChoice -match '^[Yy]$') {
            if (!(Get-ItemProperty -Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run' -Name 'Trumpet' -ErrorAction SilentlyContinue)) {
                New-ItemProperty -Path "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run" -Name "Trumpet" -Value "`"$ProgramDir\Trumpet.exe`"" -PropertyType String -Force | Out-Null
                Log "Added Trumpet to startup"
            }
        }

        # Create uninstaller script
        $UninstallFile = Join-Path $ProgramDir "uninstall-trumpet.ps1"
        $uninstallScript = @"
Add-Type -AssemblyName System.Windows.Forms
function Close-TrumpetIfRunning {
    \$proc = Get-Process -Name Trumpet -ErrorAction SilentlyContinue
    if (\$proc) { \$proc | Stop-Process -Force }
}
\$ProgramDir = '$ProgramDir'
\$UserDir = '$UserDir'
Close-TrumpetIfRunning
if (Test-Path \$ProgramDir) { Remove-Item -Path \$ProgramDir -Recurse -Force -ErrorAction SilentlyContinue }
if (Test-Path "`$UserDir\.customTrumpets") { Remove-Item -Path "`$UserDir\.customTrumpets" -Recurse -Force -ErrorAction SilentlyContinue }
Remove-ItemProperty -Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run' -Name 'Trumpet' -ErrorAction SilentlyContinue
[System.Windows.Forms.MessageBox]::Show('Trumpet uninstalled successfully.','Uninstall Complete')
"@
        $uninstallScript | Out-File -FilePath $UninstallFile -Encoding UTF8 -Force
        Log "Uninstaller created at $UninstallFile"

        # Register uninstall in App Registry
        $uninstallReg = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\Trumpet"
        New-Item -Path $uninstallReg -Force | Out-Null
        Set-ItemProperty -Path $uninstallReg -Name "DisplayName" -Value "Trumpet"
        Set-ItemProperty -Path $uninstallReg -Name "UninstallString" -Value "`"$UninstallFile`""
        Set-ItemProperty -Path $uninstallReg -Name "Publisher" -Value "LobCorp"
        Set-ItemProperty -Path $uninstallReg -Name "DisplayVersion" -Value "1.0.0"
        Set-ItemProperty -Path $uninstallReg -Name "EstimatedSize" -Value 5KB
        Set-ItemProperty -Path $uninstallReg -Name "NoModify" -Value 1
        Set-ItemProperty -Path $uninstallReg -Name "NoRepair" -Value 1
        Log "Registered uninstall in App registry"

        Log "Installation completed successfully!"
    }
    catch {
        Log "Installation failed: $_"
        foreach ($f in $created) { if (Test-Path $f) { Remove-Item $f -Force -ErrorAction SilentlyContinue } }
        if (Test-Path $ZipFile) { Remove-Item $ZipFile -Force -ErrorAction SilentlyContinue }
        Log "Rollback complete."
    }
}

# Run installer
Install-Trumpet