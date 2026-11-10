# Exercise the packaged DLL through its unchanged and additive native C APIs.
import ctypes as C,os,pathlib,subprocess,sys,tempfile,re
if len(sys.argv)==2:
 dll=str(pathlib.Path(sys.argv[1]).resolve())
 for compact in ('0','1'):
  for mode in ('on','master-off','wire-off'):
   with tempfile.TemporaryDirectory(prefix='tim77mark_') as d:
    root=pathlib.Path(d);env=os.environ.copy();(root/'empty.ini').write_text('')
    env.update(OPENADS_INI=str(root/'empty.ini'),OPENADS_WIRE_TRACE='0' if mode=='wire-off' else '1',OPENADS_WIRE_TRACE_COMPACT=compact,OPENADS_WIRE_TRACE_FILE=str(root/'wire.log'))
    subprocess.run([sys.executable,__file__,dll,mode],env=env,check=True,timeout=30)
    if mode=='on':
     rows=(root/'wire.log').read_text().splitlines();assert len(rows)==3,rows
     matches=[re.search(r'tick_us\.(.{20})  auto_thread\.(.{20})  instance\.(.{16})  thread\.(.{16})  marker\.(.*)$',x) for x in rows]
     assert all(matches),rows
     assert [(int(x[3]),int(x[4])) for x in matches]==[(0,0),(9999999999999999,42),(12,7)]
     assert len({x.index('marker.') for x in rows})==1
     assert not any('wire  op.' in x for x in rows)
    else:assert not (root/'wire.log').exists()
    print('PACKAGED-MSVC-MARKER-PASS',compact,mode)
 sys.exit()
dll,mode=sys.argv[1:];l=C.CDLL(dll)
u32=C.c_uint32;u64=C.c_uint64;p=C.c_char_p
l.OAdsSetLogging.argtypes=[C.c_uint16];l.OAdsSetLogging.restype=u32
l.OAdsTraceMark.argtypes=[p,u32];l.OAdsTraceMark.restype=u32
l.OAdsTraceMarkEx.argtypes=[p,u32,u64,u64];l.OAdsTraceMarkEx.restype=u32
assert l.OAdsSetLogging(0 if mode=='master-off' else 1)==0
expected=mode=='on'
assert (l.OAdsTraceMark(b'OLD_ABI',7)==0)==expected
assert (l.OAdsTraceMarkEx(b'IDS_MAX',7,9999999999999999,42)==0)==expected
assert (l.OAdsTraceMarkEx(b'IDS_SHORT',9,12,7)==0)==expected
for label in (b'',b'x'*121,b'with\0nul',b'with\nnewline'):
 assert l.OAdsTraceMarkEx(label,len(label),0,0)!=0
assert l.OAdsTraceMarkEx(b'BAD_ID',6,10000000000000000,0)!=0
