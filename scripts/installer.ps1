# Trumpet Installer Script

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.IO.Compression.FileSystem

# Paths
$ProgramDir = "C:\Program Files\Trumpet"
$UserDir = "$env:USERPROFILE"
$ZipFile = "$env:TEMP\Trumpets.media.zip"

# URLs
$Urls = @{
    'Trumpet.exe' = ''
    'LICENSE' = ''
    'Trumpet Info.md' = ''
    'Zip' = ''
}

# Form setup
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
$progress.Style = 'Continuous'
$form.Controls.Add($progress)

$btnInstall = New-Object System.Windows.Forms.Button
$btnInstall.Text = 'Install'
$btnInstall.Location = New-Object System.Drawing.Point(200,230)
$btnInstall.Size = New-Object System.Drawing.Size(100,30)
$form.Controls.Add($btnInstall)

function Install-Trumpet {
    try {
        if (!(Test-Path $ProgramDir)) { New-Item -ItemType Directory -Path $ProgramDir -Force }

        # Download main files
        $files = @('Trumpet.exe','LICENSE','Trumpet Info.md')
        foreach ($f in $files) {
            $progress.Value += 10
            $dest = Join-Path $ProgramDir $f
            Invoke-WebRequest -Uri $Urls[$f] -OutFile $dest
        }

        # Download and unzip .customTrumpets in user profile
        Invoke-WebRequest -Uri $Urls['Zip'] -OutFile $ZipFile
        [System.IO.Compression.ZipFile]::ExtractToDirectory($ZipFile, $UserDir, $true)
        Remove-Item $ZipFile -Force

        # Add to startup if checked
        if ($chkStartup.Checked) {
            New-ItemProperty -Path "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run" `
                             -Name "Trumpet" -Value "$ProgramDir\Trumpet.exe" -PropertyType String -Force
        }

        # Create uninstaller
        $UninstallFile = "$ProgramDir\uninstall-trumpet.ps1"
        $uninstallScript = @"
Remove-Item -Path '$ProgramDir\*' -Recurse -Force
Remove-Item -Path '$UserDir\.customTrumpets' -Recurse -Force
Remove-ItemProperty -Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run' -Name 'Trumpet' -ErrorAction SilentlyContinue
[System.Windows.Forms.MessageBox]::Show('Trumpet uninstalled successfully.','Uninstall Complete',[System.Windows.Forms.MessageBoxButtons]::OK,[System.Windows.Forms.MessageBoxIcon]::Information)
"@
        $uninstallScript | Out-File -FilePath $UninstallFile -Encoding UTF8 -Force

        $progress.Value = 100
        [System.Windows.Forms.MessageBox]::Show("Installation complete!`nAn uninstaller has been created at $UninstallFile",'Install Complete',[System.Windows.Forms.MessageBoxButtons]::OK,[System.Windows.Forms.MessageBoxIcon]::Information)
    } catch {
        [System.Windows.Forms.MessageBox]::Show("Installation failed: $_",'Error',[System.Windows.Forms.MessageBoxButtons]::OK,[System.Windows.Forms.MessageBoxIcon]::Error)
    }
}

$btnInstall.Add_Click({
    $progress.Value = 20
    Install-Trumpet
})

$form.Topmost = $true
$form.Add_Shown({$form.Activate()})
[void]$form.ShowDialog()