$ErrorActionPreference = 'Continue'
$dll = (Get-ChildItem -Recurse k -Filter openace32.dll | Select-Object -First 1).FullName
$bin = Split-Path $dll
Write-Host "DLL under test: $dll"
Write-Host "sha256: $((Get-FileHash $dll -Algorithm SHA256).Hash)"
# add bin dir to PATH so transitive deps resolve, set Win32 CWD, preload
$env:PATH = "$bin;$env:PATH"
[Environment]::CurrentDirectory = $bin
$src = @"
using System;
using System.Text;
using System.Runtime.InteropServices;
public static class AceProbe {
  [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)]
  static extern IntPtr LoadLibraryW(string lpFileName);
  [DllImport("kernel32.dll", SetLastError=true)]
  static extern IntPtr GetProcAddress(IntPtr hModule, string procName);
  [DllImport("openace32.dll", EntryPoint="AdsGetVersion", CallingConvention=CallingConvention.Cdecl, CharSet=CharSet.Ansi)]
  static extern uint AdsGetVersion(StringBuilder desc, ref ushort usLen, ref ushort maj, ref ushort min, ref ushort letter);
  public static int Run() {
    IntPtr h = LoadLibraryW("$($dll -replace '\\','\\')");
    if (h == IntPtr.Zero) { Console.WriteLine("LoadLibrary FAILED err=" + Marshal.GetLastWin32Error()); return 2; }
    Console.WriteLine("LoadLibrary OK handle=" + h.ToString("X"));
    IntPtr p = GetProcAddress(h, "AdsGetVersion");
    Console.WriteLine(p == IntPtr.Zero ? "GetProcAddress AdsGetVersion MISSING" : "GetProcAddress OK");
    var sb = new StringBuilder(128);
    ushort usLen = 128, maj = 99, min = 99, letter = 88;
    uint rc = AdsGetVersion(sb, ref usLen, ref maj, ref min, ref letter);
    Console.WriteLine("RC=" + rc);
    Console.WriteLine("MAJ=" + maj + " MIN=" + min + " LETTER=" + letter + " (" + (char)letter + ")");
    Console.WriteLine("USLEN=" + usLen);
    Console.WriteLine("DESC=[" + sb.ToString() + "]");
    if (maj == 0 && min == 0) { Console.WriteLine("WRAPPER-WOULD-RETURN=c1.9a (numbers zero, desc-parse fallback impossible? no: desc parse)"); }
    return 0;
  }
}
"@
Add-Type -TypeDefinition $src -Language CSharp
[AceProbe]::Run()
# wrapper emulation: parse desc
$dllBytes = [System.IO.File]::ReadAllBytes($dll)
$txt = [System.Text.Encoding]::ASCII.GetString($dllBytes)
$m = [regex]::Match($txt, 'OpenADS [0-9]+\.[0-9]+\.[0-9]+')
Write-Host ("STRING-IN-DLL: " + $(if ($m.Success) { $m.Value } else { 'NONE' }))
