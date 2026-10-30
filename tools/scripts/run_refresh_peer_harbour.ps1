# Candidate qualification only. Uses the existing Windows Harbour/MSVC64 route.
param([string]$OpenAdsBuild,[string]$HarbourRoot)
$ErrorActionPreference = 'Stop'
$OpenAdsBuild = (Resolve-Path $OpenAdsBuild).Path
$HarbourRoot = (Resolve-Path $HarbourRoot).Path
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$stage = Join-Path $env:RUNNER_TEMP 'refresh-peer-proof'
$data = Join-Path $stage 'data'
New-Item -ItemType Directory -Force $data | Out-Null
$mk = @('bin/win/msvc64/hbmk2.exe','bin/win/mingw64/hbmk2.exe','bin/hbmk2.exe') | ForEach-Object { Join-Path $HarbourRoot $_ } | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $mk) { throw 'Harbour hbmk2 unavailable' }
& cmake --build $OpenAdsBuild --config Release --target openads_serverd
if ($LASTEXITCODE -ne 0) { throw 'daemon build failed' }
$lib = Join-Path $OpenAdsBuild 'src/Release'
Copy-Item "$lib/openace64.dll" "$stage/openace64.dll" -Force
Copy-Item "$lib/openace64.dll" "$stage/ace64.dll" -Force
Copy-Item "$lib/openace64.lib" "$stage/ace64.lib" -Force
Copy-Item "$repo/tests/smoke/harbour/refresh_peer.prg" $stage -Force
Push-Location $stage
try {
 & $mk -comp=msvc64 -gtstd "-incpath=$HarbourRoot/contrib/rddads" refresh_peer.prg -lrddads "-L$stage" -lace64 -llegacy_stdio_definitions -loldnames
 if ($LASTEXITCODE -ne 0) { throw 'Harbour refresh proof compilation failed' }
 $daemon = Join-Path $OpenAdsBuild 'tools/serverd/Release/openads_serverd.exe'
 $p = Start-Process $daemon -ArgumentList @('--host','127.0.0.1','--allow_anonymous','--port','6288','--data',$data) -PassThru -RedirectStandardOutput server.txt -RedirectStandardError server-error.txt
 try {
  $ready=$false
  for($n=0;$n -lt 40;$n++) {
   if($p.HasExited) { throw 'daemon exited before ready' }
   $s=New-Object System.Net.Sockets.TcpClient
   try { $s.Connect('127.0.0.1',6288); $ready=$true } catch {} finally {$s.Dispose()}
   if($ready) {break}; Start-Sleep -Milliseconds 250
  }
  if(-not $ready) {throw 'daemon readiness timeout'}
  & ./refresh_peer.exe reader ('tcp://127.0.0.1:6288/'+($data -replace '\\','/')+'/') ($data+'\') *> result.txt
  $rc=$LASTEXITCODE
  Get-Content result.txt
  if($rc -ne 0 -or -not (Select-String result.txt -Pattern 'REFRESH-PEER-HARBOUR-PASS' -Quiet)) {throw 'actual Harbour refresh proof failed'}
 } finally {Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue}
} finally {Pop-Location}
