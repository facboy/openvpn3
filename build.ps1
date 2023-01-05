# prepare
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=D:\git\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_OVERLAY_PORTS=deps\vcpkg-ports -DCLI_OVPNDCOWIN=ON

# build targets
cmake --build build --config Release --target ovpnagent

cmake --build build --config Release --target omicliagent
cmake --build build --config Release --target omicli

cmake --build build --config Release --target ovpncliagent
cmake --build build --config Release --target ovpncli
