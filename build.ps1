# prepare
#$env:VCPKG_ROOT = "D:\git\vcpkg"
$cmake = "c:\Program Files\CMake\bin\cmake.exe"

$vcpkg_manifest_dir = Join-Path "$PSScriptRoot" "deps\vcpkg_manifests\windows"
Set-Location "$vcpkg_manifest_dir"
& 'd:\git\vcpkg\vcpkg' --vcpkg-root="d:\git\vcpkg" x-update-baseline
Set-Location "$PSScriptRoot"

$parallel = 15

& "$cmake" --preset my-release

# build targets
& "$cmake" --build --preset win-x64-release --target ovpnagent --parallel "$parallel"

& "$cmake" --build --preset win-x64-release --target omicliagent --parallel "$parallel"
& "$cmake" --build --preset win-x64-release --target omicli --parallel "$parallel"

& "$cmake" --build --preset win-x64-release --target ovpncliagent --parallel "$parallel"
& "$cmake" --build --preset win-x64-release --target ovpncli --parallel "$parallel"
