$ErrorActionPreference = 'Stop'

Add-Type -Namespace Native -Name Window -MemberDefinition @'
[System.Runtime.InteropServices.DllImport("user32.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
public static extern System.IntPtr FindWindow(string className, string windowName);
[System.Runtime.InteropServices.DllImport("user32.dll")]
public static extern bool PostMessage(System.IntPtr window, uint message, System.IntPtr wParam, System.IntPtr lParam);
'@
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class TrayLookup {
    [StructLayout(LayoutKind.Sequential)]
    public struct Identifier {
        public uint cbSize;
        public IntPtr hWnd;
        public uint uID;
        public Guid guidItem;
    }
    [StructLayout(LayoutKind.Sequential)]
    public struct Rect { public int left, top, right, bottom; }
    [DllImport("shell32.dll")]
    public static extern int Shell_NotifyIconGetRect(ref Identifier id, out Rect rect);
}
'@

$exe = Join-Path $PSScriptRoot '..\CapsAsShift.exe'
if ([Native.Window]::FindWindow('CapsAsShiftWindow', 'CapsAsShift') -ne [System.IntPtr]::Zero) {
    throw 'CapsAsShift is already running; exit it before running the smoke test'
}
$first = Start-Process -FilePath $exe -PassThru
try {
    $window = [System.IntPtr]::Zero
    for ($attempt = 0; $attempt -lt 30 -and $window -eq [System.IntPtr]::Zero; $attempt++) {
        Start-Sleep -Milliseconds 100
        $window = [Native.Window]::FindWindow('CapsAsShiftWindow', 'CapsAsShift')
    }
    if ($window -eq [System.IntPtr]::Zero) { throw 'App did not create its hidden window' }
    $tray = New-Object TrayLookup+Identifier
    $tray.cbSize = [System.Runtime.InteropServices.Marshal]::SizeOf($tray)
    $tray.hWnd = $window
    $tray.uID = 1
    $rect = New-Object TrayLookup+Rect
    if ([TrayLookup]::Shell_NotifyIconGetRect([ref] $tray, [ref] $rect) -ne 0) {
        throw 'Tray icon was not installed'
    }

    $second = Start-Process -FilePath $exe -PassThru
    if (-not $second.WaitForExit(3000) -or $second.ExitCode -ne 0) {
        throw 'Second instance did not exit successfully'
    }
    if (-not [Native.Window]::PostMessage($window, 0x111, [System.IntPtr]::new(2),
                                           [System.IntPtr]::Zero)) {
        throw 'Could not request tray exit'
    }
    if (-not $first.WaitForExit(3000) -or $first.ExitCode -ne 0) {
        throw 'First instance did not exit successfully'
    }
} finally {
    if (-not $first.HasExited) { $first.Kill(); $first.WaitForExit() }
}
