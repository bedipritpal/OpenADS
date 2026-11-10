"""Run the actual linked Harbour executable with both trace flavors and gates."""
import os,pathlib,subprocess,sys,tempfile
exe=str(pathlib.Path(sys.argv[1]).resolve())
for compact in ('0','1'):
 for mode in ('on','off'):
  with tempfile.TemporaryDirectory(prefix='oads_hb_mark_') as base:
   root=pathlib.Path(base);env=os.environ.copy()
   for k in list(env):
    if k.startswith('OPENADS_') and any(x in k for x in ('TRACE','LOG','DIAG','INI','OPDUMP')):env.pop(k)
   (root/'openads.ini').write_text('')
   env.update(OPENADS_INI=str(root/'openads.ini'),OPENADS_WIRE_TRACE=str(int(mode=='on')),OPENADS_WIRE_TRACE_COMPACT=compact,OPENADS_WIRE_TRACE_FILE=str(root/'wire.log'))
   p=subprocess.run([exe,mode],env=env,capture_output=True,text=True,timeout=30)
   assert p.returncode==0 and 'HARBOUR-TRACE-MARKER-PASS' in p.stdout,(compact,mode,p.returncode,p.stdout,p.stderr)
   path=root/'wire.log'
   if mode=='on':
    rows=path.read_text().splitlines();marks=[x for x in rows if '  M  ' in x]
    assert len(marks)==3 and any('L'*120 in x for x in marks),rows
    assert 'label.HB_SAVE_BEGIN' in marks[0] and 'label.HB_SAVE_END' in marks[-1]
    assert not any('wire  op.' in x for x in rows)
   else:assert not path.exists()
   print('Harbour marker',compact,mode,'PASS')
