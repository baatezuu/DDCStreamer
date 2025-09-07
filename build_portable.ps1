Param(
  [switch]$Clean,
  [switch]$Release,
  [switch]$NoBundle
)
$ErrorActionPreference = 'Stop'
$buildType = if ($Release) { 'Release' } else { 'Debug' }
$buildDir = "build_portable"
if ($Clean -and (Test-Path $buildDir)) { Remove-Item -Recurse -Force $buildDir }
if (!(Test-Path $buildDir)) { mkdir $buildDir | Out-Null }
cmake -S . -B $buildDir -G "MinGW Makefiles" -DPORTABLE_BUILD=ON -DSIMULATION_ONLY=ON -DCMAKE_BUILD_TYPE=$buildType
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed' }
cmake --build $buildDir --config $buildType -j 4
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
if (-not $NoBundle) { cmake --build $buildDir --target portable_bundle --config $buildType }
Write-Host "Portable build complete. Run .\\$buildDir\\portable\\DDCStreamerApp.exe"
