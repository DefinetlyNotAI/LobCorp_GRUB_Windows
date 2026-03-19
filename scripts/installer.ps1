# Trumpet Installer - Enterprise Grade

# Require Admin
$currentUser = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $currentUser.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator))
{
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
Add-Type -AssemblyName System.Net.Http

# Paths
$ProgramDir = "C:\Program Files\Trumpet"
$UserDir = "$env:USERPROFILE"
$ZipFile = Join-Path $env:TEMP "Trumpets.media.zip"

# URLs
$Urls = @{
    'Trumpet.exe' = 'https://github.com/DefinetlyNotAI/LobCorp_GRUB_Windows/releases/download/1.0.0-trumpet/Trumpet.exe'
    'LICENSE' = 'https://raw.githubusercontent.com/DefinetlyNotAI/LobCorp_GRUB_Windows/main/LICENSE'
    'Trumpet Info.md' = 'https://raw.githubusercontent.com/DefinetlyNotAI/LobCorp_GRUB_Windows/main/docs/Trumpet%20Info.md'
    'Zip' = 'https://github.com/DefinetlyNotAI/LobCorp_GRUB_Windows/releases/download/1.0.0-trumpet/Trumpets.media.zip'
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
function Log($msg)
{
Write-Host "[{0}] {1}" -f (Get-Date -Format 'HH:mm:ss'), $msg
}

# Close running Trumpet
function Close-TrumpetIfRunning
{
    $proc = Get-Process -Name Trumpet -ErrorAction SilentlyContinue
    if ($proc)
    {
        Log "Trumpet is running, closing..."
        $proc | Stop-Process -Force
    }
}

# Download file with synchronous progress and file size info
function DownloadFileHttpClient($url, $dest, [int]$weight=0) {
    $client = [System.Net.Http.HttpClient]::new()
    $resp = $client.GetAsync($url, [System.Net.Http.HttpCompletionOption]::ResponseHeadersRead).Result

    $total = $resp.Content.Headers.ContentLength
    if ($total) { Log "Downloading $url ($([math]::Round($total/1MB,2)) MB) -> $dest" } else { Log "Downloading $url -> $dest" }

    $stream = $resp.Content.ReadAsStreamAsync().Result
    $fs = [System.IO.File]::Open($dest, [System.IO.FileMode]::Create)

    $buffer = New-Object byte[] 8192
    $read = 0
    $bytesRead = 0
    while (($read = $stream.Read($buffer,0,$buffer.Length)) -gt 0) {
        $fs.Write($buffer,0,$read)
        $bytesRead += $read
        if ($total) {
            $progress.Value = [int](($bytesRead / $total) * $weight)
            $form.Refresh()
        }
    }

    $fs.Close()
    $stream.Close()
    $resp.Dispose()
    $client.Dispose()

    if ($weight -gt 0 -and -not $total) { $progress.Value += $weight; $form.Refresh() }

    Log "Downloaded $dest"
}

# Install function
function Install-Trumpet
{
    Close-TrumpetIfRunning

    $created = @()
    try
    {
        # Create program dir
        if (!(Test-Path $ProgramDir))
        {
            New-Item -ItemType Directory -Path $ProgramDir -Force | Out-Null
            Log "Created directory $ProgramDir"
        }

        # Download main files (50% progress)
        $files = @('Trumpet.exe', 'LICENSE', 'Trumpet Info.md')
        $fileWeight = 50 / $files.Count
        $currentProgress = 0

        foreach ($f in $files)
        {
            $dest = Join-Path $ProgramDir $f
            if (Test-Path $dest)
            {
                Log "$f exists, overwriting..."
            }
            DownloadFileHttpClient $Urls[$f] $dest
            $created += $dest
            $currentProgress += $fileWeight
            $progress.Value = [int]$currentProgress
            $form.Refresh()
        }

        # Download and extract ZIP (30%)
        DownloadFileHttpClient $Urls['Zip'] $ZipFile
        $progress.Value = 60
        $form.Refresh()
        Log "Extracting ZIP..."
        Expand-Archive -Path $ZipFile -DestinationPath $UserDir -Force
        Remove-Item $ZipFile -Force -ErrorAction SilentlyContinue
        $progress.Value = 90
        $form.Refresh()
        Log "Extraction complete"

        # Setup startup (5%)
        if ($chkStartup.Checked)
        {
            if (!(Get-ItemProperty -Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run' -Name 'Trumpet' -ErrorAction SilentlyContinue))
            {
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

        $progress.Value = 100
        [System.Windows.Forms.MessageBox]::Show("Installation complete.`nUninstaller: $UninstallFile", 'Install Complete')
        Log "Installation completed successfully"

    }
    catch
    {
        Log "Installation failed: $_"
        # rollback
        foreach ($f in $created)
        {
            if (Test-Path $f)
            {
                Remove-Item $f -Force -ErrorAction SilentlyContinue
            }
        }
        if (Test-Path $ZipFile)
        {
            Remove-Item $ZipFile -Force -ErrorAction SilentlyContinue
        }
        [System.Windows.Forms.MessageBox]::Show("Installation failed: $_", 'Error')
    }
}

# Button click
$btnInstall.Add_Click({
    $btnInstall.Enabled = $false
    Install-Trumpet
    $btnInstall.Enabled = $true
})

$form.Topmost = $true
$form.Add_Shown({ $form.Activate() })
[void]$form.ShowDialog()
