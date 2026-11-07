#pragma once

// Pagina web do gerenciador de arquivos do Orbita (servida pelo LanApp).
// Fica na flash do programa; o navegador pede usuario "orbita" e o PIN da tela.
static const char WEB_PAGE[] = R"ORB(<!doctype html>
<html lang="pt-BR"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Orbita</title>
<style>
:root{--bg:#0b0d16;--panel:#181c2c;--fg:#e9ecf3;--dim:#8a91a8;--acc:#00c8ff;--ok:#46dc78;--bad:#ff5a46}
*{box-sizing:border-box}body{margin:0;font:15px system-ui,sans-serif;background:var(--bg);color:var(--fg)}
header{display:flex;align-items:center;gap:10px;padding:12px 16px;background:var(--panel)}
header b{font-size:18px}header span{color:var(--dim);font-size:13px}
main{max-width:900px;margin:0 auto;padding:12px 16px}
.tabs{display:flex;gap:6px;margin-bottom:10px}
button,.btn{background:var(--panel);color:var(--fg);border:1px solid #2a3046;border-radius:8px;padding:7px 12px;font:inherit;cursor:pointer}
button:hover{border-color:var(--acc)}button.on{background:var(--acc);color:#000;border-color:var(--acc)}
.bar{display:flex;flex-wrap:wrap;gap:6px;align-items:center;margin:8px 0}
.crumb a{color:var(--acc);text-decoration:none;cursor:pointer}.crumb{flex:1;min-width:200px;word-break:break-all}
table{width:100%;border-collapse:collapse}td{padding:8px 6px;border-bottom:1px solid #20263a}
td.n{word-break:break-all}td.n a{color:var(--fg);text-decoration:none;cursor:pointer}
td.s{color:var(--dim);white-space:nowrap;text-align:right}td.a{white-space:nowrap;text-align:right}
td.a button{padding:4px 8px;font-size:13px}
.drop{border:2px dashed #2a3046;border-radius:10px;padding:14px;text-align:center;color:var(--dim);margin:10px 0}
.drop.hot{border-color:var(--acc);color:var(--fg)}
.prog{height:6px;background:#20263a;border-radius:3px;overflow:hidden;margin-top:6px;display:none}
.prog i{display:block;height:100%;width:0;background:var(--acc)}
#msg{position:fixed;bottom:16px;left:50%;transform:translateX(-50%);background:var(--fg);color:#000;padding:8px 14px;border-radius:8px;display:none}
.dim{color:var(--dim)}
</style></head><body>
<header><b>&#9711; Orbita</b><span id="ver"></span></header>
<main>
<div class="tabs" id="vols"></div>
<div class="bar"><div class="crumb" id="crumb"></div>
<button onclick="mk()">Nova pasta</button><button onclick="list()">Atualizar</button></div>
<label class="drop" id="drop">Solte arquivos aqui ou <u>clique para enviar</u>
<input type="file" id="file" multiple hidden><div class="prog" id="prog"><i></i></div>
<div id="up" class="dim"></div></label>
<table id="tbl"></table>
</main><div id="msg"></div>
<script>
const $=s=>document.querySelector(s);let vol='sd',path='/';
const enc=encodeURIComponent;
function fmt(b){if(b<1024)return b+' B';if(b<1048576)return(b/1024).toFixed(1)+' KB';if(b<1073741824)return(b/1048576).toFixed(2)+' MB';return(b/1073741824).toFixed(2)+' GB'}
function msg(t){const m=$('#msg');m.textContent=t;m.style.display='block';clearTimeout(m.t);m.t=setTimeout(()=>m.style.display='none',3000)}
async function api(u,o){const r=await fetch(u,o);if(!r.ok)throw new Error(await r.text());return r}
function post(u,d){return api(u,{method:'POST',body:new URLSearchParams(d)})}
function join(a,b){return a.endsWith('/')?a+b:a+'/'+b}
function esc(s){return s.replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]))}
async function info(){const j=await(await api('/api/info')).json();$('#ver').textContent='v'+j.version;
 $('#vols').innerHTML=j.volumes.map(v=>`<button class="${v.key==vol?'on':''}" ${v.ok?'':'disabled'} onclick="setVol('${v.key}')">${esc(v.name)} <span class="dim">${v.ok?fmt(v.used)+' / '+fmt(v.total):'indisponivel'}</span></button>`).join('');

function setVol(v){vol=v;path='/';info();list()}
let cur=[],crumbs=[];
function fp(i){return join(path,cur[i].n)}
function up(){go(path.substring(0,path.lastIndexOf('/'))||'/')}
async function list(){let j;try{j=await(await api(`/api/list?vol=${vol}&path=${enc(path)}`)).json()}catch(e){msg(e.message);return}
 path=j.path;const parts=path.split('/').filter(x=>x);let acc='';crumbs=['/'];parts.forEach(p=>{acc+='/'+p;crumbs.push(acc)});
 $('#crumb').innerHTML=crumbs.map((c,i)=>`<a onclick="go(crumbs[${i}])">${i?esc(parts[i-1]):vol.toUpperCase()+':/'}</a>`).join(' / ');
 cur=j.entries.sort((a,b)=>(b.d-a.d)||a.n.localeCompare(b.n));
 let rows=cur.map((e,i)=>{const full=join(path,e.n);
  return `<tr><td class="n">${e.d?'&#128193; ':''}<a ${e.d?`onclick="go(fp(${i}))"`:`href="/api/download?vol=${vol}&path=${enc(full)}"`}>${esc(e.n)}</a></td><td class="s">${e.d?'':fmt(e.s)}</td><td class="a"><button onclick="ren(fp(${i}))">Renomear</button> <button onclick="del(fp(${i}),${e.d})">Apagar</button></td></tr>`});
 if(path!='/')rows.unshift('<tr><td class="n"><a onclick="up()">..</a></td><td></td><td></td></tr>');
 $('#tbl').innerHTML=rows.join('')||'<tr><td class="dim">Pasta vazia</td></tr>'}
function go(p){path=p;list()}
async function mk(){const n=prompt('Nome da nova pasta');if(!n)return;try{await post('/api/mkdir',{vol,path:join(path,n)});list()}catch(e){msg(e.message)}}
async function ren(f){const old=f.split('/').pop();const n=prompt('Novo nome',old);if(!n||n==old)return;try{await post('/api/rename',{vol,from:f,to:join(path,n)});list()}catch(e){msg(e.message)}}
async function del(f,d){if(!confirm('Apagar '+f+(d?' e tudo dentro?':'?')))return;try{await post('/api/delete',{vol,path:f});list();info()}catch(e){msg(e.message)}}
function upload(files){files=[...files];if(!files.length)return;const bar=$('#prog'),fill=bar.firstElementChild;
 let i=0;const next=()=>{if(i>=files.length){bar.style.display='none';$('#up').textContent='Enviado!';list();info();return}
  const f=files[i++];const fd=new FormData();fd.append('file',f,f.name);const x=new XMLHttpRequest();
  x.open('POST',`/api/upload?vol=${vol}&dir=${enc(path)}`);bar.style.display='block';
  x.upload.onprogress=e=>{if(e.lengthComputable){fill.style.width=(e.loaded*100/e.total)+'%';$('#up').textContent=`${f.name}: ${fmt(e.loaded)} de ${fmt(e.total)}`}};
  x.onload=()=>{if(x.status!=200){msg(x.responseText||'Falhou');bar.style.display='none';list();return}next()};
  x.onerror=()=>{msg('Erro de rede');bar.style.display='none'};x.send(fd)};next()}
$('#file').onchange=e=>upload(e.target.files);
const d=$('#drop');d.ondragover=e=>{e.preventDefault();d.classList.add('hot')};d.ondragleave=()=>d.classList.remove('hot');
d.ondrop=e=>{e.preventDefault();d.classList.remove('hot');upload(e.dataTransfer.files)};
info().then(list).catch(e=>msg(e.message));
</script></body></html>)ORB";
