#at top of script
if (!
    #current role
    (New-Object Security.Principal.WindowsPrincipal(
        [Security.Principal.WindowsIdentity]::GetCurrent()
    #is admin?
    )).IsInRole(
        [Security.Principal.WindowsBuiltInRole]::Administrator
    )
) {
    $cwd = [Environment]::CurrentDirectory
    #elevate script and exit current non-elevated runtime
    Start-Process `
        -FilePath 'powershell' `
        -ArgumentList (
            #flatten to single array
            '-File', $MyInvocation.MyCommand.Source, $args `
            | %{ $_ }
        ) `
        -WorkingDirectory $cwd `
        -Verb RunAs `
        -Wait
    exit
}

sc.exe stop ovpnagent
cd $PSScriptRoot
robocopy "build\openvpn\omi\Release\" "c:\bin\openvpn3\omi" /w:5
robocopy "build\openvpn\ovpnagent\win\Release\" "c:\bin\openvpn3\ovpnagent" /w:5
robocopy "build\test\ovpncli\Release\" "c:\bin\openvpn3\ovpncli" /w:5
sc.exe start ovpnagent
pause
