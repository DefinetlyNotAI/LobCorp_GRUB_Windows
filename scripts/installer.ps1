# Trumpet Installer - Enterprise Grade with App Registry Uninstall & Responsive UI

# Require Admin
$currentUser = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $currentUser.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Add-Type -AssemblyName System.Windows.Forms
    [System.Windows.Forms.MessageBox]::Show(
            "This installer must be run as administrator.",
            "Administrator Required",
            [System.Windows.Forms.MessageBoxButtons]::OK,
            [System.Windows.Forms.MessageBoxIcon]::Error
    )
    exit 1
}

# Force TLS1.2
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

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

# Form UI
$form = New-Object System.Windows.Forms.Form
$form.Text = 'Trumpet Installer'
$form.Size = New-Object System.Drawing.Size(500, 300)
$form.StartPosition = 'CenterScreen'

$lbl = New-Object System.Windows.Forms.Label
$lbl.Text = 'Install Trumpet'
$lbl.Font = New-Object System.Drawing.Font('Segoe UI', 16, [System.Drawing.FontStyle]::Bold)
$lbl.AutoSize = $true
$lbl.Location = New-Object System.Drawing.Point(150, 20)
$form.Controls.Add($lbl)

$chkStartup = New-Object System.Windows.Forms.CheckBox
$chkStartup.Text = 'Run Trumpet on Windows startup'
$chkStartup.AutoSize = $true
$chkStartup.Location = New-Object System.Drawing.Point(150, 150)
$form.Controls.Add($chkStartup)

$progress = New-Object System.Windows.Forms.ProgressBar
$progress.Location = New-Object System.Drawing.Point(50, 200)
$progress.Size = New-Object System.Drawing.Size(400, 20)
$progress.Minimum = 0
$progress.Maximum = 100
$form.Controls.Add($progress)

$btnInstall = New-Object System.Windows.Forms.Button
$btnInstall.Text = 'Install'
$btnInstall.Location = New-Object System.Drawing.Point(200, 230)
$btnInstall.Size = New-Object System.Drawing.Size(100, 30)
$form.Controls.Add($btnInstall)

# Logging helper
function Log($msg) {
    Write-Host "[{0}] {1}" -f (Get-Date -Format 'HH:mm:ss'), $msg
}

# Close running Trumpet
function Close-TrumpetIfRunning {
    $proc = Get-Process -Name Trumpet -ErrorAction SilentlyContinue
    if ($proc) {
        Log "Trumpet is running, closing..."
        $proc | Stop-Process -Force
    }
}

# Synchronous download with optional progress weight
function DownloadFile($url, $dest, [int]$weight=0) {
    $sizeMB = 0
    try {
        $req = [System.Net.WebRequest]::Create($url)
        $req.Method = "HEAD"
        $resp = $req.GetResponse()
        $total = $resp.Headers["Content-Length"]
        $resp.Close()
        if ($total) { $sizeMB = [math]::Round($total/1MB,2) }
    } catch { $total = 0 }

    if ($sizeMB -gt 0) { Log "Downloading $url ($sizeMB MB) -> $dest" } else { Log "Downloading $url -> $dest" }

    Invoke-WebRequest -Uri $url -OutFile $dest -UseBasicParsing -ErrorAction Stop
    Log "Downloaded $dest"

    if ($weight -gt 0) {
        $progress.Value += $weight
        if ($progress.Value -gt 100) { $progress.Value = 100 }
        $form.Refresh()
    }
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

        # Download main files (50%)
        $files = @('Trumpet.exe','LICENSE','Trumpet Info.md')
        $fileWeight = 50 / $files.Count
        foreach ($f in $files) {
            $dest = Join-Path $ProgramDir $f
            if (Test-Path $dest) { Log "$f exists, overwriting..." }
            DownloadFile $Urls[$f] $dest $fileWeight
            $created += $dest
        }

        # Download and extract ZIP (30%)
        DownloadFile $Urls['Zip'] $ZipFile 30
        Log "Extracting ZIP..."
        Expand-Archive -Path $ZipFile -DestinationPath $UserDir -Force
        Remove-Item $ZipFile -Force -ErrorAction SilentlyContinue
        $progress.Value = 90
        $form.Refresh()
        Log "Extraction complete"

        # Setup startup (5%)
        if ($chkStartup.Checked) {
            if (!(Get-ItemProperty -Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run' -Name 'Trumpet' -ErrorAction SilentlyContinue)) {
                New-ItemProperty -Path "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run" -Name "Trumpet" -Value "`"$ProgramDir\Trumpet.exe`"" -PropertyType String -Force | Out-Null
                Log "Added Trumpet to startup"
            }
        }

        # Create uninstaller (5%)
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

        $progress.Value = 100
        [System.Windows.Forms.MessageBox]::Show("Installation complete.`nUninstaller: $UninstallFile",'Install Complete')
        Log "Installation completed successfully"

    } catch {
        Log "Installation failed: $_"
        # rollback
        foreach ($f in $created) { if (Test-Path $f) { Remove-Item $f -Force -ErrorAction SilentlyContinue } }
        if (Test-Path $ZipFile) { Remove-Item $ZipFile -Force -ErrorAction SilentlyContinue }
        [System.Windows.Forms.MessageBox]::Show("Installation failed: $_",'Error')
    }
}

# BackgroundWorker to keep UI responsive
$worker = New-Object System.ComponentModel.BackgroundWorker
$worker.WorkerReportsProgress = $true

# Register DoWork event
Register-ObjectEvent -InputObject $worker -EventName DoWork -Action {
    Install-Trumpet
}

# Register RunWorkerCompleted event
Register-ObjectEvent -InputObject $worker -EventName RunWorkerCompleted -Action {
    $btnInstall.Enabled = $true
}

# Button click triggers the worker
$btnInstall.Add_Click({
    $btnInstall.Enabled = $false
    $worker.RunWorkerAsync()
})

$form.Topmost = $true
$form.Add_Shown({ $form.Activate() })
[void]$form.ShowDialog()