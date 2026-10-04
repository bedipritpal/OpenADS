// Exercise source functions without copying them into the test.
const fs=require('fs'),vm=require('vm'),assert=require('assert'),path=require('path');
const root=path.resolve(__dirname,'..');
const source=fs.readFileSync(path.join(root,'DA-Web/js/app.js'),'utf8');
const start=source.indexOf('  function serverHealthHtml('),end=source.indexOf('  function loadServerInfo(',start);
const containers={};let submit=null,loaded=0;
const form={elements:{user:{value:'test'},password:{value:'test-only'}},addEventListener:(event,fn)=>{submit=fn}};
const ctx={escHtml:s=>String(s).replaceAll('&','&amp;').replaceAll('<','&lt;').replaceAll('"','&quot;'),document:{getElementById:id=>id.startsWith('mg-login-')?form:containers[id]},loadServerInfo:()=>loaded++,apiFetch:async()=>({ok:true})};
vm.createContext(ctx);vm.runInContext(source.slice(start,end),ctx);
const html=ctx.serverHealthHtml({schema_version:1,workareas:{current:0,max_used:4,rejected:null},version:'<script>',semantics:{snapshot:'<bad>'}},null);
assert(html.includes('Current: 0'));assert(html.includes('Max used: 4'));assert(html.includes('Unavailable / not measured'));assert(!html.includes('<script>'));assert(html.includes('&lt;bad>'));
assert(ctx.serverHealthHtml(null,'<bad>').includes('&lt;bad>'));
const container={innerHTML:''};containers['mg-error-t']={textContent:''};ctx.showManagementLogin(container,'t','dd',{management_required:true,management_csrf:'token'});
assert(container.innerHTML.includes('type="password"'));
(async()=>{await submit({preventDefault:()=>{}});assert.equal(form.elements.password.value,'');assert.equal(loaded,1);console.log('portal health JS behavior passed')})().catch(e=>{console.error(e);process.exit(1)});
