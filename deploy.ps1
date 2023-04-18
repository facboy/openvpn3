gsudo {
	sc.exe stop ovpnagent

	cd $args[0]

	robocopy "build\msvc\amd64\openvpn\omi\Release\\" "c:\bin\openvpn3\omi" /w:5
	robocopy "build\msvc\amd64\openvpn\ovpnagent\win\Release\" "c:\bin\openvpn3\ovpnagent" /w:5
	robocopy "build\msvc\amd64\test\ovpncli\Release\" "c:\bin\openvpn3\ovpncli" /w:5

	sc.exe start ovpnagent
} -args $PSScriptRoot
