"""Generated-data process matrix; no customer source or data."""
import os, pathlib, subprocess, sys, tempfile
exe=sys.argv[1]
with tempfile.TemporaryDirectory(prefix='openads_trace_contract_') as base:
 for transport in ('env','ini','both'):
  for mode in ('default','off','on','lifecycle','late'):
   root=pathlib.Path(base)/(transport+'-'+mode);root.mkdir()
   env=os.environ.copy()
   for key in list(env):
    if key.startswith('OPENADS_') and any(x in key for x in ('TRACE','LOG','DIAG','INI','OPDUMP')):env.pop(key)
   env['OPENADS_INI']=str(root/'openads.ini')
   settings={'wire_trace':'1','wire_trace_file':str(root/'wire.log'),'log_file':str(root/'audit.log'),'log':'debug','resolve_verbose':'1','arc_trace':'1','trace':'1'}
   (root/'openads.ini').write_text('\n'.join(k+'='+v for k,v in settings.items()) if transport!='env' else '')
   if transport!='ini':
    for key,val in settings.items():env['OPENADS_'+key.upper()]=val
   # Lifecycle probe gets path independently of config lookup.
   env['OPENADS_WIRE_TRACE_FILE']=str(root/'wire.log')
   for k,name in [('CREATE_INDEX_DIAG_FILE','index.log'),('CREATE_TABLE_DIAG_FILE','table.log'),('LOCK_DIAG','lock.log'),('ERROR_LOG_PATH','errors')]:env['OPENADS_'+k]=str(root/name)
   p=subprocess.run([exe,mode],env=env,capture_output=True,text=True,timeout=30)
   if p.returncode:raise AssertionError((transport,mode,p.returncode,p.stdout,p.stderr))
   texts='\n'.join(f.read_bytes().decode('latin1') for f in root.rglob('*') if f.is_file() and f.name!='openads.ini')+p.stderr
   for canary in ('PRIVATE_TABLE_CANARY','PRIVATE_ERROR_CANARY','FIELD_X','TAGX','1234567','openads_trace_gate_probe'):
    assert canary not in texts,(transport,mode,canary)
   if mode in ('default','off'):
    assert not p.stderr,(transport,mode,p.stderr)
    assert not (root/'wire.log').exists(),(transport,mode)
    assert not list((root/'errors').glob('*')),(transport,mode)
    assert not any((root/f).exists() for f in ('index.log','table.log','audit.log','lock.log')),(transport,mode)
   else:
    wire=(root/'wire.log').read_text()
    assert 'wire op=' in wire,(transport,mode)
    assert 'SAFE_ALIAS' in wire,(transport,mode)
   print(transport,mode,'PASS',p.stdout.strip())

# Explicit master ON must not itself turn on an unselected wire channel.
for value in ('0', 'false', 'off', 'no'):
 with tempfile.TemporaryDirectory(prefix='openads_trace_false_') as base:
  root=pathlib.Path(base);env=os.environ.copy()
  env['OPENADS_INI']=str(root/'openads.ini');(root/'openads.ini').write_text('wire_trace=1\n')
  env['OPENADS_WIRE_TRACE']=value;env['OPENADS_WIRE_TRACE_FILE']=str(root/'wire.log')
  env['OPENADS_ERROR_LOG_PATH']=str(root/'errors')
  p=subprocess.run([exe,'on'],env=env,capture_output=True,text=True,timeout=30)
  assert p.returncode==0,(value,p.returncode,p.stderr)
  assert not (root/'wire.log').exists(),value
  assert '[wire]' not in p.stderr,value
  print('env_false_overrides_ini',value,'PASS')

# Standalone daemon keeps necessary operational replies, not diagnostics.
if len(sys.argv)>2:
 import time
 with tempfile.TemporaryDirectory(prefix='PRIVATE_SERVER_CANARY_') as base:
  root=pathlib.Path(base);cfg=root/'server.ini'
  cfg.write_text('host=127.0.0.1\nport=0\ndata='+base+'\ndiagnostics=false\n')
  env=os.environ.copy();env['OPENADS_WIRE_TRACE']='1';env['OPENADS_OPDUMP']='1'
  env['OPENADS_ERROR_LOG_PATH']=str(root/'errors')
  env['OPENADS_LOG_FILE']=str(root/'audit.log')
  server=subprocess.Popen([sys.argv[2],'--config',str(cfg)],env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
  try:
   time.sleep(1)
   assert server.poll() is None,'server failed startup'
   server.terminate();out,err=server.communicate(timeout=10)
  finally:
   if server.poll() is None:server.kill();server.communicate()
  assert 'PRIVATE_SERVER_CANARY' not in out+err
  assert '[wire]' not in err
  assert not list((root/'errors').glob('*'))
  assert not (root/'audit.log').exists()
  print('standalone_server_OFF_env_OPDUMP_and_wire PASS')
