<#
.SYNOPSIS
  GitHub Releases에서 OCCT V8.0.1 Windows(vc14, x64) 바이너리 패키지를 내려받아
  third_party/occt (+ third_party/3rdparty)에 풀어 놓는다.

.DESCRIPTION
  릴리스 페이지: https://github.com/Open-Cascade-SAS/OCCT/releases/tag/V8.0.1

  패키지 (각각 내부에 zip 하나를 더 담고 있는 zip):
    occt-combined-release-no-pch.zip      ~245 MB  OCCT Release + 모든 서드파티 라이브러리
    occt-combined-with-debug-no-pch.zip   ~443 MB  OCCT Release + Debug (bind/libd) + 서드파티

  결과 레이아웃 (Directory.Build.props가 기대하는 구조):
    third_party/occt/inc                 헤더
    third_party/occt/win64/vc14/bin|lib  Release DLL / 임포트 라이브러리
    third_party/occt/win64/vc14/bind|libd Debug DLL / 임포트 라이브러리 (with-debug 패키지에만 포함)
    third_party/3rdparty/<lib>/bin       freetype, tbb, freeimage, ...

.PARAMETER WithDebug
  Debug 바이너리까지 포함된 패키지를 내려받는다 (Debug|x64 빌드에 필요).

.PARAMETER Force
  third_party/occt가 이미 있어도 다시 내려받고 다시 푼다.

.EXAMPLE
  .\scripts\setup-occt.ps1 -WithDebug
#>
[CmdletBinding()]
param(
    [switch]$WithDebug,
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'   # 진행률 표시줄을 끄면 Invoke-WebRequest가 훨씬 빨라진다

$Version    = 'V8.0.1'
$Asset      = if ($WithDebug) { 'occt-combined-with-debug-no-pch.zip' } else { 'occt-combined-release-no-pch.zip' }
$Url        = "https://github.com/Open-Cascade-SAS/OCCT/releases/download/$Version/$Asset"

$RepoRoot   = Split-Path -Parent $PSScriptRoot
$ThirdParty = Join-Path $RepoRoot 'third_party'
$Downloads  = Join-Path $ThirdParty '_downloads'
$OcctDir    = Join-Path $ThirdParty 'occt'
$LibsDir    = Join-Path $ThirdParty '3rdparty'
$ZipPath    = Join-Path $Downloads $Asset

if ((Test-Path (Join-Path $OcctDir 'win64\vc14\bin\TKernel.dll')) -and -not $Force) {
    Write-Host "OCCT already present at $OcctDir (use -Force to re-install)." -ForegroundColor Green
    exit 0
}

New-Item -ItemType Directory -Force $Downloads | Out-Null

if (-not (Test-Path $ZipPath) -or $Force) {
    Write-Host "Downloading $Url ..." -ForegroundColor Cyan
    Invoke-WebRequest -Uri $Url -OutFile $ZipPath -UseBasicParsing
}
Write-Host ("Package: {0} ({1:N0} MB)" -f $ZipPath, ((Get-Item $ZipPath).Length / 1MB))

# 바깥 zip -> 안쪽 zip (opencascade-8.0.1-vc14-64-*.zip)
Write-Host 'Extracting outer archive ...' -ForegroundColor Cyan
Expand-Archive -Path $ZipPath -DestinationPath $Downloads -Force
$Inner = Get-ChildItem $Downloads -Filter 'opencascade-8.0.1-vc14-64*.zip' | Select-Object -First 1
if ($null -eq $Inner) { throw "Inner archive not found in $Downloads" }

$Extract = Join-Path $ThirdParty '_extract'
if (Test-Path $Extract) { Remove-Item -Recurse -Force $Extract }
Write-Host "Extracting $($Inner.Name) (this takes a few minutes, ~1.8 GB) ..." -ForegroundColor Cyan
Expand-Archive -Path $Inner.FullName -DestinationPath $Extract -Force

foreach ($pair in @(@{ From = 'opencascade-8.0.1-vc14-64'; To = $OcctDir }, @{ From = '3rdparty-vc14-64'; To = $LibsDir })) {
    $src = Join-Path $Extract $pair.From
    if (-not (Test-Path $src)) { throw "Expected folder missing in archive: $($pair.From)" }
    if (Test-Path $pair.To) { Remove-Item -Recurse -Force $pair.To }
    Move-Item -Path $src -Destination $pair.To
}
Remove-Item -Recurse -Force $Extract

Write-Host ''
Write-Host "OCCT $Version installed:" -ForegroundColor Green
Write-Host "  headers : $OcctDir\inc"
Write-Host "  release : $OcctDir\win64\vc14\bin"
if (Test-Path (Join-Path $OcctDir 'win64\vc14\bind')) { Write-Host "  debug   : $OcctDir\win64\vc14\bind" }
Write-Host "  3rdparty: $LibsDir"
Write-Host ''
Write-Host 'Next: .\scripts\build.ps1   (or open OpenCascade-Lab.sln in Visual Studio)'
