<#
.SYNOPSIS
  Visual Studio에 포함된 MSBuild로 OpenCascade-Lab.sln(x64)을 빌드한다.

.DESCRIPTION
  `dotnet build`는 C++/CLI 프로젝트(OcctProxy.vcxproj)를 컴파일하지 못하므로,
  이 스크립트는 vswhere로 MSBuild.exe를 찾아 솔루션 전체를 빌드한다.

.PARAMETER Configuration
  Debug 또는 Release(기본값). Debug는 "with-debug" OCCT 패키지(bind/ + libd/)가 필요하다.

.EXAMPLE
  .\scripts\build.ps1
  .\scripts\build.ps1 -Configuration Debug
#>
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'
$RepoRoot = Split-Path -Parent $PSScriptRoot
$Solution = Join-Path $RepoRoot 'OpenCascade-Lab.sln'

if (-not (Test-Path (Join-Path $RepoRoot 'third_party\occt\inc\Standard_Version.hxx'))) {
    throw 'OCCT not found in third_party/occt. Run scripts\setup-occt.ps1 first.'
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) { throw 'vswhere.exe not found - is Visual Studio installed?' }

# 필요: MSBuild + C++ 툴셋 + C++/CLI 지원
$msbuild = & $vswhere -latest -products * `
    -requires Microsoft.Component.MSBuild Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (-not $msbuild) { throw 'MSBuild with the C++ toolset was not found. Install the "Desktop development with C++" workload and "C++/CLI support".' }

Write-Host "MSBuild : $msbuild"
Write-Host "Config  : $Configuration | x64"

$target = if ($Clean) { 'Rebuild' } else { 'Build' }
& $msbuild $Solution "/t:$target" "/p:Configuration=$Configuration" '/p:Platform=x64' '/restore' '/m' '/nologo' '/v:minimal'
if ($LASTEXITCODE -ne 0) { throw "Build failed (exit code $LASTEXITCODE)" }

Write-Host ''
Write-Host 'Build succeeded.' -ForegroundColor Green
Write-Host "  CLI   : src\OcctLab.Cli\bin\x64\$Configuration\net8.0\OcctLab.Cli.exe"
Write-Host "  Viewer: src\OcctLab.Viewer\bin\x64\$Configuration\net8.0-windows\OcctLab.Viewer.exe"
