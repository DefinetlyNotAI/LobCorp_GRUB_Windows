# Trumpet CLI/TUI Installer - Enterprise Grade

# Require Admin
$currentUser = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $currentUser.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host "ERROR: This installer must be run as Administrator." -ForegroundColor Red
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

# Logging and TUI helper
function Log($msg) {
    $time = Get-Date -Format 'HH:mm:ss'
    Write-Host "[$time] $msg"
}

function ProgressBar($percent) {
    $width = 50
    $filled = [math]::Floor($percent/100 * $width)
    $empty  = $width - $filled
    $bar = ('#'* $filled) + ('-'* $empty)
    Write-Host ("[{0}] {1,3}%" -f $bar, [int]$percent) -NoNewline
    Write-Host "`r"
}

# Close running Trumpet
function Close-TrumpetIfRunning {
    $proc = Get-Process -Name Trumpet -ErrorAction SilentlyContinue
    if ($proc) {
        Log "Trumpet is running, closing..."
        $proc | Stop-Process -Force
    }
}

# Download file with terminal progress
function DownloadFile($url, $dest, [int]$weight=0) {
    try {
        $req = [System.Net.WebRequest]::Create($url)
        $req.Method = "HEAD"
        $resp = $req.GetResponse()
        $totalBytes = [int64]$resp.Headers["Content-Length"]
        $resp.Close()
    } catch { $totalBytes = 0 }

    if ($totalBytes -gt 0) {
        $sizeMB = [math]::Round($totalBytes/1MB,2)
        Log "Downloading $url ($sizeMB MB) -> $dest"
    } else { Log "Downloading $url -> $dest" }

    $client = New-Object System.Net.WebClient

    if ($totalBytes -gt 0) {
        $client.DownloadProgressChanged += {
            $p = $_.ProgressPercentage * $weight / 100
            ProgressBar $p
        }
    }

    $client.DownloadFile($url, $dest)
    ProgressBar 100
    Write-Host ""
    Log "Downloaded $dest"
}

# Install function
function Install-Trumpet {
    Close-TrumpetIfRunning
    $created = @()

    try {
        # Create program dir
        if (!(Test-Path $ProgramDir)) {
            New-Item -ItemType Directory -Path $ProgramDir -Force | Out-Null
            Log "Created directory $ProgramDir"
        }

        # Download main files
        $files = @('Trumpet.exe','LICENSE','Trumpet Info.md')
        $fileWeight = 50 / $files.Count
        foreach ($f in $files) {
            $dest = Join-Path $ProgramDir $f
            if (Test-Path $dest) { Log "$f exists, overwriting..." }
            DownloadFile $Urls[$f] $dest $fileWeight
            $created += $dest
        }

        # Download and extract ZIP
        DownloadFile $Urls['Zip'] $ZipFile 30
        Log "Extracting ZIP..."
        Expand-Archive -Path $ZipFile -DestinationPath $UserDir -Force
        Remove-Item $ZipFile -Force -ErrorAction SilentlyContinue
        ProgressBar 90
        Write-Host ""
        Log "Extraction complete"

        # Setup startup
        $startup = Read-Host "Run Trumpet on Windows startup? (Y/N)"
        if ($startup -match '^[Yy]') {
            if (!(Get-ItemProperty -Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run' -Name 'Trumpet' -ErrorAction SilentlyContinue)) {
                New-ItemProperty -Path "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run" -Name "Trumpet" -Value "`"$ProgramDir\Trumpet.exe`"" -PropertyType String -Force | Out-Null
                Log "Added Trumpet to startup"
            }
        }

        # Create uninstaller
        $UninstallFile = Join-Path $ProgramDir "uninstall-trumpet.ps1"
        $uninstallScript = @"
\$ProgramDir = '$ProgramDir'
\$UserDir = '$UserDir'
if (Get-Process -Name Trumpet -ErrorAction SilentlyContinue) { Stop-Process -Name Trumpet -Force }
if (Test-Path \$ProgramDir) { Remove-Item -Path \$ProgramDir -Recurse -Force -ErrorAction SilentlyContinue }
if (Test-Path "`$UserDir\.customTrumpets") { Remove-Item -Path "`$UserDir\.customTrumpets" -Recurse -Force -ErrorAction SilentlyContinue }
Remove-ItemProperty -Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run' -Name 'Trumpet' -ErrorAction SilentlyContinue
Write-Host 'Trumpet uninstalled successfully.'
"@
        $uninstallScript | Out-File -FilePath $UninstallFile -Encoding UTF8 -Force
        Log "Uninstaller created at $UninstallFile"

        # Register uninstall in registry
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

        ProgressBar 100
        Write-Host ""
        Log "Installation completed successfully."

    } catch {
        Log "Installation failed: $_"
        # rollback
        foreach ($f in $created) { if (Test-Path $f) { Remove-Item $f -Force -ErrorAction SilentlyContinue } }
        if (Test-Path $ZipFile) { Remove-Item $ZipFile -Force -ErrorAction SilentlyContinue }
        Write-Host "Installation failed."
    }
}

# Start installer
Install-Trumpet