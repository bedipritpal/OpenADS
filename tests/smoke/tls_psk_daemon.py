"""Local-only daemon policy fixtures. Keys are deterministic public test data."""
import os,pathlib,subprocess,sys,tempfile,socket,time
server=sys.argv[1]
with tempfile.TemporaryDirectory() as d:
    p=pathlib.Path(d)/'keys.txt'
    p.write_text('unit:'+('01'*32)+'\n')
    if os.name=='nt':
        owner=subprocess.check_output(['whoami'],text=True).strip()
        subprocess.run(['icacls',str(p),'/inheritance:r','/grant:r',owner+':F'],check=True,capture_output=True)
    else:p.chmod(0o600)
    def rejected(*flags):
        r=subprocess.run([server,'--data',d,*flags],capture_output=True,text=True,timeout=5)
        assert r.returncode!=0,flags
        assert '01010101' not in r.stdout+r.stderr
    rejected('--tls_mode','optional','--tls_psk_file',str(p))
    rejected('--tls_mode','off','--tls_psk_file',str(p))
    rejected('--tls_mode','required','--plaintext_port','45670','--tls_psk_file',str(p))
    rejected('--tls_mode','required','--tls_psk_file',str(p),'--tls_cert','none','--tls_key','none')
    p.write_text('unit:badhex\n');rejected('--tls_mode','required','--tls_psk_file',str(p))
    p.write_text('unit:'+('01'*32)+'\nunit:'+('02'*32)+'\n');rejected('--tls_mode','required','--tls_psk_file',str(p))
    p.write_text('unit:'+('01'*32)+'\n')
    if os.name=='nt':
        subprocess.run(['icacls',str(p),'/grant','*S-1-1-0:R'],check=True,capture_output=True)
        rejected('--tls_mode','required','--tls_psk_file',str(p))
        subprocess.run(['icacls',str(p),'/remove:g','*S-1-1-0'],check=True,capture_output=True)
        print(subprocess.check_output(['icacls',str(p)],text=True),flush=True)
        print('fixture owner='+owner,flush=True)
    else:
        p.chmod(0o644);rejected('--tls_mode','required','--tls_psk_file',str(p));p.chmod(0o600)
        link=pathlib.Path(d)/'link';link.symlink_to(p);rejected('--tls_mode','required','--tls_psk_file',str(link))
    # Successful required and optional startup on explicitly chosen local ports.
    for mode in ['required','optional']:
        slots=[]
        for _ in range(2):
            s=socket.socket();s.bind(('127.0.0.1',0));slots.append(s.getsockname()[1]);s.close()
        flags=[server,'--host','127.0.0.1','--port',str(slots[0]),'--data',d,'--tls_mode',mode,'--tls_psk_file',str(p)]
        if mode=='optional':flags+=['--plaintext_port',str(slots[1])]
        proc=subprocess.Popen(flags,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        try:
            ready=False
            for _ in range(30):
                if proc.poll() is not None:break
                try:
                    with socket.create_connection(('127.0.0.1',slots[0]),timeout=.2):ready=True
                    break
                except OSError:time.sleep(.1)
            assert ready,(mode,proc.communicate() if proc.poll() is not None else 'not ready')
            if mode=='optional':
                with socket.create_connection(('127.0.0.1',slots[1]),timeout=1):pass
        finally:
            proc.terminate();proc.wait(timeout=5)
print('PSK daemon mode/key-file permissions fixtures PASS')
