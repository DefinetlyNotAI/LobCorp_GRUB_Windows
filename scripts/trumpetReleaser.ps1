# Release Builder Script

$root = Resolve-Path ".."
$buildExe = Join-Path $root "build\Trumpet.exe"
$mediaDir = Join-Path $root "trumpet\.customTrumpets"
$releaseDir = Join-Path $root "releases"

if (!(Test-Path $releaseDir)) {
    New-Item $releaseDir -ItemType Directory | Out-Null
}

$exeOut = Join-Path $releaseDir "Trumpet.exe"
$zipOut = Join-Path $releaseDir "Trumpets.media.zip"

function Hash($f) {
    (Get-FileHash $f -Algorithm SHA256).Hash.ToLower()
}

# EXE
if (Test-Path $exeOut) {
    if ((Hash $exeOut) -eq (Hash $buildExe)) {
        if ((Read-Host "EXE unchanged. Replace? (Y/N)") -notmatch '^[Yy]') {
            Write-Host "Skipping EXE"
        } else {
            Copy-Item $buildExe $exeOut -Force
        }
    } else {
        Copy-Item $buildExe $exeOut -Force
    }
} else {
    Copy-Item $buildExe $exeOut
}

# ZIP
if (Test-Path $zipOut) {
    $tmp = Join-Path $env:TEMP "tmp_media.zip"
    Compress-Archive $mediaDir $tmp -Force

    if ((Hash $zipOut) -eq (Hash $tmp)) {
        if ((Read-Host "Media unchanged. Replace? (Y/N)") -notmatch '^[Yy]') {
            Remove-Item $tmp
        } else {
            Move-Item $tmp $zipOut -Force
        }
    } else {
        Move-Item $tmp $zipOut -Force
    }
} else {
    Compress-Archive $mediaDir $zipOut -Force
}

# Generate hashes
Get-ChildItem $releaseDir -File | ForEach-Object {
    $h = (Hash $_.FullName)
    Set-Content ($_.FullName + ".sha256") $h
}

Write-Host "Release ready."