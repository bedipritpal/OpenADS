$ErrorActionPreference = 'Continue'
$dll = (Get-ChildItem -Recurse k -Filter openace32.dll | Select-Object -First 1).FullName
$bin = Split-Path $dll
Write-Host "DLL under test: $dll"
Write-Host "sha256: $((Get-FileHash $dll -Algorithm SHA256).Hash)"
$env:PATH = "$bin;$env:PATH"
[Environment]::CurrentDirectory = $bin
$src = @"
using System;
using System.Text;
using System.Runtime.InteropServices;
public static class AceProbe {
  // REAL signature from include/openads/ace.h (cdecl in implementation TU):
  // UNSIGNED32 AdsGetVersion(UNSIGNED32* pulMajor, UNSIGNED32* pulMinor,
  //                          UNSIGNED8* pucLetter, UNSIGNED8* pucDesc,
  //                          UNSIGNED16* pusDescLen)
  [DllImport("openace32.dll", EntryPoint="AdsGetVersion", CallingConvention=CallingConvention.Cdecl, CharSet=CharSet.Ansi)]
  static extern uint AdsGetVersion(ref uint maj, ref uint min, ref byte letter,
                                   [MarshalAs(UnmanagedType.LPStr)] StringBuilder desc, ref ushort descLen);
  public static int Run() {
    uint maj = 999, min = 999; byte letter = 0x3F; ushort descLen = 128;
    var sb = new StringBuilder(128);
    uint rc = AdsGetVersion(ref maj, ref min, ref letter, sb, ref descLen);
    Console.WriteLine("RC=" + rc);
    Console.WriteLine("MAJ=" + maj + " MIN=" + min + " LETTER=" + letter + " (" + (char)letter + ")");
    Console.WriteLine("DESCLEN-AFTER=" + descLen);
    string desc = sb.ToString();
    Console.WriteLine("DESC=[" + desc + "]");
    // exact wrapper emulation (oads_hb.c OADS_ADSVERSION)
    if (desc.StartsWith("OpenADS ")) {
      string tok = desc.Substring(8);
      int sp = tok.IndexOf(' ');
      if (sp > 0) tok = tok.Substring(0, sp);
      Console.WriteLine("WRAPPER-WOULD-RETURN=[" + tok + "]");
    } else {
      Console.WriteLine("WRAPPER-WOULD-RETURN=[" + maj + "." + min + (char)letter + "]  (fallback path)");
    }
    return 0;
  }
}
"@
Add-Type -TypeDefinition $src -Language CSharp
[AceProbe]::Run()
