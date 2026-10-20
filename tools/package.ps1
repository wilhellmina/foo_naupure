# Builds Release x86 and x64 and packs them into dist\foo_naupure-<version>.fb2k-component.
# The version comes from version.h.
#
#   powershell -ExecutionPolicy Bypass -File tools\package.ps1            (rebuild, then pack)
#   powershell -ExecutionPolicy Bypass -File tools\package.ps1 -NoBuild   (pack the DLLs already in bin\)
param([switch]$NoBuild)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot

$header = Get-Content -Raw (Join-Path $root 'version.h')
$numbers = foreach ($part in 'MAJOR', 'MINOR', 'PATCH') {
    if ($header -notmatch "#define\s+NAUPURE_VERSION_$part\s+(\d+)") { throw "NAUPURE_VERSION_$part not found in version.h" }
    $Matches[1]
}
$version = $numbers -join '.'

if (-not $NoBuild) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $msbuild = & $vswhere -latest -prerelease -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
    if (-not $msbuild) { throw 'MSBuild not found (is Visual Studio with C++ installed?)' }
    foreach ($platform in 'x86', 'x64') {
        & $msbuild (Join-Path $root 'foo_naupure.sln') /t:Rebuild /p:Configuration=Release /p:Platform=$platform /m /nologo /v:minimal
        if ($LASTEXITCODE -ne 0) { throw "Release|$platform build failed" }
    }
}

# foobar2000 takes the 32-bit DLL from the root of the package and the 64-bit one from x64\.
$entries = [ordered]@{
    'foo_naupure.dll'     = Join-Path $root 'bin\Win32\Release\foo_naupure.dll'
    'x64/foo_naupure.dll' = Join-Path $root 'bin\x64\Release\foo_naupure.dll'
}
foreach ($dll in $entries.Values) {
    if (-not (Test-Path $dll)) { throw "$dll not found" }
    # Catches packing stale DLLs with -NoBuild after the version was bumped.
    $built = (Get-Item $dll).VersionInfo.FileVersion
    if ($built -ne $version) { throw "$dll is version $built, but version.h says $version" }
}

$dist = Join-Path $root 'dist'
New-Item -ItemType Directory -Force $dist | Out-Null
$package = Join-Path $dist "foo_naupure-$version.fb2k-component"
if (Test-Path $package) { Remove-Item $package }

# Written entry by entry so the paths use '/' (Compress-Archive in Windows PowerShell writes '\').
Add-Type -AssemblyName System.IO.Compression, System.IO.Compression.FileSystem
$zip = [IO.Compression.ZipFile]::Open($package, [IO.Compression.ZipArchiveMode]::Create)
try {
    foreach ($entry in $entries.GetEnumerator()) {
        [void][IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, $entry.Value, $entry.Key, [IO.Compression.CompressionLevel]::Optimal)
    }
}
finally {
    $zip.Dispose()
}

Write-Host "Packed $package"

# Release notes: the "## <version>" section of CHANGELOG.md.
$notes = Join-Path $dist "release-notes-$version.md"
$section = @()
$inside = $false
foreach ($line in (Get-Content -Encoding UTF8 (Join-Path $root 'CHANGELOG.md'))) {
    if ($line -match '^## ') { $inside = $line -match "^##\s+$([regex]::Escape($version))\s*$"; continue }
    if ($inside) { $section += $line }
}
if ($section.Count -eq 0) {
    Write-Warning "CHANGELOG.md has no '## $version' section"
}
else {
    [IO.File]::WriteAllText($notes, (($section -join "`n").Trim() + "`n"), (New-Object Text.UTF8Encoding $false))
    Write-Host "Release notes: $notes"
}

$tag = "v$version"
if (-not (git -C $root tag --list $tag)) {
    Write-Host ''
    Write-Host "Tag $tag does not exist yet. To publish:"
    Write-Host "  git tag $tag"
    Write-Host "  git push origin $tag"
    Write-Host "  gh release create $tag `"$package`" --title `"foo_naupure $version`" --notes-file `"$notes`""
}
