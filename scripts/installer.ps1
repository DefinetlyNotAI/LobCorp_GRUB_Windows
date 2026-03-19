# Trumpet Installer Script (fixed)

# Require admin
$currentUser = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
$isAdmin = $currentUser.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Add-Type -AssemblyName System.Windows.Forms
    [System.Windows.Forms.MessageBox]::Show(
            "This installer must be run as administrator.",
            "Administrator Required",
            [System.Windows.Forms.MessageBoxButtons]::OK,
            [System.Windows.Forms.MessageBoxIcon]::Error
    )
    exit 1
}

# Force TLS 1.2
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

# Paths
$ProgramDir = "C:\Program Files\Trumpet"
$UserDir = "$env:USERPROFILE"
$ZipFile = Join-Path $env:TEMP "Trumpets.media.zip"

# URLs (RAW, not blob)
$Urls = @{
    'Trumpet.exe'     = 'https://github.com/DefinetlyNotAI/LobCorp_GRUB_Windows/releases/download/1.0.0-trumpet/Trumpet.exe'
    'LICENSE'         = 'https://raw.githubusercontent.com/DefinetlyNotAI/LobCorp_GRUB_Windows/main/LICENSE'
    'Trumpet Info.md' = 'https://raw.githubusercontent.com/DefinetlyNotAI/LobCorp_GRUB_Windows/main/docs/Trumpet%20Info.md'
    'Zip'             = 'https://github.com/DefinetlyNotAI/LobCorp_GRUB_Windows/releases/download/1.0.0-trumpet/Trumpets.media.zip'
}

# Form
$form = New-Object System.Windows.Forms.Form
$form.Text = 'Trumpet Installer'
$form.Size = New-Object System.Drawing.Size(500,300)
$form.StartPosition = 'CenterScreen'

$lbl = New-Object System.Windows.Forms.Label
$lbl.Text = 'Install Trumpet'
$lbl.Font = New-Object System.Drawing.Font('Segoe UI',16,[System.Drawing.FontStyle]::Bold)
$lbl.AutoSize = $true
$lbl.Location = New-Object System.Drawing.Point(150,20)
$form.Controls.Add($lbl)

$chkStartup = New-Object System.Windows.Forms.CheckBox
$chkStartup.Text = 'Run Trumpet on Windows startup'
$chkStartup.AutoSize = $true
$chkStartup.Location = New-Object System.Drawing.Point(150,150)
$form.Controls.Add($chkStartup)

$progress = New-Object System.Windows.Forms.ProgressBar
$progress.Location = New-Object System.Drawing.Point(50,200)
$progress.Size = New-Object System.Drawing.Size(400,20)
$progress.Minimum = 0
$progress.Maximum = 100
$form.Controls.Add($progress)

$btnInstall = New-Object System.Windows.Forms.Button
$btnInstall.Text = 'Install'
$btnInstall.Location = New-Object System.Drawing.Point(200,230)
$btnInstall.Size = New-Object System.Drawing.Size(100,30)
$form.Controls.Add($btnInstall)

function Download($url, $dest) {
    $tmp = "$dest.tmp"
    Invoke-WebRequest -Uri $url -OutFile $tmp -UseBasicParsing
    Move-Item $tmp $dest -Force
}

function Install-Trumpet {
    $created = @()
    try {
        if (!(Test-Path $ProgramDir)) {
            New-Item -ItemType Directory -Path $ProgramDir -Force | Out-Null
        }

        $files = @('Trumpet.exe','LICENSE','Trumpet Info.md')
        $step = 60 / $files.Count
        $current = 0

        foreach ($f in $files) {
            $dest = Join-Path $ProgramDir $f
            Download $Urls[$f] $dest
            $created += $dest

            $current += $step
            $progress.Value = [int]$current
            $form.Refresh()
        }

        # ZIP
        Download $Urls['Zip'] $ZipFile
        $progress.Value = 70
        $form.Refresh()

        Expand-Archive -Path $ZipFile -DestinationPath $UserDir -Force
        Remove-Item $ZipFile -Force -ErrorAction SilentlyContinue

        $progress.Value = 85
        $form.Refresh()

        # Startup
        if ($chkStartup.Checked) {
            New-ItemProperty `
                -Path "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run" `
                -Name "Trumpet" `
                -Value "`"$ProgramDir\Trumpet.exe`"" `
                -PropertyType String -Force | Out-Null
        }

        # Uninstaller
        $UninstallFile = Join-Path $ProgramDir "uninstall-trumpet.ps1"
        $uninstallScript = @"
Add-Type -AssemblyName System.Windows.Forms

`$ProgramDir = '$ProgramDir'
`$UserDir = '$UserDir'

if (Test-Path `$ProgramDir) {
    Remove-Item -Path `$ProgramDir -Recurse -Force -ErrorAction SilentlyContinue
}

if (Test-Path "`$UserDir\.customTrumpets") {
    Remove-Item -Path "`$UserDir\.customTrumpets" -Recurse -Force -ErrorAction SilentlyContinue
}

Remove-ItemProperty -Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run' -Name 'Trumpet' -ErrorAction SilentlyContinue

[System.Windows.Forms.MessageBox]::Show('Trumpet uninstalled successfully.','Uninstall Complete')
"@

        $uninstallScript | Out-File -FilePath $UninstallFile -Encoding UTF8 -Force

        $progress.Value = 100
        [System.Windows.Forms.MessageBox]::Show("Installation complete.`nUninstaller: $UninstallFile",'Install Complete')
    }
    catch {
        # rollback
        foreach ($f in $created) {
            if (Test-Path $f) { Remove-Item $f -Force -ErrorAction SilentlyContinue }
        }
        if (Test-Path $ZipFile) { Remove-Item $ZipFile -Force -ErrorAction SilentlyContinue }

        [System.Windows.Forms.MessageBox]::Show("Installation failed: $_",'Error')
    }
}

# Run install in background to avoid UI freeze
$btnInstall.Add_Click({
    $btnInstall.Enabled = $false
    Install-Trumpet
    $btnInstall.Enabled = $true
})

$form.Topmost = $true
$form.Add_Shown({$form.Activate()})
[void]$form.ShowDialog()
