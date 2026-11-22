$ErrorActionPreference = "Stop"

New-Item -ItemType Directory -Force -Path "build" | Out-Null

$cxx = "g++"
$flags = @("-std=c++14", "-Wall", "-Wextra", "-O2", "-Iinclude")
$coreSources = @(
    "src/core/expression.cpp",
    "src/core/types.cpp",
    "src/core/host_signals.cpp",
    "src/core/snapshot.cpp",
    "src/usb/descriptor_parser.cpp",
    "src/usb/win32_hub_enumerator.cpp",
    "src/platform/win32_system.cpp",
    "src/app/hub_monitor.cpp",
    "src/investigation/capture_recorder.cpp",
    "src/gui/main_window.cpp"
)
$libs = @("-static-libgcc", "-static-libstdc++", "-lsetupapi", "-lcomctl32", "-lgdi32", "-luser32", "-lshell32")

Write-Host "[Build] Compiling unoface.exe (Native Windows GUI)..." -ForegroundColor Cyan
& $cxx @flags @coreSources "src/main.cpp" -mwindows -o "build/unoface.exe" @libs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "[Build] Compiling test_unoface.exe..." -ForegroundColor Cyan
& $cxx @flags @coreSources "tests/test_unoface.cpp" -o "build/test_unoface.exe" @libs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "[Build] Build completed successfully." -ForegroundColor Green
