"""Numeric fine-open probe: disposable multi-tag/mixed-case data, raw reply parity."""
import argparse, ctypes as C, hashlib, json, os, pathlib, re, socket, struct, subprocess, tempfile, time, threading, concurrent.futures
p=argparse.ArgumentParser();p.add_argument('--client',required=True);p.add_argument('--server',required=True);p.add_argument('--bench',type=int,default=0);p.add_argument('--output');p.add_argument('--baseline-server');a=p.parse_args()
H=C.c_uint32;U=C.c_uint16;S=C.c_char_p;P=C.POINTER(H);lib=C.CDLL(str(pathlib.Path(a.client).resolve()))
specs={'AdsConnect60':[S,U,S,S,H,P],'AdsCreateTable':[H,S,S,U,U,U,U,U,S,P],'AdsCloseTable':[H],'AdsDisconnect':[H],'AdsAppendRecord':[H],'AdsSetString':[H,S,S,H],'AdsWriteRecord':[H],'AdsCreateIndex61':[H,S,S,S,S,S,H,H,P]}
for n,types in specs.items():f=getattr(lib,n);f.argtypes=types;f.restype=H
def ok(n,*args):
 rc=getattr(lib,n)(*args);assert rc==0,(n,rc)
def readn(s,n):
 b=b''
 while len(b)<n:
  t=s.recv(n-len(b));assert t,'EOF';b+=t
 return b
def request(s,op,b=b''):
 s.sendall(struct.pack('>IB',len(b),op)+b);n,ack=struct.unpack('>IB',readn(s,5));return ack,readn(s,n)
def connect(port,data):
 s=socket.create_connection(('127.0.0.1',port),timeout=10);s.setsockopt(socket.IPPROTO_TCP,socket.TCP_NODELAY,1)
 b=str(data).encode();ack,_=request(s,0x10,struct.pack('<H',len(b))+b+b'\0'*4+struct.pack('<I',0x209));assert ack==0x11;return s
def openclose(s,name,mode=1):
 ack,b=request(s,0x20,struct.pack('<H',mode)+name.encode());assert ack==0x21,(ack,b)
 tid=b[:4];assert request(s,0x22,tid)[0]==0x23
 return b
with tempfile.TemporaryDirectory(prefix='fine80_') as tmp:
 root=pathlib.Path(tmp);data=root/'data';data.mkdir();c=H();t=H();ix=H()
 (root/'empty.ini').write_text('');os.environ['OPENADS_INI']=str(root/'empty.ini')
 ok('AdsConnect60',str(data).encode(),1,None,None,0,C.byref(c))
 for name,bag,nonempty in [(b'Probe.DBF',b'Probe.Z01',True),(b'Empty.dbf',b'Empty.cdx',False),(b'Plain.dbf',None,False)]:
  ok('AdsCreateTable',c,name,None,2,0,0,0,0,b'K,C,8;V,C,8',C.byref(t))
  if nonempty:
   for key in (b'c',b'a',b'b'):
    ok('AdsAppendRecord',t);ok('AdsSetString',t,b'K',key,1);ok('AdsWriteRecord',t)
  if bag:
   for tag,expr in [(b'KEY',b'K'),(b'VALUE',b'V')]:ok('AdsCreateIndex61',t,bag,tag,expr,None,None,2,0,C.byref(ix))
  ok('AdsCloseTable',t)
 ok('AdsDisconnect',c)
 for i in range(25):(data/f'neighbour{i:02}.txt').write_text('synthetic')
 # Invalid index opens may fail or fall back; ON/OFF must do the same.
 (data/'Broken.dbf').write_bytes((data/'Plain.dbf').read_bytes());(data/'Broken.cdx').write_bytes(b'not-a-valid-index')
 results=[];summaries=[]
 for server,enabled in ([(a.baseline_server,False)] if a.baseline_server else [])+[(a.server,False),(a.server,True)]:
  cfg=root/'server.ini';log=root/('on.log' if enabled else 'off.log');cfg.write_text('diagnostics=false\n'+('open_phase_log='+log.as_posix()+'\n' if enabled else ''))
  with socket.socket() as s:s.bind(('127.0.0.1',0));port=s.getsockname()[1]
  env=os.environ.copy();env['OPENADS_ERROR_LOG_PATH']=str(root/'errors')
  out=open(root/'daemon.log','w');proc=subprocess.Popen([str(pathlib.Path(server).resolve()),'--config',str(cfg),'--host','127.0.0.1','--port',str(port),'--http_port','0','--data',str(data)],cwd=root,env=env,stdout=out,stderr=subprocess.STDOUT)
  try:
   for _ in range(200):
    try:
     with socket.create_connection(('127.0.0.1',port),timeout=.1):break
    except OSError:
     if proc.poll() is not None:raise RuntimeError((root/'daemon.log').read_text())
     time.sleep(.05)
   else:raise RuntimeError('server did not listen')
   replies=[]
   for name in ('Probe.DBF','Empty.dbf','Plain.dbf','Broken.dbf','Missing.dbf'):
    for mode in (1,0,2):
     with connect(port,data) as s:
      ack,b=request(s,0x20,struct.pack('<H',mode)+name.encode());replies.append((ack,b.hex()))
      if ack==0x21:assert request(s,0x22,b[:4])[0]==0x23
   # Explicit OpenIndex outside an OpenTable must not emit another phase row.
   with connect(port,data) as s:
    ack,b=request(s,0x20,struct.pack('<H',1)+b'Probe.DBF');assert ack==0x21
    assert request(s,0x88,b[:4]+b'Probe.Z01')[0]==0x89
    assert request(s,0x22,b[:4])[0]==0x23
   bench_us=None
   if a.bench:
    with connect(port,data) as s:
     for _ in range(100):openclose(s,'Probe.DBF')
     before=time.perf_counter_ns()
     for _ in range(a.bench):openclose(s,'Probe.DBF')
     bench_us=(time.perf_counter_ns()-before)/1000/a.bench
   time.sleep(.1);results.append(replies)
   if enabled:
    text=log.read_text();rows=text.splitlines();assert len(rows)==16+(a.bench+100 if a.bench else 0),len(rows)
    assert all(re.fullmatch(r'OPEN80(?: [a-z0-9_]+=\d+)+',x) and len(x)<4096 for x in rows)
    assert 'Probe' not in text and str(data) not in text
    parsed=[dict((k,int(v)) for k,v in re.findall(r'(\w+)=(\d+)',x)) for x in rows]
    assert parsed[0]['auto_tag_open_n']==2 and parsed[0]['refresh_tag_open_n']==2,parsed[0]
    assert parsed[0]['auto_tag_list_n']==1 and parsed[0]['refresh_tag_list_n']==1
    assert parsed[0]['auto_dir_pass_n']>0 and parsed[0]['auto_dir_entry_n']>0
    assert parsed[0]['auto_heal_n']==2 and parsed[0]['refresh_heal_n']==2
    assert parsed[0]['auto_reindex_n']==0 and parsed[0]['refresh_reindex_n']==0
    assert parsed[6]['drivers']==1 and parsed[6]['auto_tag_open_n']==0
    if a.output:pathlib.Path(a.output).write_text(text)
   else:assert not log.exists()
   # Real socket sessions plus repeated atomic replacement of identical DBF
   # bytes. Existing Windows file-sharing may refuse replacement while open;
   # that is counted, not overridden. No production/customer data is used.
   payload=(data/'Plain.dbf').read_bytes();race=data/'Race.DBF';race.write_bytes(payload)
   stop=threading.Event();replaced=[0];blocked=[0]
   def replace_file():
    for iteration in range(200):
     candidate=data/'replacement.tmp';candidate.write_bytes(payload)
     try:os.replace(candidate,race);replaced[0]+=1
     except PermissionError:blocked[0]+=1
    stop.set()
   def session_reads(_):
    values=[]
    with connect(port,data) as session:
     for iteration in range(100):
      ack,b=request(session,0x20,struct.pack('<H',1)+b'race.dbf')
      values.append((ack,b[4:] if ack==0x21 else b))
      if ack==0x21:assert request(session,0x22,b[:4])[0]==0x23
    return values
   writer=threading.Thread(target=replace_file);writer.start()
   with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:session_results=list(pool.map(session_reads,range(8)))
   writer.join();assert proc.poll() is None,'server exited during replacement'
   # Same-byte atomic replacement on POSIX, refused replacements on Windows:
   # every open must succeed and return identical table metadata after IDs.
   flattened=[v for row in session_results for v in row]
   assert len(flattened)==800 and all(v==flattened[0] and v[0]==0x21 for v in flattened),set((v[0],len(v[1])) for v in flattened)
   summaries.append({'enabled':enabled,'per_open_close_us':bench_us,'concurrent_sessions':8,'replacement_opens':800,'replacements':replaced[0],'replacement_sharing_denials':blocked[0]})
  finally:
   proc.terminate()
   try:proc.wait(timeout=10)
   except subprocess.TimeoutExpired:proc.kill();proc.wait()
   out.close()
 assert all(x==results[0] for x in results),'raw reply bytes changed old/new or OFF/ON'
 print(json.dumps({'raw_reply_parity_cases':len(results[0]),'baseline80_compared':bool(a.baseline_server),'multi_tag_auto_refresh_split':True,'mixed_case_z01_scans':True,'empty_no_bag_missing_invalid':True,'read_shared_exclusive':True,'numeric_only_bounded':True,'general_diagnostics':False,'OFF_no_file':True,'bench':summaries}))
