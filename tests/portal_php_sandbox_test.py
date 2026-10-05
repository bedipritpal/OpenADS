"""Run with a sandbox hardened daemon on16482 and PHP server on16483.
PHP: OPENADS_DLL=<matching-library> php -d ffi.enable=1 -d session.save_path=<tmp> -S 127.0.0.1:16483 -t DA-Web
Seed session portaltest1 with a demo connection to127.0.0.1:16482.
No user credentials/server required. Fixed test ports, never production.
"""
import json, urllib.request, urllib.error, time
base='http://127.0.0.1:16483/api/server_info.php'
def req(body=None,content='application/json'):
    request=urllib.request.Request(base+'?dd=demo',data=json.dumps(body).encode() if body else None,headers={'Cookie':'PHPSESSID=portaltest1','Content-Type':content})
    try:
        with urllib.request.urlopen(request) as r:return r.status,json.load(r)
    except urllib.error.HTTPError as e:return e.code,json.load(e)
status,body=req();assert status==401 and body['management_required'];token=body['management_csrf']
login={'action':'management_login','dd':'demo','csrf':token,'management_user':'testadmin','management_password':'sandbox-only'}
assert req({**login,'csrf':'wrong'})[0]==403
assert req(login,'text/plain')[0]==415
time.sleep(1.1) # deliberate denied management probe triggers daemon backoff
status,body=req(login);assert body.get('ok'), (status,body)
status,body=req();assert status==200 and body['health']['schema_version']==1 and body['health']['scope']=='server'
assert body['health']['parked_handles'] is None
assert 'password' not in json.dumps(body)
assert req({'action':'management_logout','dd':'demo','csrf':token})[1]['ok']
assert req()[0]==401
print('PHP management login/CSRF/JSON/denial/health/forget passed')
