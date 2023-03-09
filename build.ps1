# prepare
set VCPKG_ROOT="D:\git\vcpkg"
cmake --preset win-amd64-release

# build targets
cmake --build --preset win-amd64-release --target ovpnagent

cmake --build --preset win-amd64-release --target omicliagent
cmake --build --preset win-amd64-release --target omicli

cmake --build --preset win-amd64-release --target ovpncliagent
cmake --build --preset win-amd64-release --target ovpncli
