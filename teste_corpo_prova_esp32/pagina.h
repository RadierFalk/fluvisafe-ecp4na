// Interface web do teste do corpo de prova (HTML, CSS e JavaScript).
// O ESP32 envia este texto quando você abre o endereço no navegador.
// Não usa nada da internet: funciona na rede própria do ESP32.
#pragma once

const char PAGINA[] = R"rawliteral(<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>FluviSafe: teste do corpo de prova</title>
<style>
:root{
  --bg:#080f0e; --panel:#0c1614; --card:#0f1f1b; --line:#1a2e28;
  --accent:#1D9E75; --accent-2:#9FE1CB; --text:#e2ede9; --text-2:#7da898; --text-3:#4a6e62;
  --warn:#FAC775; --danger:#f56565;
  --ui:"Segoe UI",Roboto,"Helvetica Neue",system-ui,sans-serif;
}
*,*::before,*::after{box-sizing:border-box;margin:0;padding:0}
html,body{background:var(--bg);color:var(--text);font-family:var(--ui);font-size:16px;line-height:1.5;-webkit-font-smoothing:antialiased}
button,input{font:inherit;color:inherit}
:focus-visible{outline:2px solid var(--accent-2);outline-offset:2px}
main{max-width:720px;margin:0 auto;padding:16px 16px 40px}

header{display:flex;flex-wrap:wrap;align-items:flex-end;justify-content:space-between;gap:8px 16px}
h1{font-size:22px;font-weight:600;letter-spacing:-.3px;color:var(--accent-2)}
header p{font-size:14px;color:var(--text-2)}
.status{display:flex;align-items:center;gap:8px;font-size:14px;padding:5px 12px;border:1px solid var(--line);border-radius:999px;background:var(--panel)}
.dot{width:9px;height:9px;border-radius:50%;background:var(--text-3)}
.status[data-n="ok"] .dot{background:var(--accent);box-shadow:0 0 6px var(--accent)}
.status[data-n="alerta"] .dot{background:var(--warn)}
.status[data-n="erro"] .dot,.status[data-n="offline"] .dot{background:var(--danger)}
.aviso{margin-top:12px;padding:10px 12px;border-left:3px solid var(--danger);background:var(--panel);font-size:15px}
.aviso[data-n="alerta"]{border-color:var(--warn)}

.cena{margin-top:14px;background:var(--panel);border:1px solid var(--line);border-radius:14px;overflow:hidden}
.cena svg{display:block;width:100%;height:auto}
#copo{transition:transform .8s cubic-bezier(.3,.7,.3,1)}
.cota{font-variant-numeric:tabular-nums}

.passos{list-style:none;margin-top:16px;display:flex;flex-direction:column;gap:10px}
.passo{background:var(--card);border:1px solid var(--line);border-radius:12px}
.passo.atual{border-color:var(--accent-dim,#17785a)}
.cab{width:100%;display:flex;align-items:center;gap:12px;background:none;border:0;padding:12px 14px;text-align:left;cursor:pointer}
.num{width:28px;height:28px;border-radius:50%;display:grid;place-items:center;font-size:14px;font-weight:600;border:1.5px solid var(--text-3);color:var(--text-2);flex:none}
.passo.feito .num{background:var(--accent);border-color:var(--accent);color:var(--bg)}
.passo.atual .num{border-color:var(--accent-2);color:var(--accent-2)}
.tit{font-size:17px;font-weight:600;flex:1}
.est{font-size:13px;color:var(--text-2)}
.corpo{display:none;padding:0 14px 16px 54px}
.passo.aberto .corpo{display:flex;flex-direction:column;gap:12px}
.corpo p{color:var(--text-2);font-size:15px;max-width:60ch}
.btn{align-self:flex-start;background:var(--accent);color:var(--bg);border:0;border-radius:8px;padding:10px 20px;font-size:16px;font-weight:600;cursor:pointer}
.btn:disabled{opacity:.5;cursor:wait}
.btn.sec{background:transparent;color:var(--text-2);border:1px solid var(--line);font-weight:500;padding:8px 14px;font-size:15px}
.msg{font-size:14px;min-height:1.2em}
.msg[data-n="ok"]{color:var(--accent-2)} .msg[data-n="erro"]{color:var(--danger)}
.sinal{background:var(--panel);border-radius:8px;padding:8px 10px}
.sinal svg{display:block;width:100%;height:56px}
.sinal small{font-size:13px;color:var(--text-2)}
.modo{display:flex;flex-direction:column;gap:6px;font-size:15px}
.modo label{display:flex;align-items:center;gap:8px;cursor:pointer}
.modo input{accent-color:var(--accent);width:18px;height:18px}
.stepper{display:flex;align-items:center;gap:8px}
.stepper button{width:44px;height:44px;border-radius:8px;border:1px solid var(--line);background:var(--panel);font-size:22px;cursor:pointer}
.stepper input{width:90px;height:44px;text-align:center;font-size:22px;font-weight:600;background:var(--panel);border:1px solid var(--line);border-radius:8px;font-variant-numeric:tabular-nums}
.stepper span{color:var(--text-2)}
.conta{font-size:15px;color:var(--text)}
.grafico svg{display:block;width:100%;height:auto;background:var(--panel);border-radius:8px}
.massa{display:flex;flex-wrap:wrap;align-items:center;gap:8px;font-size:15px;color:var(--text-2)}
.massa input{width:80px;background:var(--panel);border:1px solid var(--line);border-radius:6px;padding:6px 8px}
.massa b{color:var(--text);font-weight:600}
.massa b[data-n="ok"]{color:var(--accent-2)} .massa b[data-n="alerta"]{color:var(--warn)}
.acoes{display:flex;flex-wrap:wrap;gap:8px}

details{margin-top:16px;font-size:14px;color:var(--text-2)}
summary{cursor:pointer;padding:6px 0}
dl{display:grid;grid-template-columns:auto 1fr;gap:4px 16px;margin-top:6px}
dd{text-align:right;color:var(--text);font-variant-numeric:tabular-nums}

@media (max-width:480px){.corpo{padding-left:14px}h1{font-size:20px}}
@media (prefers-reduced-motion:reduce){#copo{transition:none}}
</style>
</head>
<body>
<main>
  <header>
    <div>
      <h1>Teste do corpo de prova</h1>
      <p>Quanto o barquinho afundou, medido pelo sensor de pressão.</p>
    </div>
    <div class="status" id="status" data-n="aguardando" role="status"><span class="dot"></span><span id="statusTxt">Conectando ao ESP32…</span></div>
  </header>
  <p class="aviso" id="aviso" hidden></p>

  <figure class="cena">
    <svg viewBox="0 0 360 250" role="img" aria-label="Corpo de prova flutuando na vasilha, afundando conforme o calado medido">
      <defs>
        <linearGradient id="agua" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0" stop-color="#1D9E75" stop-opacity=".55"/>
          <stop offset="1" stop-color="#085041" stop-opacity=".95"/>
        </linearGradient>
        <linearGradient id="tubo" x1="0" y1="1" x2="0" y2="0">
          <stop offset="0" stop-color="#e2ede9" stop-opacity=".9"/>
          <stop offset="1" stop-color="#e2ede9" stop-opacity="0"/>
        </linearGradient>
        <clipPath id="vasilha"><path d="M18 50 Q18 236 180 240 Q342 236 342 50 Z"/></clipPath>
      </defs>
      <g clip-path="url(#vasilha)"><rect x="0" y="120" width="360" height="130" fill="url(#agua)"/></g>
      <path d="M18 50 Q18 236 180 240 Q342 236 342 50" fill="none" stroke="#4a6e62" stroke-width="3"/>
      <line x1="22" y1="120" x2="338" y2="120" stroke="#9FE1CB" stroke-width="1.2" opacity=".7"/>

      <g id="copo" transform="translate(0,104)">
        <path d="M143 -84 C 143 -130 230 -120 260 -200" fill="none" stroke="url(#tubo)" stroke-width="4" stroke-linecap="round"/>
        <path d="M115 -96 L115 0 L136 0 L136 -7 L121 -7 L121 -96 Z M245 -96 L245 0 L150 0 L150 -7 L239 -7 L239 -96 Z" fill="#c9d6d1"/>
        <rect x="121" y="-96" width="118" height="89" fill="#0f1f1b" opacity=".6"/>
        <path d="M128 -7 L128 -31 L158 -31 L158 -7 Z M131 -31 L139 -55 L147 -55 L155 -31 Z" fill="#FAC775"/>
        <rect x="140" y="-84" width="6" height="30" fill="#FAC775"/>
        <text x="200" y="-40" text-anchor="middle" font-size="12" fill="#9FE1CB">corpo de prova</text>
      </g>

      <g id="cota" visibility="hidden">
        <line id="cotaL" x1="262" x2="262" y1="120" stroke="#e2ede9" stroke-width="1.5"/>
        <path id="cotaA" fill="#e2ede9"/>
        <text id="cotaT" class="cota" x="272" font-size="34" font-weight="700" fill="#e2ede9"></text>
        <text id="cotaU" x="272" font-size="13" fill="#7da898">mm de calado</text>
      </g>
      <text id="semCal" x="180" y="34" text-anchor="middle" font-size="15" fill="#7da898">Siga os passos abaixo para medir</text>
    </svg>
  </figure>

  <ol class="passos">
    <li class="passo" id="p1">
      <button class="cab" data-p="1" aria-expanded="false"><span class="num">1</span><span class="tit">Zerar fora da água</span><span class="est" id="e1"></span></button>
      <div class="corpo">
        <p>Tire o corpo de prova da água e deixe o furo do fundo livre.</p>
        <div class="sinal"><svg id="spark" viewBox="0 0 300 56" preserveAspectRatio="none"></svg>
          <small>Aperte o tubo de leve: esta linha deve se mexer. Se ficar parada, o sensor não está sentindo a pressão.</small></div>
        <button class="btn" id="bZerar">Zerar</button>
        <p class="msg" id="m1"></p>
      </div>
    </li>
    <li class="passo" id="p2">
      <button class="cab" data-p="2" aria-expanded="false"><span class="num">2</span><span class="tit">Calibrar na água</span><span class="est" id="e2"></span></button>
      <div class="corpo">
        <p>Coloque na água e espere parar de balançar. Meça com a régua, no lado do sino:</p>
        <div class="modo">
          <label><input type="radio" name="modo" value="borda" checked> a parte que ficou fora da água (borda)</label>
          <label><input type="radio" name="modo" value="prof"> a profundidade da boca do sino (sino solto)</label>
        </div>
        <div class="stepper">
          <button type="button" id="menos" aria-label="Diminuir 1 mm">−</button>
          <input type="number" id="valor" value="14" min="0" max="60" step="0.5" inputmode="decimal" aria-label="Medida em milímetros">
          <button type="button" id="mais" aria-label="Aumentar 1 mm">+</button>
          <span>mm</span>
        </div>
        <p class="conta" id="conta"></p>
        <button class="btn" id="bCal">Calibrar</button>
        <p class="msg" id="m2"></p>
      </div>
    </li>
    <li class="passo" id="p3">
      <button class="cab" data-p="3" aria-expanded="false"><span class="num">3</span><span class="tit">Testar com moedas</span><span class="est" id="e3"></span></button>
      <div class="corpo">
        <p>Coloque moedas dentro e veja o barquinho e o número descerem. Peso num lado só inclina o barco: a leitura sobe se o peso estiver do lado do sino.</p>
        <div class="grafico"><svg id="graf" viewBox="0 0 320 140"></svg></div>
        <div class="massa">Pesou o conjunto? <input type="number" id="massa" min="0" max="400" placeholder="g"> g
          <span>Esperado <b id="esp">–</b>, diferença <b id="dif">–</b></span></div>
        <div class="acoes"><button class="btn sec" id="bRefazer">Calibrar de novo</button><button class="btn sec" id="bApagar">Apagar calibração</button></div>
        <p class="msg" id="m3"></p>
      </div>
    </li>
  </ol>

  <details>
    <summary>Detalhes do sensor</summary>
    <dl>
      <dt>Leitura bruta</dt><dd id="dBruto">–</dd>
      <dt>Leituras válidas</dt><dd id="dVal">–</dd>
      <dt>Ruído com o tubo parado</dt><dd id="dRuido">–</dd>
      <dt>Fator de calibração</dt><dd id="dFator">–</dd>
      <dt>Endereço desta página</dt><dd id="dEnd">–</dd>
    </dl>
  </details>
</main>

<script>
const $ = id => document.getElementById(id);
const NS = 'http://www.w3.org/2000/svg';
const SUP = 120, PX = 2.4, ALTURA = 40;          // superfície da água, pixels por mm, altura do corpo de prova
const fmt = (v, d) => v.toLocaleString('pt-BR', {minimumFractionDigits: d, maximumFractionDigits: d});
let estado = null, abertoManual = null, ultimaAcao = null, histSinal = [], histMm = [];

function passoAtual(e) { return !e.zerado ? 1 : !e.calibrado ? 2 : 3; }

function abrir(n) {
  [1, 2, 3].forEach(i => {
    const li = $('p' + i), aberto = i === n;
    li.classList.toggle('aberto', aberto);
    li.querySelector('.cab').setAttribute('aria-expanded', aberto);
  });
}
document.querySelectorAll('.cab').forEach(b => b.onclick = () => { abertoManual = +b.dataset.p; abrir(abertoManual); });

function caladoInformado() {
  const v = parseFloat($('valor').value);
  if (isNaN(v)) return null;
  return document.querySelector('input[name=modo]:checked').value === 'borda' ? ALTURA - v : v;
}
function atualizarConta() {
  const c = caladoInformado(), borda = document.querySelector('input[name=modo]:checked').value === 'borda';
  $('conta').textContent = c === null ? '' : borda ? `Calado = ${ALTURA} − ${fmt(ALTURA - c, 1)} = ${fmt(c, 1)} mm` : `Calado = ${fmt(c, 1)} mm`;
  $('bCal').textContent = c === null ? 'Calibrar' : `Calibrar com ${fmt(c, 1)} mm`;
}
$('menos').onclick = () => { $('valor').value = Math.max(0, (parseFloat($('valor').value) || 0) - 1); atualizarConta(); };
$('mais').onclick = () => { $('valor').value = (parseFloat($('valor').value) || 0) + 1; atualizarConta(); };
$('valor').oninput = atualizarConta;
document.querySelectorAll('input[name=modo]').forEach(r => r.onchange = atualizarConta);
atualizarConta();

function linha(svg, pts, W, H, cor, lo, hi, extra) {
  svg.innerHTML = '';
  const el = (n, a) => { const x = document.createElementNS(NS, n); for (const k in a) x.setAttribute(k, a[k]); svg.appendChild(x); return x; };
  if (extra) extra(el, v => H - 6 - (v - lo) / (hi - lo) * (H - 12));
  if (pts.length < 2) return;
  const t0 = pts[pts.length - 1].t, janela = pts[pts.length - 1].t - pts[0].t || 1;
  el('polyline', {points: pts.map(p => ((1 - (t0 - p.t) / janela) * W).toFixed(1) + ',' + (H - 6 - (p.v - lo) / (hi - lo) * (H - 12)).toFixed(1)).join(' '),
    fill: 'none', stroke: cor, 'stroke-width': 2.5, 'stroke-linejoin': 'round', 'vector-effect': 'non-scaling-stroke'});
}

function esperado() { const m = parseFloat($('massa').value); return m > 0 ? m / 4.8 : null; }   // fundo de 8 × 6 cm: 1 g afunda ~0,21 mm

function desenharCena(e) {
  const mm = e && e.calibrado ? e.mm : null, dentro = mm !== null && mm > 0.5;
  const fundo = dentro ? SUP + Math.min(mm, ALTURA) * PX : SUP - 16;
  $('copo').setAttribute('transform', `translate(0,${fundo})`);
  $('semCal').style.display = e && e.calibrado ? 'none' : '';
  $('cota').setAttribute('visibility', dentro ? 'visible' : 'hidden');
  if (dentro) {
    $('cotaL').setAttribute('y2', fundo);
    $('cotaA').setAttribute('d', `M257 ${SUP + 7} L262 ${SUP} L267 ${SUP + 7} Z M257 ${fundo - 7} L262 ${fundo} L267 ${fundo - 7} Z`);
    const meio = Math.max(SUP + 34, (SUP + fundo) / 2 + 8);
    $('cotaT').setAttribute('y', meio); $('cotaU').setAttribute('y', meio + 18);
    $('cotaT').textContent = fmt(mm, 1);
  } else if (e && e.calibrado) {
    $('semCal').style.display = ''; $('semCal').textContent = 'Fora da água';
  }
}

function painel(e) {
  const d = e.diag, rotulo = {ok: 'Sensor respondendo', alerta: 'Sensor com falhas', erro: 'Sensor com problema', aguardando: 'Iniciando…'};
  $('status').dataset.n = d.nivel; $('statusTxt').textContent = rotulo[d.nivel];
  $('aviso').hidden = !(d.nivel === 'erro' || d.nivel === 'alerta');
  $('aviso').dataset.n = d.nivel; $('aviso').textContent = d.texto;
  $('semCal').textContent = 'Siga os passos abaixo para medir';

  const atual = passoAtual(e);
  [1, 2, 3].forEach(i => { $('p' + i).classList.toggle('feito', i < atual); $('p' + i).classList.toggle('atual', i === atual); });
  $('e1').textContent = e.zerado ? 'feito' : ''; $('e2').textContent = e.calibrado ? 'feito' : '';
  $('e3').textContent = e.calibrado && e.mm !== null ? fmt(e.mm, 1) + ' mm' : '';
  abrir(abertoManual || atual);

  const ocupado = e.ocupado;
  ['bZerar', 'bCal'].forEach(id => $(id).disabled = ocupado);
  if (ocupado && ultimaAcao) { $(ultimaAcao.msg).textContent = ultimaAcao.espera; $(ultimaAcao.msg).dataset.n = ''; }
  if (!ocupado && ultimaAcao && e.mensagem) {
    $(ultimaAcao.msg).textContent = e.mensagem; $(ultimaAcao.msg).dataset.n = e.tipo;
    if (e.tipo === 'ok') abertoManual = ultimaAcao.msg === 'm1' ? 2 : null;   // depois de zerar, abre a calibração
    ultimaAcao = null;
  }

  const esp = esperado();
  $('esp').textContent = esp ? fmt(esp, 1) + ' mm' : '–';
  if (esp && e.calibrado && e.mm !== null && e.mm > 0.5) {
    const dif = e.mm - esp; $('dif').textContent = (Math.abs(dif) < 0.05 ? '' : dif > 0 ? '+' : '−') + fmt(Math.abs(dif), 1) + ' mm';
    $('dif').dataset.n = Math.abs(dif) <= 3 ? 'ok' : 'alerta';
  } else { $('dif').textContent = '–'; $('dif').dataset.n = ''; }

  $('dBruto').textContent = e.bruto === null ? '–' : e.bruto.toLocaleString('pt-BR');
  $('dVal').textContent = fmt(d.validas, 0) + '%';
  $('dRuido').textContent = d.ruido_mm !== null ? fmt(d.ruido_mm, 2) + ' mm' : d.ruido !== null ? d.ruido.toLocaleString('pt-BR') + ' contagens' : '–';
  $('dFator').textContent = e.fator === null ? '–' : fmt(e.fator, 1) + ' por mm';
  $('dEnd').textContent = e.endereco;
}

function graficos(e) {
  const t = Date.now() / 1000;
  if (e.filtrado !== null) histSinal.push({t, v: e.filtrado});
  histSinal = histSinal.filter(p => t - p.t <= 20);
  let lo = Math.min(...histSinal.map(p => p.v)), hi = Math.max(...histSinal.map(p => p.v));
  if (!isFinite(lo)) { lo = 0; hi = 1; }
  const pad = Math.max(1500, (hi - lo) * 0.3); lo -= pad; hi += pad;
  linha($('spark'), histSinal, 300, 56, '#9FE1CB', lo, hi);

  if (e.calibrado && e.mm !== null) histMm.push({t, v: e.mm}); else histMm = [];
  histMm = histMm.filter(p => t - p.t <= 60);
  let top = Math.max(ALTURA, ...histMm.map(p => p.v)), bot = Math.min(0, ...histMm.map(p => p.v));
  linha($('graf'), histMm, 320, 140, '#1D9E75', bot, top, (el, py) => {
    [0, 10, 20, 30, 40].forEach(v => { if (v > top) return; const y = py(v);
      el('line', {x1: 26, x2: 320, y1: y, y2: y, stroke: '#1a2e28'});
      el('text', {x: 2, y: y + 4, 'font-size': 10, fill: '#7da898'}).textContent = v; });
    const esp = esperado(); if (esp) el('line', {x1: 26, x2: 320, y1: py(esp), y2: py(esp), stroke: '#FAC775', 'stroke-dasharray': '5 4'});
  });
}

async function atualizar() {
  try {
    const r = await fetch('/api/estado', {cache: 'no-store'});
    estado = await r.json();
    painel(estado); desenharCena(estado); graficos(estado);
  } catch (err) {
    $('status').dataset.n = 'offline'; $('statusTxt').textContent = 'Sem conexão com o ESP32';
    $('aviso').hidden = false; $('aviso').dataset.n = 'erro';
    $('aviso').textContent = 'A página não está recebendo dados. Confira se o celular continua na rede FluviSafe-Teste e se o ESP32 está ligado.';
  }
}

async function acao(url, msg, espera) {
  ultimaAcao = {msg, espera};
  $(msg).textContent = espera; $(msg).dataset.n = '';
  try {
    const r = await fetch(url, {method: 'POST'});
    if (!r.ok) { const j = await r.json(); throw new Error(j.erro || 'Erro ' + r.status); }
  } catch (err) { $(msg).textContent = err.message; $(msg).dataset.n = 'erro'; ultimaAcao = null; }
  atualizar();
}
$('bZerar').onclick = () => acao('/api/zerar', 'm1', 'Zerando… mantenha fora da água por 2 segundos.');
$('bCal').onclick = () => {
  const c = caladoInformado();
  if (c === null || c < 3) { $('m2').textContent = 'Informe a medida da régua. O calado precisa ser de pelo menos 3 mm.'; $('m2').dataset.n = 'erro'; return; }
  acao('/api/calibrar?mm=' + c.toFixed(1), 'm2', 'Calibrando… não mexa na vasilha por 2 segundos.');
};
$('bRefazer').onclick = () => { abertoManual = 1; abrir(1); };
$('bApagar').onclick = () => { abertoManual = null; acao('/api/apagar', 'm3', 'Apagando…'); };
$('massa').oninput = () => estado && (painel(estado), graficos(estado));

atualizar();
setInterval(atualizar, 500);
</script>
</body>
</html>
)rawliteral";
