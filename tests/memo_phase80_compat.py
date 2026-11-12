"""Generated memo cursor and default-OFF/opt-in server timing proof; no user data."""
import argparse, ctypes as C, json, os, pathlib, socket, subprocess, tempfile, time
p=argparse.ArgumentParser();p.add_argument('--client',required=True);p.add_argument('--server',required=True);a=p.parse_args()
lib=C.CDLL(str(pathlib.Path(a.client).resolve()));H=C.c_uint32;U=C.c_uint16;S=C.c_char_p;P=C.POINTER(H)
specs={'AdsConnect60':[S,U,S,S,H,P],'AdsCreateTable':[H,S,S,U,U,U,U,U,S,P],'AdsOpenTable':[H,S,S,U,U,U,U,U,P],'AdsCloseTable':[H],'AdsDisconnect':[H],'AdsAppendRecord':[H],'AdsSetString':[H,S,S,H],'AdsWriteRecord':[H],'AdsGotoTop':[H],'AdsGotoBottom':[H],'AdsSkip':[H,C.c_int32],'AdsGetMemoLength':[H,S,P],'AdsGetString':[H,S,S,P,H]}
for n,types in specs.items():f=getattr(lib,n);f.argtypes=types;f.restype=H
def ok(n,*args):
 rc=getattr(lib,n)(*args);assert rc==0,(n,rc)
with tempfile.TemporaryDirectory(prefix='phase80_native_') as tmp:
 root=pathlib.Path(tmp);data=root/'data';data.mkdir();c=H();t=H()
 env=os.environ.copy();env['OPENADS_INI']=str(root/'empty.ini');(root/'empty.ini').write_text('')
 ok('AdsConnect60',str(data).encode(),1,None,None,0,C.byref(c));ok('AdsCreateTable',c,b'PRIVATE_MEMO.DBF',None,2,0,0,0,0,b'ID,C,4;BLOB,M,10',C.byref(t))
 payloads={i:(b'' if i==3 else bytes([65+i%26])*(200000 if i==4 else i*17)) for i in range(1,41)}
 for i,body in payloads.items():ok('AdsAppendRecord',t);ok('AdsSetString',t,b'BLOB',body,len(body));ok('AdsWriteRecord',t)
 ok('AdsCloseTable',t);ok('AdsDisconnect',c)
 results=[]
 for enabled in (False,True):
  log=root/('on.log' if enabled else 'off.log');cfg=root/'server.ini'
  cfg.write_text('diagnostics=false\n'+('open_phase_log='+log.as_posix()+'\n' if enabled else ''))
  with socket.socket() as s:s.bind(('127.0.0.1',0));port=s.getsockname()[1]
  out=open(root/'server.log','w');proc=subprocess.Popen([str(pathlib.Path(a.server).resolve()),'--config',str(cfg),'--host','127.0.0.1','--port',str(port),'--http_port','0','--data',str(data)],cwd=root,env=env,stdout=out,stderr=subprocess.STDOUT)
  try:
   for _ in range(200):
    try:
     with socket.create_connection(('127.0.0.1',port),timeout=.1):break
    except OSError:
     if proc.poll() is not None:raise RuntimeError((root/'server.log').read_text())
     time.sleep(.05)
   else:raise RuntimeError('server did not listen')
   ok('AdsConnect60',f'tcp://127.0.0.1:{port}/{data}'.encode(),2,None,None,0,C.byref(c));ok('AdsOpenTable',c,b'PRIVATE_MEMO.DBF',None,2,0,0,0,0,C.byref(t))
   seen=[]
   def check(i):
    n=H(99999);ok('AdsGetMemoLength',t,b'BLOB',C.byref(n));assert n.value==len(payloads[i]),(i,n.value,len(payloads[i]))
    buf=C.create_string_buffer(len(payloads[i])+1);size=H(len(buf));ok('AdsGetString',t,b'BLOB',buf,C.byref(size),0);assert buf.raw[:size.value]==payloads[i],(i,size.value)
    seen.append((i,n.value))
   ok('AdsGotoTop',t)
   for i in range(1,21):
    if i>1:ok('AdsSkip',t,1)
    check(i)
   ok('AdsGotoBottom',t)
   for i in range(40,21,-1):
    if i<40:ok('AdsSkip',t,-1)
    check(i)
   ok('AdsCloseTable',t);ok('AdsDisconnect',c);time.sleep(.1)
   if enabled:
    rows=log.read_text().splitlines();assert rows,rows
    assert all(r.startswith('OPEN80 session=') and 'fields_us=' in r and 'reply_submit_us=' in r for r in rows),rows
    assert 'PRIVATE' not in log.read_text() and str(data) not in log.read_text(),rows
   else:assert not log.exists()
   results.append(seen)
  finally:
   proc.terminate()
   try:proc.wait(timeout=10)
   except subprocess.TimeoutExpired:proc.kill();proc.wait()
   out.close()
 assert results[0]==results[1]
 print(json.dumps({'memo_positions_each':len(results[0]),'lengths_and_contents_exact':True,'empty_and_200KB':True,'OFF_no_file':True,'ON_numeric_only':True,'general_diagnostics':False,'OFF_ON_results_identical':True}))
