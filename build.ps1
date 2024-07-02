# prepare
set VCPKG_ROOT="D:\git\vcpkg"

$vcpkg_manifest_dir = Join-Path "$PSScriptRoot" "deps\vcpkg_manifests\windows"
Set-Location "$vcpkg_manifest_dir"
vcpkg x-update-baseline
Set-Location "$PSScriptRoot"

cmake --preset win-amd64-release

# build targets
cmake --build --preset win-amd64-release --target ovpnagent

cmake --build --preset win-amd64-release --target omicliagent
cmake --build --preset win-amd64-release --target omicli

cmake --build --preset win-amd64-release --target ovpncliagent
cmake --build --preset win-amd64-release --target ovpncli
