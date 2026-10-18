param([string]$DllDir = ".")
Push-Location $DllDir
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
