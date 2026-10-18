param([string]$DllDir = ".")
Push-Location $DllDir
Write-Host ("Is64BitProcess=" + [Environment]::Is64BitProcess)
Write-Host ("cwd=" + (Get-Location).Path)
Write-Host ("dll exists=" + (Test-Path ".\openace32.dll"))
Add-Type -Namespace K -Name L -MemberDefinition '[DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)] public static extern System.IntPtr LoadLibrary(string p); [DllImport("kernel32.dll")] public static extern bool FreeLibrary(System.IntPtr h);'
$full = (Resolve-Path ".\openace32.dll").Path
Write-Host ("full path=" + $full)
$h = [K.L]::LoadLibrary($full)
if ($h -eq [System.IntPtr]::Zero) {
  Write-Host ("LoadLibrary FAILED err=" + [System.Runtime.InteropServices.Marshal]::GetLastWin32Error())
} else {
  Write-Host "LoadLibrary OK"
  [K.L]::FreeLibrary($h) | Out-Null
}
$src = @"
using System;
using System.Runtime.InteropServices;
using System.Text;
public class AceProbe {
  [DllImport("openace32.dll", EntryPoint="AdsGetVersion", ExactSpelling=true, CallingConvention=CallingConvention.Cdecl)]
  public static extern uint AdsGetVersion(ref uint maj, ref uint minn, ref byte letter, [Out] byte[] desc, ref ushort descLen);
  public static void Run() {
    uint maj = 0; uint minn = 0; byte letter = 0;
    byte[] desc = new byte[128]; ushort n = 128;
    uint rc = AdsGetVersion(ref maj, ref minn, ref letter, desc, ref n);
    int used = n > 128 ? 128 : (int)n;
    string d = Encoding.ASCII.GetString(desc, 0, used);
    Console.WriteLine("rc=" + rc + " maj=" + maj + " min=" + minn + " letter=" + (char)letter + " usLen=" + n);
    Console.WriteLine("desc=[" + d + "]");
    string ver;
    if (d.StartsWith("OpenADS ")) {
      int sp = d.IndexOf(' ', 8);
      ver = sp > 8 ? d.Substring(8, sp - 8) : d.Substring(8);
    } else {
      ver = maj + "." + minn + ((char)letter).ToString();
    }
    Console.WriteLine("WRAPPER-WOULD-RETURN=[" + ver + "]");
  }
}
"@
Add-Type -TypeDefinition $src
[AceProbe]::Run()
Pop-Location
