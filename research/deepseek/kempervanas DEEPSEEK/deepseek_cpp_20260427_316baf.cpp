#pragma once

static const char KEMPERIS_DASHBOARD_HTML[] PROGMEM = R"KEMPERIS(<!DOCTYPE html>
<html lang="lt">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="mobile-web-app-capable" content="yes">
<title>Kempervanas</title>
<style>
:root {
  --bg: #0a0d11;
  --bg2: #0f1318;
  --card: #161b22;
  --card2: #1c2128;
  --card3: #21262d;
  --accent: #00adb5;
  --accent2: #2dd4bf;
  --text: #f0f6fc;
  --muted: #8b949e;
  --dim: #6e7681;
  --red: #f85149;
  --orange: #fb8500;
  --yellow: #f5b84e;
  --green: #3fb950;
  --green-dark: #238636;
  --blue: #58a6ff;
  --purple: #a371f7;
  --grey: #30363d;
  --grey-dark: #21262d;
}
* { box-sizing: border-box; -webkit-tap-highlight-color: transparent; }
html, body {
  margin: 0; padding: 0;
  background: var(--bg);
  color: var(--text);
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
  -webkit-user-select: none;
  user-select: none;
  overflow-x: hidden;
}
body { padding: 8px 8px 80px 8px; }

.statusbar {
  display: flex; align-items: center; gap: 10px;
  padding: 6px 10px;
  background: linear-gradient(180deg, var(--card) 0%, var(--card2) 100%);
  border-radius: 12px;
  border: 1px solid var(--grey);
  margin-bottom: 10px;
  font-size: 12px;
}
.dot {
  width: 10px; height: 10px; border-radius: 50%;
  background: var(--red);
  transition: all 0.3s;
}
.dot.ok { background: var(--green); box-shadow: 0 0 8px var(--green); }
.statusbar .spacer { flex: 1; }
.chip {
  padding: 3px 9px; border-radius: 10px;
  background: var(--card3); border: 1px solid var(--grey);
  font-size: 11px;
  display: inline-flex; align-items: center; gap: 4px;
}
.chip.green { background: rgba(63,185,80,0.15); border-color: var(--green-dark); color: var(--green); }
.chip.red   { background: rgba(248,81,73,0.15); border-color: var(--red);  color: var(--red); }
.chip.yellow{ background: rgba(245,184,78,0.15);border-color: var(--yellow); color: var(--yellow); }
.chip.dim   { color: var(--muted); }

.tabs {
  display: grid;
  grid-template-columns: repeat(6, 1fr);
  gap: 4px;
  position: sticky;
  top: 0;
  background: var(--bg);
  z-index: 100;
  padding: 4px 0 8px;
}
.tab {
  padding: 10px 2px;
  border: 1px solid var(--grey);
  background: var(--card);
  color: var(--muted);
  border-radius: 10px;
  font-weight: 600;
  font-size: 9px;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  cursor: pointer;
  transition: all 0.2s;
  text-align: center;
}
.tab.active {
  color: var(--accent);
  border-color: var(--accent);
  background: linear-gradient(180deg, var(--card2) 0%, var(--card) 100%);
  box-shadow: 0 0 12px rgba(0,173,181,0.2), inset 0 0 8px rgba(0,173,181,0.1);
}
.tab-icon { font-size: 16px; display: block; margin-bottom: 2px; }

.page { display: none; flex-direction: column; gap: 10px; }
.page.active { display: flex; }

.card {
  background: linear-gradient(180deg, var(--card) 0%, var(--card2) 100%);
  border: 1px solid var(--grey);
  border-radius: 14px;
  padding: 14px;
  position: relative;
}
.card-h {
  display: flex; align-items: center; gap: 8px;
  font-size: 11px;
  color: var(--muted);
  text-transform: uppercase;
  letter-spacing: 1px;
  margin-bottom: 12px;
  font-weight: 600;
}
.card-h .ico { font-size: 16px; }
.card-h .right { margin-left: auto; }

.grid-2 { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; }
.grid-3 { display: grid; grid-template-columns: repeat(3, 1fr); gap: 8px; }

.stat {
  background: var(--card3);
  border: 1px solid var(--grey-dark);
  border-radius: 10px;
  padding: 10px 8px;
  text-align: center;
}
.stat-l { font-size: 10px; color: var(--muted); text-transform: uppercase; letter-spacing: 0.6px; margin-bottom: 4px; }
.stat-v { font-size: 22px; font-weight: 700; line-height: 1; font-variant-numeric: tabular-nums; }
.stat-u { font-size: 11px; color: var(--muted); font-weight: 400; margin-left: 2px; }
.stat .stat-sub { font-size: 10px; color: var(--dim); margin-top: 4px; }

.row {
  display: flex; justify-content: space-between; align-items: center;
  padding: 7px 0;
  font-size: 13px;
}
.row + .row { border-top: 1px solid var(--grey-dark); }
.row .lbl { color: var(--muted); font-size: 12px; }
.row .val { font-weight: 600; font-variant-numeric: tabular-nums; }

.pbar {
  width: 100%; height: 8px;
  background: var(--grey-dark);
  border-radius: 4px;
  overflow: hidden;
  margin-top: 8px;
}
.pbar-fill {
  height: 100%;
  background: var(--green);
  transition: width 0.5s, background 0.3s;
  border-radius: 4px;
}

/* === TILT === */
.tilt-wrap {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
}
@media (max-width: 480px) { .tilt-wrap { grid-template-columns: 1fr; } }

.tilt-card {
  background: linear-gradient(180deg, var(--card) 0%, var(--card2) 100%);
  border: 1px solid var(--grey);
  border-radius: 14px;
  padding: 12px;
  display: flex; flex-direction: column; align-items: center;
  position: relative; overflow: hidden;
}
.tilt-arc {
  width: 100%; height: 200px;
  position: relative;
  display: flex; justify-content: center; align-items: flex-end;
  margin-top: 4px;
}
.arc-bg {
  width: 280px; height: 280px;
  border: 2px solid var(--grey-dark);
  border-radius: 50%;
  position: absolute;
  bottom: -140px;
}
.arc-bg::after {
  content: "";
  position: absolute;
  width: 1px; height: 30px;
  background: var(--grey);
  left: 50%; top: -2px;
  transform: translateX(-50%);
}
.angle-readout {
  position: absolute; top: -2px; left: 50%;
  transform: translateX(-50%);
  font-size: 28px; font-weight: 700;
  color: var(--green);
  text-shadow: 0 0 12px rgba(0,0,0,0.8);
  z-index: 10;
  font-variant-numeric: tabular-nums;
}
.rot-c {
  position: absolute; bottom: 0; left: 50%;
  width: 200px; height: 140px;
  display: flex; justify-content: center; align-items: flex-end;
  transform-origin: center bottom;
  transition: transform 0.2s ease-out;
  z-index: 5;
  transform: translateX(-50%) rotate(0deg);
}
.needle {
  position: absolute; top: -10px; left: 50%;
  width: 4px; height: 22px;
  background: var(--green);
  transform: translateX(-50%);
  border-radius: 2px;
  box-shadow: 0 0 10px var(--green);
  z-index: 10;
  transition: background 0.3s, box-shadow 0.3s;
}
.svg-h {
  position: absolute; bottom: 0; left: 0;
  width: 100%; height: 130px;
  display: flex; align-items: flex-end; justify-content: center;
  z-index: 2;
}
.svg-h svg {
  height: 100% !important;
  width: auto !important;
  max-width: 100%;
  color: var(--text);
}
.svg-h.top svg path { fill: none !important; stroke: currentColor !important; stroke-linejoin: round; }
.svg-h.top svg path[fill="black"],
.svg-h.top svg path[fill="#000000"],
.svg-h.top svg path[fill="rgb(0, 0, 0)"] {
  fill: currentColor !important;
}
.svg-h.side svg path { fill: none !important; stroke: currentColor !important; stroke-linejoin: round; }
.svg-h.side svg path[fill="black"],
.svg-h.side svg path[fill="#000000"],
.svg-h.side svg path[fill="rgb(0, 0, 0)"] {
  fill: currentColor !important;
}
.tilt-dir {
  margin-top: 10px;
  font-size: 12px; color: var(--muted);
  text-transform: uppercase; letter-spacing: 1px;
  height: 14px; text-align: center;
}

/* === BUBBLE === */
.bubble-w { display: flex; flex-direction: column; align-items: center; padding: 12px 0; }
.bubble-o {
  width: 110px; height: 110px;
  border: 2px solid var(--grey);
  border-radius: 50%;
  position: relative;
  background: var(--card3);
}
.bubble-o::before, .bubble-o::after {
  content: "";
  position: absolute;
  border: 1px solid rgba(255,255,255,0.04);
  border-radius: 50%;
}
.bubble-o::before { inset: 18px; }
.bubble-o::after  { inset: 38px; }
.bubble-cross-h, .bubble-cross-v { position: absolute; background: var(--grey); }
.bubble-cross-h { top: 50%; left: 0; width: 100%; height: 1px; }
.bubble-cross-v { left: 50%; top: 0; width: 1px; height: 100%; }
.bubble-i {
  width: 22px; height: 22px;
  background: var(--accent);
  border-radius: 50%;
  position: absolute;
  top: 44px; left: 44px;
  box-shadow: 0 0 14px var(--accent), inset 0 -2px 4px rgba(0,0,0,0.2);
  transition: left 0.25s, top 0.25s, background 0.3s;
}

/* === TANK / GAS === */
.tank {
  width: 80px; height: 130px;
  border: 2px solid var(--grey);
  border-radius: 6px 6px 8px 8px;
  position: relative;
  overflow: hidden;
  background: var(--card3);
  margin: 0 auto 12px;
}
.tank-fill {
  position: absolute;
  bottom: 0; left: 0; right: 0;
  background: linear-gradient(to top, #0969da, #58a6ff);
  height: 0%;
  transition: height 0.6s ease-out;
}
.tank-fill::before {
  content: ""; position: absolute;
  top: -3px; left: 0; right: 0; height: 3px;
  background: rgba(255,255,255,0.4);
}
.tank-pct {
  position: absolute; top: 50%; left: 50%;
  transform: translate(-50%, -50%);
  font-weight: 700; font-size: 18px;
  z-index: 2;
  text-shadow: 0 1px 4px rgba(0,0,0,0.9);
  font-variant-numeric: tabular-nums;
}
.gas { width: 60px; height: 130px; position: relative; margin: 0 auto 12px; }
.gas-cap {
  width: 24px; height: 14px;
  background: var(--grey);
  position: absolute; top: 0; left: 50%;
  transform: translateX(-50%);
  border-radius: 3px 3px 0 0;
}
.gas-body {
  width: 100%; height: 116px;
  border: 2px solid var(--grey);
  border-radius: 12px 12px 6px 6px;
  position: absolute; bottom: 0;
  overflow: hidden;
  background: var(--card3);
}
.gas-fill {
  position: absolute; bottom: 0; left: 0; right: 0;
  background: linear-gradient(to top, #db6d28, #f0883e);
  height: 0%;
  transition: height 0.6s ease-out;
}
.gas-pct {
  position: absolute; top: 50%; left: 50%;
  transform: translate(-50%, -50%);
  font-weight: 700; font-size: 14px;
  z-index: 2;
  text-shadow: 0 1px 3px rgba(0,0,0,0.9);
}

.big-status {
  text-align: center;
  padding: 14px 10px;
  border-radius: 10px;
  background: var(--card3);
  border: 1px solid var(--grey-dark);
  font-weight: 700;
  font-size: 14px;
}
.big-status.on { background: rgba(63,185,80,0.1); color: var(--green); border-color: var(--green-dark); }
.big-status.off { color: var(--muted); }

.weather-card { text-align: center; padding: 18px 10px; }
.weather-ico { font-size: 56px; line-height: 1; filter: drop-shadow(0 2px 8px rgba(0,0,0,0.5)); }
.weather-txt { font-size: 16px; font-weight: 700; margin-top: 8px; }
.weather-sub { font-size: 11px; color: var(--muted); margin-top: 4px; }

.sig-bars {
  display: inline-flex; align-items: flex-end; gap: 2px;
  height: 18px;
  vertical-align: middle;
  margin-right: 6px;
}
.sig-bars span { width: 3px; background: var(--grey); border-radius: 1px; }
.sig-bars span:nth-child(1) { height: 5px; }
.sig-bars span:nth-child(2) { height: 8px; }
.sig-bars span:nth-child(3) { height: 11px; }
.sig-bars span:nth-child(4) { height: 14px; }
.sig-bars span:nth-child(5) { height: 17px; }
.sig-bars span.on { background: var(--green); }

/* === NUMBER CONTROLS === */
.num-row { display: flex; align-items: center; gap: 10px; padding: 12px 0; }
.num-row + .num-row { border-top: 1px solid var(--grey-dark); }
.num-row .lbl { flex: 1; font-size: 13px; }
.num-row .lbl small { display: block; color: var(--muted); font-size: 10px; margin-top: 2px; }
.num-btn {
  width: 38px; height: 38px;
  background: var(--card3);
  border: 1px solid var(--grey);
  color: var(--text);
  border-radius: 8px;
  font-size: 20px; font-weight: 700;
  cursor: pointer;
  transition: all 0.15s;
}
.num-btn:active { background: var(--accent); color: var(--bg); transform: scale(0.95); }
.num-val {
  min-width: 70px; text-align: center;
  font-weight: 700; font-size: 14px;
  font-variant-numeric: tabular-nums;
}
.num-u { color: var(--muted); font-size: 10px; margin-left: 2px; font-weight: 400; }

.btn {
  width: 100%;
  padding: 14px;
  background: var(--green-dark);
  border: 1px solid transparent;
  color: white;
  font-weight: 700;
  border-radius: 10px;
  font-size: 13px;
  text-transform: uppercase;
  letter-spacing: 0.6px;
  cursor: pointer;
  transition: all 0.15s;
  margin-top: 6px;
}
.btn:active { transform: scale(0.98); }
.btn.sec { background: var(--card3); color: var(--text); border-color: var(--grey); }
.btn.warn { background: var(--card3); color: var(--orange); border-color: var(--orange); }
.btn.danger { background: rgba(248,81,73,0.15); color: var(--red); border-color: var(--red); }

.gps-dot {
  display: inline-block;
  width: 8px; height: 8px;
  border-radius: 50%;
  background: var(--grey);
  margin-right: 6px;
}
.gps-dot.fix { background: var(--green); box-shadow: 0 0 6px var(--green); animation: pulse 2s infinite; }
@keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.6; } }

.sec-h {
  font-size: 10px; color: var(--dim);
  text-transform: uppercase; letter-spacing: 1.5px;
  font-weight: 700; padding: 6px 4px 2px;
}

.coord-mono { font-family: ui-monospace, "SF Mono", Menlo, monospace; font-size: 12px; }

/* === LOG VIEWER === */
.log-stats {
  display: grid;
  grid-template-columns: 1fr 1fr 1fr;
  gap: 8px;
  margin-bottom: 10px;
}
.log-controls {
  display: flex; gap: 6px; flex-wrap: wrap;
  margin-bottom: 10px;
}
.log-controls .btn { margin-top: 0; flex: 1; min-width: 100px; padding: 10px; font-size: 11px; }
.log-list {
  max-height: 60vh;
  overflow-y: auto;
  background: var(--card3);
  border: 1px solid var(--grey);
  border-radius: 10px;
  padding: 4px;
  font-family: ui-monospace, "SF Mono", Menlo, monospace;
  font-size: 11px;
  -webkit-overflow-scrolling: touch;
}
.log-entry {
  padding: 4px 8px;
  border-bottom: 1px solid var(--grey-dark);
  word-break: break-all;
  color: var(--muted);
}
.log-entry .ts { color: var(--dim); margin-right: 6px; }
.log-entry .id { color: var(--accent); margin-right: 6px; }
.log-entry .v { color: var(--text); }
.log-entry.txt .v { color: var(--blue); }
.log-entry.bin .v { color: var(--purple); }

.log-filter {
  width: 100%;
  padding: 10px;
  background: var(--card3);
  border: 1px solid var(--grey);
  color: var(--text);
  border-radius: 8px;
  font-size: 13px;
  margin-bottom: 8px;
  -webkit-user-select: text;
  user-select: text;
}
.log-filter::placeholder { color: var(--muted); }
.log-empty {
  text-align: center; padding: 30px;
  color: var(--muted); font-size: 12px;
}
</style>
</head>
<body>

<div class="statusbar">
  <div class="dot" id="conn-dot"></div>
  <span id="conn-txt">Jungiamasi...</span>
  <span class="spacer"></span>
  <span class="chip" id="chip-220v">220V —</span>
  <span class="chip dim" id="chip-uptime">⏱ —</span>
  <span class="chip dim" id="chip-logs">📊 0</span>
</div>

<div class="tabs">
  <div class="tab active" data-tab="lvl"><span class="tab-icon">📐</span>Lygis</div>
  <div class="tab" data-tab="sts"><span class="tab-icon">⚡</span>Statusas</div>
  <div class="tab" data-tab="clm"><span class="tab-icon">🌡️</span>Klimatas</div>
  <div class="tab" data-tab="com"><span class="tab-icon">📡</span>Ryšys</div>
  <div class="tab" data-tab="set"><span class="tab-icon">⚙️</span>Nustat.</div>
  <div class="tab" data-tab="log"><span class="tab-icon">📊</span>Logai</div>
</div>

<!-- LYGIS -->
<div id="lvl" class="page active">
  <div class="tilt-wrap">
    <div class="tilt-card">
      <div class="card-h"><span class="ico">↔️</span> Šoninis (K/D)</div>
      <div class="tilt-arc">
        <div class="angle-readout" id="ang-s">0.0°</div>
        <div class="arc-bg"></div>
        <div class="rot-c" id="rot-s">
          <div class="needle" id="ndl-s"></div>
          <div class="svg-h top" id="svg-s-slot"></div>
        </div>
      </div>
      <div class="tilt-dir" id="dir-s">Lygu</div>
    </div>

    <div class="tilt-card">
      <div class="card-h"><span class="ico">↕️</span> Išilginis (P/G)</div>
      <div class="tilt-arc">
        <div class="angle-readout" id="ang-f">0.0°</div>
        <div class="arc-bg"></div>
        <div class="rot-c" id="rot-f">
          <div class="needle" id="ndl-f"></div>
          <div class="svg-h side" id="svg-f-slot"></div>
        </div>
      </div>
      <div class="tilt-dir" id="dir-f">Lygu</div>
    </div>
  </div>

  <div class="card">
    <div class="card-h"><span class="ico">🎯</span> Burbulas — 2D niveliras</div>
    <div class="bubble-w">
      <div class="bubble-o">
        <div class="bubble-cross-h"></div>
        <div class="bubble-cross-v"></div>
        <div class="bubble-i" id="bubble"></div>
      </div>
    </div>
    <button class="btn" data-press="btn_cal_tilt">🎯 Kalibruoti nulį</button>
  </div>
</div>

<!-- STATUSAS -->
<div id="sts" class="page">
  <!-- Baterijos kortelė pašalinta, nes nėra duomenų -->

  <div class="card">
    <div class="card-h"><span class="ico">🔌</span> Išorinis tinklas</div>
    <div class="big-status off" id="status-220v">220V nėra</div>
  </div>

  <div class="sec-h">💧 Ištekliai</div>

  <div class="grid-2">
    <div class="card">
      <div class="card-h"><span class="ico">💧</span> Vanduo</div>
      <div class="tank">
        <div class="tank-fill" id="tank-fill"></div>
        <div class="tank-pct" id="v-water-pct">--%</div>
      </div>
      <div class="row"><span class="lbl">Lygis</span><span class="val" id="v-water-cm">— cm</span></div>
      <div class="row"><span class="lbl">TDS</span><span class="val" id="v-water-tds">— PPM</span></div>
    </div>

    <div class="card">
      <div class="card-h"><span class="ico">🔥</span> Dujos</div>
      <div class="gas">
        <div class="gas-cap"></div>
        <div class="gas-body">
          <div class="gas-fill" id="gas-fill"></div>
          <div class="gas-pct" id="v-gas-pct">--%</div>
        </div>
      </div>
      <div class="row"><span class="lbl">Likutis</span><span class="val" id="v-gas-net" style="font-size:15px;">— kg</span></div>
      <div class="row"><span class="lbl">Bendras</span><span class="val" id="v-gas-total">— kg</span></div>
      <button class="btn sec" style="font-size:11px; padding:9px;" data-press="btn_gas_tare">⚖️ Taruoti</button>
    </div>
  </div>
</div>

<!-- KLIMATAS -->
<div id="clm" class="page">
  <div class="card weather-card">
    <div class="weather-ico" id="v-weather-ico">⛅</div>
    <div class="weather-txt" id="v-weather">Kraunama...</div>
    <div class="weather-sub" id="v-pressure-trend">Tendencija: —</div>
  </div>

  <div class="grid-2">
    <div class="card">
      <div class="card-h"><span class="ico">🌡️</span> Kajutės temp.</div>
      <div class="stat-v" style="font-size:32px;" id="v-cabin-temp">--<span class="stat-u">°C</span></div>
    </div>
    <div class="card">
      <div class="card-h"><span class="ico">🔽</span> Slėgis</div>
      <div class="stat-v" style="font-size:32px;" id="v-pressure">--<span class="stat-u">hPa</span></div>
    </div>
  </div>

  <div class="card">
    <div class="card-h"><span class="ico">🗻</span> Aukštis (barometrinis)</div>
    <div class="stat-v" style="font-size:32px;" id="v-altitude-baro">--<span class="stat-u">m</span></div>
    <div class="row"><span class="lbl">QNH atskaita</span><span class="val" id="v-qnh">— hPa</span></div>
    <div class="row"><span class="lbl">GPS aukštis (palyg.)</span><span class="val" id="v-gps-alt">— m</span></div>
  </div>
</div>

<!-- RYŠYS -->
<div id="com" class="page">
  <div class="card">
    <div class="card-h"><span class="ico">📡</span> GSM / 4G</div>
    <div class="row"><span class="lbl">Operatorius</span><span class="val" id="v-gsm-op">—</span></div>
    <div class="row">
      <span class="lbl">Signalas</span>
      <span class="val">
        <span class="sig-bars" id="sig-bars"><span></span><span></span><span></span><span></span><span></span></span>
        <span id="v-gsm-signal">—%</span>
      </span>
    </div>
  </div>

  <div class="card">
    <div class="card-h"><span class="ico">🛰️</span> GPS</div>
    <div class="row">
      <span class="lbl"><span class="gps-dot" id="gps-dot"></span>Statusas</span>
      <span class="chip dim" id="v-gps-fix">Ieškoma...</span>
    </div>
    <div class="row"><span class="lbl">Koordinatės</span><span class="val coord-mono" id="v-gps-coords">—</span></div>
    <div class="row"><span class="lbl">Palydovai</span><span class="val" id="v-gps-sats">— vnt</span></div>
    <div class="row"><span class="lbl">Aukštis</span><span class="val" id="v-gps-alt2">— m</span></div>
    <div class="row"><span class="lbl">Kursas</span><span class="val" id="v-gps-course">—°</span></div>

    <div class="grid-2" style="margin-top:12px;">
      <div class="stat">
        <div class="stat-l">Greitis</div>
        <div class="stat-v" id="v-gps-speed">--<span class="stat-u">km/h</span></div>
      </div>
      <div class="stat">
        <div class="stat-l">Kelionė</div>
        <div class="stat-v" id="v-odometer">--<span class="stat-u">km</span></div>
      </div>
    </div>
    <button class="btn sec" data-press="btn_reset_odometer">🔄 Nulinti kelionę</button>
  </div>

  <div class="card">
    <div class="card-h"><span class="ico">💻</span> Sistema</div>
    <div class="row"><span class="lbl">Darbo laikas</span><span class="val" id="v-uptime">—</span></div>
    <div class="row"><span class="lbl">ESP temp.</span><span class="val" id="v-esp-temp">— °C</span></div>
  </div>
</div>

<!-- NUSTATYMAI -->
<div id="set" class="page">
  <div class="card">
    <div class="card-h"><span class="ico">🛁</span> Vandens bakas</div>
    <div class="num-row" data-num="tank_fresh_empty" data-step="1" data-min="0" data-max="200" data-unit="cm">
      <div class="lbl">Tuščio bako reikšmė<small>Atstumas nuo sensoriaus</small></div>
      <button class="num-btn" data-step-dir="-1">−</button>
      <span class="num-val">—<span class="num-u">cm</span></span>
      <button class="num-btn" data-step-dir="1">+</button>
    </div>
    <div class="num-row" data-num="tank_fresh_full" data-step="1" data-min="0" data-max="200" data-unit="cm">
      <div class="lbl">Pilno bako reikšmė<small>Atstumas nuo sensoriaus</small></div>
      <button class="num-btn" data-step-dir="-1">−</button>
      <span class="num-val">—<span class="num-u">cm</span></span>
      <button class="num-btn" data-step-dir="1">+</button>
    </div>
  </div>

  <div class="card">
    <div class="card-h"><span class="ico">🔥</span> Dujos</div>
    <div class="num-row" data-num="cylinder_tare" data-step="0.1" data-min="0" data-max="20" data-unit="kg">
      <div class="lbl">Baliono tara<small>Tuščio baliono svoris</small></div>
      <button class="num-btn" data-step-dir="-1">−</button>
      <span class="num-val">—<span class="num-u">kg</span></span>
      <button class="num-btn" data-step-dir="1">+</button>
    </div>
    <div class="num-row" data-num="gas_ref_weight" data-step="0.1" data-min="0.5" data-max="30" data-unit="kg">
      <div class="lbl">Etalono svoris<small>Žinomas svoris kalibravimui</small></div>
      <button class="num-btn" data-step-dir="-1">−</button>
      <span class="num-val">—<span class="num-u">kg</span></span>
      <button class="num-btn" data-step-dir="1">+</button>
    </div>
    <button class="btn" data-press="btn_gas_tare">⚖️ Taruoti svarstykles (0 kg)</button>
    <button class="btn sec" data-press="btn_gas_cal_span">🎯 Kalibruoti pagal etaloną</button>
  </div>

  <div class="card">
    <div class="card-h"><span class="ico">🌡️</span> Kalibravimas</div>
    <div class="num-row" data-num="temp_offset" data-step="0.1" data-min="-10" data-max="10" data-unit="°C">
      <div class="lbl">Temperatūros korekcija<small>BMP180 offset</small></div>
      <button class="num-btn" data-step-dir="-1">−</button>
      <span class="num-val">—<span class="num-u">°C</span></span>
      <button class="num-btn" data-step-dir="1">+</button>
    </div>
    <div class="num-row" data-num="sea_level_pressure" data-step="0.1" data-min="950" data-max="1080" data-unit="hPa">
      <div class="lbl">QNH (jūros slėgis)<small>Aukščio atskaita</small></div>
      <button class="num-btn" data-step-dir="-1">−</button>
      <span class="num-val">—<span class="num-u">hPa</span></span>
      <button class="num-btn" data-step-dir="1">+</button>
    </div>
  </div>

  <div class="card">
    <div class="card-h"><span class="ico">🎯</span> Veiksmai</div>
    <button class="btn" data-press="btn_cal_tilt">Kalibruoti posvyrio nulį</button>
    <button class="btn sec" data-press="btn_reset_odometer">Nulinti kelionės atstumą</button>
  </div>
</div>

<!-- LOGAI -->
<div id="log" class="page">
  <div class="card">
    <div class="card-h"><span class="ico">📊</span> Logų statistika</div>
    <div class="log-stats">
      <div class="stat">
        <div class="stat-l">Įrašai</div>
        <div class="stat-v" id="log-count" style="font-size:18px;">0</div>
      </div>
      <div class="stat">
        <div class="stat-l">Dydis</div>
        <div class="stat-v" id="log-size" style="font-size:18px;">0 KB</div>
      </div>
      <div class="stat">
        <div class="stat-l">Pradžia</div>
        <div class="stat-v" id="log-start" style="font-size:11px;">—</div>
      </div>
    </div>

    <div class="log-controls">
      <button class="btn sec" id="btn-export-csv">📥 CSV</button>
      <button class="btn sec" id="btn-export-json">📥 JSON</button>
      <button class="btn sec" id="btn-pause">⏸ Pauzė</button>
      <button class="btn danger" id="btn-clear">🗑️ Išvalyti</button>
    </div>

    <input class="log-filter" id="log-filter" placeholder="🔍 Filtruoti pagal ID arba reikšmę..." />

    <div class="log-list" id="log-list">
      <div class="log-empty">Logų sistema veikia. Įrašai pildomi automatiškai realiu laiku.</div>
    </div>
  </div>

  <div class="card">
    <div class="card-h"><span class="ico">⚙️</span> Logų nustatymai</div>
    <div class="row">
      <span class="lbl">Maksimalus įrašų skaičius</span>
      <span class="val" id="log-max-display">10000</span>
    </div>
    <div class="row">
      <span class="lbl">Saugojimo vieta</span>
      <span class="val" id="log-storage">localStorage</span>
    </div>
    <div class="row">
      <span class="lbl">Sample'inimo intervalas</span>
      <span class="val" id="log-interval-display">2s / sensor</span>
    </div>
    <p style="font-size:11px; color:var(--muted); margin:10px 0 0;">
      Logai saugomi planšetėje (offline). Kelionės metu galima eksportuoti į CSV/JSON failą analizei.
      Tas pats sensorius rašomas ne dažniau kaip kas 2s, kad nesiprigrūsdintų atminties.
    </p>
  </div>
</div>

<!-- Crafter top-view (for Šoninis K/D) -->
<template id="crafter-top"><svg width="1920.00" height="1440.00" viewBox="0.00 0.00 1920.00 1440.00" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">
<g stroke-linecap="round" id="Layer_2_Copy">
<path d="M757.86,215.21 Q688.90,239.92 619.00,571.00" stroke-width="5px"/>
<path d="M618.81,570.66 Q536.85,662.86 550.17,902.60" stroke-width="5px"/>
<path d="M549.96,900.83 Q537.94,963.94 572.50,1036.81" stroke-width="5px"/>
<path d="M573.25,1036.81 Q565.74,1114.20 686.70,1117.96" stroke-width="5px"/>
<path d="M685.20,1117.96 Q712.25,1106.69 712.25,1106.69" stroke-width="5px"/>
<path d="M1206.03,215.84 Q1273.07,248.26 1341.00,570.00" stroke-width="5px"/>
<path d="M1341.19,569.66 Q1423.15,661.86 1409.83,901.60" stroke-width="5px"/>
<path d="M1410.04,899.83 Q1422.06,962.94 1387.50,1035.81" stroke-width="5px"/>
<path d="M1386.75,1035.81 Q1394.26,1113.20 1273.30,1116.96" stroke-width="5px"/>
<path d="M1274.80,1116.96 Q1258.18,1110.03 1251.80,1107.37" stroke-width="5px"/>
<path d="M758.18,214.55 Q982.73,197.73 1207.27,215.91" stroke-width="5px"/>
<path d="M710.30,1106.06 Q982.42,1129.70 1253.33,1107.27" stroke-width="5px"/>
<path d="M698.72,749.95 L724.68,961.00" stroke-width="5px"/>
<path d="M724.27,960.56 Q739.29,1051.47 818.56,1051.84" stroke-width="5px"/>
<path d="M1262.28,748.95 L1236.32,960.00" stroke-width="5px"/>
<path d="M1236.73,959.56 Q1221.71,1050.47 1142.44,1050.84" stroke-width="5px"/>
<path d="M698.07,749.86 Q971.90,762.53 1262.81,747.66" stroke-width="5px"/>
<path d="M818.18,1051.47 Q973.70,1053.72 1149.14,1050.34" stroke-width="5px"/>
<path d="M590.00,960.50 Q584.00,1005.50 650.50,1021.25" stroke-width="5px"/>
<path d="M589.75,960.00 Q622.00,970.50 662.75,973.00" stroke-width="5px"/>
<path d="M662.25,972.50 Q666.50,1010.00 666.50,1010.25" stroke-width="5px"/>
<path d="M648.76,1020.85 Q668.29,1028.74 666.79,1009.58" stroke-width="5px"/>
<path d="M642.37,734.79 Q639.37,690.08 581.89,656.27" stroke-width="5px"/>
<path d="M581.89,655.90 Q555.22,702.86 566.87,871.90" stroke-width="5px"/>
<path d="M642.00,733.66 Q650.64,866.27 650.64,866.27" stroke-width="5px"/>
<path d="M566.87,870.77 Q637.12,885.05 637.12,885.05" stroke-width="5px"/>
<path d="M635.99,884.67 Q654.02,888.81 650.64,864.76" stroke-width="5px"/>
<path d="M740.39,580.56 Q729.12,581.07 734.41,586.71" stroke-width="5px"/>
<path d="M733.39,586.20 Q749.27,598.83 749.10,598.66" stroke-width="5px"/>
<path d="M748.92,598.83 Q754.05,602.93 762.58,601.56" stroke-width="5px"/>
<path d="M719.75,327.00 Q698.00,327.50 692.00,354.00" stroke-width="5px"/>
<path d="M691.94,352.05 Q632.81,566.25 632.81,565.92" stroke-width="5px"/>
<path d="M1369.00,958.50 Q1375.00,1003.50 1308.50,1019.25" stroke-width="5px"/>
<path d="M1369.25,958.00 Q1337.00,968.50 1296.25,971.00" stroke-width="5px"/>
<path d="M1296.75,970.50 Q1292.50,1008.00 1292.50,1008.25" stroke-width="5px"/>
<path d="M1310.24,1018.85 Q1290.71,1026.74 1292.21,1007.58" stroke-width="5px"/>
<path d="M1316.63,732.79 Q1319.63,688.08 1377.11,654.27" stroke-width="5px"/>
<path d="M1377.11,653.90 Q1403.78,700.86 1392.13,869.90" stroke-width="5px"/>
<path d="M1317.00,731.66 Q1308.36,864.27 1308.36,864.27" stroke-width="5px"/>
<path d="M1392.13,868.77 Q1321.88,883.05 1321.88,883.05" stroke-width="5px"/>
<path d="M1323.01,882.67 Q1304.98,886.81 1308.36,862.76" stroke-width="5px"/>
<path d="M1218.61,578.56 Q1229.88,579.07 1224.59,584.71" stroke-width="5px"/>
<path d="M1225.61,584.20 Q1209.73,596.83 1209.90,596.66" stroke-width="5px"/>
<path d="M1210.08,596.83 Q1204.95,600.93 1196.42,599.56" stroke-width="5px"/>
<path d="M1239.25,325.00 Q1261.00,325.50 1267.00,352.00" stroke-width="5px"/>
<path d="M1267.06,350.05 Q1326.19,564.25 1326.19,563.92" stroke-width="5px"/>
<path d="M760.45,601.36 L1201.82,600.00" stroke-width="5px"/>
<path d="M738.64,580.00 L1220.00,578.64" stroke-width="5px"/>
<path d="M632.51,566.39 L1326.17,561.98" stroke-width="5px"/>
<path d="M716.80,327.27 Q1028.65,307.44 1249.04,326.17" stroke-width="5px"/>
<path d="M701.82,338.18 Q698.41,327.50 852.73,324.09" stroke-width="5px"/>
<path d="M853.41,324.32 Q881.36,324.09 898.64,322.27" stroke-width="5px"/>
<path d="M877.95,338.86 Q888.64,352.50 912.50,349.32" stroke-width="5px"/>
<path d="M877.27,338.41 Q873.18,328.41 843.41,322.73" stroke-width="5px"/>
<path d="M899.17,321.28 Q957.44,320.66 957.44,320.66" stroke-width="5px"/>
<path d="M957.02,321.07 Q912.60,346.69 911.57,350.21" stroke-width="5px"/>
<path d="M870.25,326.45 Q891.74,347.11 910.33,344.83" stroke-width="5px"/>
<path d="M875.21,325.62 Q899.59,341.94 916.32,342.15" stroke-width="5px"/>
<path d="M884.71,325.83 Q907.44,339.46 920.87,340.08" stroke-width="5px"/>
<path d="M892.15,326.65 Q935.33,326.45 942.56,325.21" stroke-width="5px"/>
<path d="M935.95,327.48 Q918.60,335.95 918.80,335.95" stroke-width="5px"/>
<path d="M900.83,329.55 Q917.36,333.88 917.56,334.30" stroke-width="5px"/>
<path d="M916.53,330.58 Q923.55,329.34 923.55,329.34" stroke-width="5px"/>
<path d="M834.50,327.50 Q711.00,327.50 700.00,341.50" stroke-width="5px"/>
<path d="M830.95,327.39 Q840.16,327.01 870.21,328.32" stroke-width="5px"/>
<path d="M728.04,334.96 Q698.66,341.89 687.50,373.07" stroke-width="5px"/>
<path d="M717.91,337.27 Q699.82,345.36 689.81,364.09" stroke-width="5px"/>
<path d="M717.65,336.89 Q699.43,345.10 692.12,358.18" stroke-width="5px"/>
<path d="M707.90,339.33 Q694.69,350.74 693.66,353.57" stroke-width="5px"/>
<path d="M701.23,340.74 Q694.94,350.74 695.07,349.46" stroke-width="5px"/>
<path d="M722.25,331.00 Q710.12,332.88 710.12,332.88" stroke-width="5px"/>
<path d="M707.38,337.38 Q699.88,343.75 700.25,343.62" stroke-width="5px"/>
<path d="M697.25,343.38 Q695.00,352.50 695.00,352.50" stroke-width="5px"/>
<path d="M691.25,359.88 Q693.88,353.12 693.88,353.00" stroke-width="5px"/>
<path d="M689.25,364.25 Q695.12,350.12 689.88,362.88" stroke-width="5px"/>
<path d="M1258.18,336.18 Q1261.59,325.50 1107.27,322.09" stroke-width="5px"/>
<path d="M1106.59,322.32 Q1078.64,322.09 1061.36,320.27" stroke-width="5px"/>
<path d="M1082.05,336.86 Q1071.36,350.50 1047.50,347.32" stroke-width="5px"/>
<path d="M1082.73,336.41 Q1086.82,326.41 1116.59,320.73" stroke-width="5px"/>
<path d="M1060.83,319.28 Q1002.56,318.66 1002.56,318.66" stroke-width="5px"/>
<path d="M1002.98,319.07 Q1047.40,344.69 1048.43,348.21" stroke-width="5px"/>
<path d="M1089.75,324.45 Q1068.26,345.11 1049.67,342.83" stroke-width="5px"/>
<path d="M1084.79,323.62 Q1060.41,339.94 1043.68,340.15" stroke-width="5px"/>
<path d="M1075.29,323.83 Q1052.56,337.46 1039.13,338.08" stroke-width="5px"/>
<path d="M1067.85,324.65 Q1024.67,324.45 1017.44,323.21" stroke-width="5px"/>
<path d="M1024.05,325.48 Q1041.40,333.95 1041.20,333.95" stroke-width="5px"/>
<path d="M1059.17,327.55 Q1042.64,331.88 1042.44,332.30" stroke-width="5px"/>
<path d="M1043.47,328.58 Q1036.45,327.34 1036.45,327.34" stroke-width="5px"/>
<path d="M1125.50,325.50 Q1249.00,325.50 1260.00,339.50" stroke-width="5px"/>
<path d="M1129.05,325.39 Q1119.84,325.01 1089.79,326.32" stroke-width="5px"/>
<path d="M1231.96,332.96 Q1261.34,339.89 1272.50,371.07" stroke-width="5px"/>
<path d="M1242.09,335.27 Q1260.18,343.36 1270.19,362.09" stroke-width="5px"/>
<path d="M1242.35,334.89 Q1260.57,343.10 1267.88,356.18" stroke-width="5px"/>
<path d="M1252.10,337.33 Q1265.31,348.74 1266.34,351.57" stroke-width="5px"/>
<path d="M1258.77,338.74 Q1265.06,348.74 1264.93,347.46" stroke-width="5px"/>
<path d="M912.47,351.43 Q1050.15,349.74 1050.71,349.55" stroke-width="5px"/>
<path d="M954.92,320.81 Q999.25,322.13 1006.20,322.50" stroke-width="5px"/>
<path d="M939.14,329.64 Q1016.15,327.57 1016.15,327.57" stroke-width="5px"/>
<path d="M930.69,336.78 Q1029.30,333.58 1028.74,333.77" stroke-width="5px"/>
<path d="M922.24,343.54 Q1041.89,342.41 1041.51,342.79" stroke-width="5px"/>
<path d="M913.04,345.42 Q1045.45,344.67 1045.08,344.67" stroke-width="5px"/>
<path d="M622.00,534.75 Q611.50,534.50 611.75,534.00" stroke-width="5px"/>
<path d="M611.75,533.25 Q610.50,485.00 610.50,485.00" stroke-width="5px"/>
<path d="M586.50,469.25 Q609.50,468.75 610.75,485.25" stroke-width="5px"/>
<path d="M587.00,469.75 Q555.75,467.00 556.75,490.25" stroke-width="5px"/>
<path d="M599.75,586.75 Q575.00,586.50 575.25,586.50" stroke-width="5px"/>
<path d="M576.75,586.50 Q554.75,590.75 552.50,550.25" stroke-width="5px"/>
<path d="M557.00,487.75 Q552.50,545.00 552.75,554.00" stroke-width="5px"/>
<path d="M1334.00,529.75 Q1344.50,529.50 1344.25,529.00" stroke-width="5px"/>
<path d="M1344.25,528.25 Q1345.50,480.00 1345.50,480.00" stroke-width="5px"/>
<path d="M1369.50,464.25 Q1346.50,463.75 1345.25,480.25" stroke-width="5px"/>
<path d="M1369.00,464.75 Q1400.25,462.00 1399.25,485.25" stroke-width="5px"/>
<path d="M1356.25,581.75 Q1381.00,581.50 1380.75,581.50" stroke-width="5px"/>
<path d="M1379.25,581.50 Q1401.25,585.75 1403.50,545.25" stroke-width="5px"/>
<path d="M1399.00,482.75 Q1403.50,540.00 1403.25,549.00" stroke-width="5px"/>
<path d="M741.05,762.78 Q976.03,773.55 1217.84,761.59" stroke-width="5px"/>
<path d="M746.00,807.50 Q834.32,810.42 924.31,810.99" stroke-width="5px"/>
<path d="M1031.27,810.84 Q1119.84,810.04 1210.00,807.00" stroke-width="5px"/>
<path d="M753.00,862.00 Q836.10,864.94 918.92,865.65" stroke-width="5px"/>
<path d="M1036.43,865.54 Q1119.85,864.66 1203.00,861.50" stroke-width="5px"/>
<path d="M762.50,916.50 Q986.00,923.50 1197.00,916.50" stroke-width="5px"/>
<path d="M768.50,966.00 Q976.50,981.50 1193.00,969.50" stroke-width="5px"/>
<path d="M770.00,972.50 Q798.50,1023.00 828.00,1027.00" stroke-width="5px"/>
<path d="M829.50,1028.50 Q906.00,1010.50 1002.00,979.00" stroke-width="5px"/>
<path d="M795.00,985.50 Q909.00,991.00 939.00,991.50" stroke-width="5px"/>
<path d="M805.50,1002.50 Q874.50,1005.00 874.00,1005.50" stroke-width="5px"/>
<path d="M1016.96,963.19 Q1017.83,962.56 1018.70,961.94" stroke-width="5px"/>
<path d="M1195.50,969.00 Q1080.57,977.56 963.04,977.73 Q956.29,972.86 949.54,968.00" stroke-width="5px"/>
<path d="M1194.00,975.50 Q1165.50,1026.00 1136.00,1030.00" stroke-width="5px"/>
<path d="M1134.50,1031.50 Q1058.00,1013.50 962.00,982.00" stroke-width="5px"/>
<path d="M1169.00,988.50 Q1055.00,994.00 1025.00,994.50" stroke-width="5px"/>
<path d="M1158.50,1005.50 Q1089.50,1008.00 1090.00,1008.50" stroke-width="5px"/>
<path d="M829.03,1029.56 Q1126.08,1030.30 1139.54,1031.05" stroke-width="5px"/>
<path d="M924.06,1010.85 Q1031.05,1013.09 1031.05,1013.09" stroke-width="5px"/>
<path d="M944.01,811.89 C946.29,809.52 952.60,804.75 955.94,802.86 C959.20,801.06 966.36,798.66 970.02,797.97 C974.14,797.22 980.71,797.06 984.86,797.62 C988.54,798.15 995.81,800.21 999.15,801.85 C1002.58,803.58 1009.10,808.05 1011.49,810.31 C1013.85,812.59 1018.63,818.89 1020.51,822.24 C1022.31,825.49 1024.71,832.66 1025.41,836.31 C1026.16,840.44 1026.31,847.00 1025.75,851.16 C1025.23,854.84 1023.17,862.11 1021.52,865.45 C1019.80,868.87 1015.32,875.40 1013.07,877.78 C1010.79,880.15 1004.48,884.92 1001.14,886.81 C997.88,888.61 990.72,891.01 987.06,891.70 C982.94,892.46 976.37,892.61 972.22,892.05 C968.54,891.53 961.27,889.47 957.93,887.82 C954.50,886.09 947.98,881.62 945.59,879.36 C943.23,877.08 938.45,870.78 936.57,867.43 C934.77,864.18 932.37,857.01 931.67,853.36 C930.92,849.23 930.77,842.67 931.33,838.51 C931.85,834.83 933.91,827.56 935.56,824.23 C937.28,820.80 941.76,814.27 944.01,811.89 Z" fill="black" />
</g>
</svg></template>

<!-- Side-view van (for Išilginis P/G) -->
<template id="crafter-side"><svg width="8160.00" height="6144.00" viewBox="0.00 0.00 8160.00 6144.00" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">
<g stroke-linecap="round" id="Layer_1_Copy_4">
<path d="M981.16,1904.85 Q1014.00,1732.43 1264.43,1748.85" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M982.00,1906.00 Q904.00,2316.00 914.00,2958.00" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M914.55,2959.09 Q917.27,3000.91 936.36,3026.36" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M936.51,3027.05 Q920.74,3222.01 971.45,3386.55" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M941.33,3410.00 Q970.00,3386.67 969.33,3386.67" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M940.91,3407.72 Q949.09,3516.81 992.73,3638.17" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M1131.82,3724.08 Q1022.73,3710.45 988.63,3631.36" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M1256.20,1748.76 Q5041.32,1626.45 5041.32,1626.45" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M5039.82,1625.09 Q5402.70,1620.58 5740.79,1972.20" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M5741.82,1972.73 Q6480.00,2669.09 6480.00,2669.09" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M6482.34,2668.67 Q6811.41,2764.84 7143.50,2903.08" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M7131.31,2900.11 Q7349.04,2980.92 7362.51,3203.14" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M7361.98,3200.00 Q7370.25,3403.31 7335.54,3634.71" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M7336.49,3633.00 Q7306.49,3834.00 7204.49,3837.00" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M6929.99,3871.50 Q7225.49,3834.00 7225.49,3834.00" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M6932.00,3864.00 C6930.15,3843.57 6925.73,3793.99 6923.17,3764.84 C6921.62,3746.99 6917.90,3705.55 6915.79,3690.88 C6914.23,3681.57 6910.48,3663.31 6908.29,3654.37 C6905.90,3645.55 6900.50,3628.27 6897.49,3619.82 C6894.27,3611.48 6887.21,3595.19 6883.37,3587.22 C6879.33,3579.38 6870.62,3564.06 6865.95,3556.58 C6859.02,3546.13 6853.80,3539.01 6847.15,3530.75 C6842.52,3525.25 6832.84,3514.52 6827.78,3509.29 C6822.59,3504.14 6811.79,3494.10 6806.18,3489.21 C6800.43,3484.41 6788.51,3475.06 6782.34,3470.52 C6776.03,3466.06 6762.99,3457.41 6756.26,3453.21 C6742.52,3444.99 6713.10,3429.77 6697.40,3422.76 C6681.64,3416.00 6650.09,3404.44 6634.31,3399.63 C6618.52,3395.09 6586.93,3387.97 6571.13,3385.38 C6555.32,3383.07 6523.69,3380.38 6507.87,3380.02 C6492.31,3379.93 6461.71,3381.58 6446.45,3383.30 C6431.20,3385.28 6400.68,3391.05 6385.41,3394.83 C6370.13,3398.87 6339.57,3408.75 6324.28,3414.59 C6312.05,3419.47 6278.44,3434.66 6263.77,3442.29 C6249.30,3450.09 6221.72,3466.91 6208.61,3475.93 C6195.70,3485.12 6171.24,3504.72 6159.69,3515.13 C6148.34,3525.72 6125.20,3550.02 6116.41,3560.79 C6110.95,3567.68 6100.39,3581.77 6095.29,3588.97 C6090.31,3596.28 6080.73,3611.21 6076.12,3618.83 C6071.63,3626.56 6063.03,3642.33 6058.91,3650.37 C6054.91,3658.51 6047.28,3675.11 6043.65,3683.57 C6036.63,3700.69 6025.75,3732.14 6018.97,3754.74 C6009.39,3786.81 5992.38,3843.68 5984.94,3868.46 C5982.02,3878.12 5977.55,3892.89 5976.00,3898.00" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="miter"/>
<path d="M5974.55,3894.55 L2742.75,3823.20 L2740.47,3821.39" fill="none" stroke="rgb(3, 3, 2)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M1132.73,3723.64 Q1440.15,3785.44 1793.15,3805.81 Q1801.10,3814.21 1809.05,3822.62" fill="none" stroke="rgb(3, 3, 2)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M5010.02,1633.64 Q5004.57,1638.21 4999.12,1642.78" fill="none" stroke="rgb(3, 3, 2)" stroke-width="10.40" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M5001.84,3864.56" fill="none" stroke="rgb(3, 3, 2)" stroke-width="10.40" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M3501.14,1670.93 Q3500.82,1677.03 3500.52,1683.19" fill="none" stroke="rgb(0, 0, 0)" stroke-width="10.40" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M3546.22,3837.73" fill="none" stroke="rgb(0, 0, 0)" stroke-width="10.40" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M1792.55,3803.58 Q1784.12,3793.91 1775.69,3784.24" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M1790.45,3796.11 C1792.08,3775.65 1795.94,3726.03 1798.18,3696.85 C1799.53,3678.99 1802.79,3637.51 1804.74,3622.81 C1806.20,3613.49 1809.74,3595.19 1811.83,3586.22 C1814.12,3577.38 1819.33,3560.04 1822.25,3551.55 C1825.37,3543.19 1832.25,3526.81 1836.00,3518.80 C1839.96,3510.92 1848.50,3495.50 1853.08,3487.97 C1859.89,3477.44 1865.03,3470.27 1871.59,3461.93 C1876.16,3456.38 1885.72,3445.55 1890.72,3440.25 C1895.85,3435.05 1906.54,3424.89 1912.10,3419.94 C1917.79,3415.07 1929.61,3405.59 1935.73,3400.98 C1941.99,3396.46 1954.93,3387.66 1961.61,3383.39 C1975.26,3375.02 2004.51,3359.46 2020.13,3352.28 C2035.81,3345.35 2067.23,3333.43 2082.96,3328.45 C2098.69,3323.74 2130.20,3316.26 2145.98,3313.50 C2161.75,3311.01 2193.35,3307.98 2209.17,3307.43 C2224.73,3307.17 2255.35,3308.48 2270.62,3310.03 C2285.90,3311.84 2316.48,3317.27 2331.79,3320.88 C2347.11,3324.75 2377.78,3334.29 2393.13,3339.96 C2405.42,3344.70 2439.20,3359.52 2453.95,3366.98 C2468.50,3374.62 2496.27,3391.14 2509.48,3400.01 C2522.49,3409.06 2547.17,3428.38 2558.83,3438.67 C2570.30,3449.12 2593.71,3473.17 2602.62,3483.83 C2608.16,3490.66 2618.88,3504.64 2624.05,3511.78 C2629.11,3519.04 2638.86,3533.86 2643.55,3541.43 C2648.13,3549.11 2656.91,3564.77 2661.12,3572.77 C2665.20,3580.87 2673.02,3597.38 2676.74,3605.80 C2683.95,3622.85 2695.18,3654.17 2702.22,3676.69 C2712.15,3708.66 2729.79,3765.33 2737.50,3790.03 C2740.53,3799.65 2745.16,3814.37 2746.77,3819.46" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="miter"/>
<path d="M2748.19,3815.99 L2755.26,3798.47" fill="none" stroke="rgb(3, 3, 2)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M2229.38,4075.68 C2225.57,4075.46 2206.46,4073.30 2198.79,4072.15 C2191.11,4070.82 2175.85,4067.66 2168.34,4065.84 C2160.90,4063.86 2149.27,4060.43 2139.09,4056.67 C2131.93,4053.84 2117.71,4047.65 2110.66,4044.29 C2103.69,4040.79 2090.12,4033.38 2083.50,4029.47 C2076.98,4025.47 2064.49,4016.58 2058.35,4011.91 C2052.28,4007.08 2040.40,3996.96 2034.66,3991.72 C2029.06,3986.39 2016.10,3973.11 2013.22,3969.72 C2008.32,3963.82 1998.87,3951.62 1994.32,3945.32 C1989.88,3938.89 1981.49,3925.80 1977.55,3919.17 C1971.89,3909.13 1968.46,3902.32 1963.76,3891.80 C1960.77,3884.69 1955.25,3870.16 1952.71,3862.77 C1950.35,3855.36 1946.17,3840.51 1944.34,3833.06 C1943.32,3828.73 1940.37,3810.41 1939.41,3802.74 C1938.62,3795.00 1937.55,3779.43 1937.27,3771.69 C1937.17,3763.97 1937.46,3748.64 1938.12,3741.02 C1938.91,3733.38 1941.03,3718.06 1942.36,3710.38 C1943.86,3702.71 1947.34,3687.60 1949.32,3680.16 C1952.36,3669.74 1956.51,3658.35 1959.35,3651.19 C1962.35,3644.07 1968.89,3629.92 1972.39,3622.96 C1976.03,3616.11 1985.66,3599.46 1987.75,3596.27 C1989.84,3593.08 2001.27,3577.61 2006.10,3571.55 C2011.09,3565.56 2021.46,3553.92 2026.79,3548.32 C2032.23,3542.87 2041.01,3534.52 2049.36,3527.58 C2055.39,3522.80 2067.86,3513.58 2074.30,3509.15 C2080.81,3504.87 2094.02,3496.82 2100.71,3493.04 C2107.43,3489.40 2121.38,3483.02 2128.49,3480.04 C2135.71,3477.20 2150.41,3471.97 2157.82,3469.62 C2165.24,3467.43 2183.22,3462.85 2187.60,3462.05 C2195.16,3460.75 2210.45,3458.67 2218.18,3457.88 C2225.97,3457.26 2241.50,3456.54 2249.21,3456.44 C2260.73,3456.55 2268.34,3456.98 2279.80,3458.18 C2287.46,3459.15 2302.81,3461.62 2310.47,3463.12 C2318.07,3464.78 2333.02,3468.59 2340.38,3470.73 C2344.65,3472.02 2361.99,3478.61 2369.11,3481.62 C2376.20,3484.80 2390.22,3491.66 2397.07,3495.29 C2403.80,3499.07 2416.94,3506.98 2423.21,3511.36 C2429.43,3515.87 2441.64,3525.37 2447.62,3530.36 C2453.51,3535.49 2464.86,3546.06 2470.31,3551.49 C2477.82,3559.33 2485.60,3568.63 2490.38,3574.66 C2495.05,3580.82 2504.03,3593.56 2508.31,3600.08 C2512.42,3606.65 2522.03,3623.31 2523.75,3626.72 C2525.46,3630.12 2533.14,3647.76 2535.98,3654.97 C2538.67,3662.29 2543.57,3677.08 2545.75,3684.50 C2547.75,3691.94 2550.59,3703.72 2552.43,3714.42 C2553.55,3722.04 2555.30,3737.44 2555.92,3745.23 C2556.37,3753.01 2556.74,3768.47 2556.67,3776.15 C2556.46,3783.80 2555.01,3799.07 2554.04,3806.72 C2552.89,3814.39 2550.06,3829.74 2548.40,3837.33 C2546.58,3844.85 2541.56,3862.71 2540.06,3866.90 C2537.40,3874.09 2531.56,3888.38 2528.38,3895.47 C2525.03,3902.52 2517.89,3916.33 2514.12,3923.06 C2508.26,3932.99 2504.08,3939.36 2497.31,3948.69 C2492.65,3954.83 2482.83,3966.89 2477.69,3972.78 C2472.46,3978.52 2461.69,3989.57 2456.15,3994.88 C2452.91,3997.93 2438.52,4009.65 2432.36,4014.31 C2426.06,4018.86 2413.11,4027.58 2406.53,4031.69 C2399.90,4035.63 2386.48,4043.05 2379.55,4046.29 C2372.54,4049.42 2358.21,4055.25 2350.89,4057.94 C2343.50,4060.47 2328.68,4065.01 2321.25,4067.01 C2310.70,4069.60 2298.76,4071.69 2291.15,4072.82 C2283.47,4073.78 2267.95,4075.19 2260.17,4075.64 C2252.42,4075.91 2233.19,4075.89 2229.38,4075.68 Z" fill="none" stroke="rgb(0, 0, 0)" stroke-width="35.57" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M6472.38,4117.68 C6468.57,4117.46 6449.46,4115.30 6441.79,4114.15 C6434.11,4112.82 6418.85,4109.66 6411.34,4107.84 C6403.90,4105.86 6392.27,4102.43 6382.09,4098.67 C6374.93,4095.84 6360.71,4089.65 6353.66,4086.29 C6346.69,4082.79 6333.12,4075.38 6326.50,4071.47 C6319.98,4067.47 6307.49,4058.58 6301.35,4053.91 C6295.28,4049.08 6283.40,4038.96 6277.66,4033.72 C6272.06,4028.39 6259.10,4015.11 6256.22,4011.72 C6251.32,4005.82 6241.87,3993.62 6237.32,3987.32 C6232.88,3980.89 6224.49,3967.80 6220.55,3961.17 C6214.89,3951.13 6211.46,3944.32 6206.76,3933.80 C6203.77,3926.69 6198.25,3912.16 6195.71,3904.77 C6193.35,3897.36 6189.17,3882.51 6187.34,3875.06 C6186.32,3870.73 6183.37,3852.41 6182.41,3844.74 C6181.62,3837.00 6180.55,3821.43 6180.27,3813.69 C6180.17,3805.97 6180.46,3790.64 6181.12,3783.02 C6181.91,3775.38 6184.03,3760.06 6185.36,3752.38 C6186.86,3744.71 6190.34,3729.60 6192.32,3722.16 C6195.36,3711.74 6199.51,3700.35 6202.35,3693.19 C6205.35,3686.07 6211.89,3671.92 6215.39,3664.96 C6219.03,3658.11 6228.66,3641.46 6230.75,3638.27 C6232.84,3635.08 6244.27,3619.61 6249.10,3613.55 C6254.09,3607.56 6264.46,3595.92 6269.79,3590.32 C6275.23,3584.87 6284.01,3576.52 6292.36,3569.58 C6298.39,3564.80 6310.86,3555.58 6317.30,3551.15 C6323.81,3546.87 6337.02,3538.82 6343.71,3535.04 C6350.43,3531.40 6364.38,3525.02 6371.49,3522.04 C6378.71,3519.20 6393.41,3513.97 6400.82,3511.62 C6408.24,3509.43 6426.22,3504.85 6430.60,3504.05 C6438.16,3502.75 6453.45,3500.67 6461.18,3499.88 C6468.97,3499.26 6484.50,3498.54 6492.21,3498.44 C6503.73,3498.55 6511.34,3498.98 6522.80,3500.18 C6530.46,3501.15 6545.81,3503.62 6553.47,3505.12 C6561.07,3506.78 6576.02,3510.59 6583.38,3512.73 C6587.65,3514.02 6604.99,3520.61 6612.11,3523.62 C6619.20,3526.80 6633.22,3533.66 6640.07,3537.29 C6646.80,3541.07 6659.94,3548.98 6666.21,3553.36 C6672.43,3557.87 6684.64,3567.37 6690.62,3572.36 C6696.51,3577.49 6707.86,3588.06 6713.31,3593.49 C6720.82,3601.33 6728.60,3610.63 6733.38,3616.66 C6738.05,3622.82 6747.03,3635.56 6751.31,3642.08 C6755.42,3648.65 6765.03,3665.31 6766.75,3668.72 C6768.46,3672.12 6776.14,3689.76 6778.98,3696.97 C6781.67,3704.29 6786.57,3719.08 6788.75,3726.50 C6790.75,3733.94 6793.59,3745.72 6795.43,3756.42 C6796.55,3764.04 6798.30,3779.44 6798.92,3787.23 C6799.37,3795.01 6799.74,3810.47 6799.67,3818.15 C6799.46,3825.80 6798.01,3841.07 6797.04,3848.72 C6795.89,3856.39 6793.06,3871.74 6791.40,3879.33 C6789.58,3886.85 6784.56,3904.71 6783.06,3908.90 C6780.40,3916.09 6774.56,3930.38 6771.38,3937.47 C6768.03,3944.52 6760.89,3958.33 6757.12,3965.06 C6751.26,3974.99 6747.08,3981.36 6740.31,3990.69 C6735.65,3996.83 6725.83,4008.89 6720.69,4014.78 C6715.46,4020.52 6704.69,4031.57 6699.15,4036.88 C6695.91,4039.93 6681.52,4051.65 6675.36,4056.31 C6669.06,4060.86 6656.11,4069.58 6649.53,4073.69 C6642.90,4077.63 6629.48,4085.05 6622.55,4088.29 C6615.54,4091.42 6601.21,4097.25 6593.89,4099.94 C6586.50,4102.47 6571.68,4107.01 6564.25,4109.01 C6553.70,4111.60 6541.76,4113.69 6534.15,4114.82 C6526.47,4115.78 6510.95,4117.19 6503.17,4117.64 C6495.42,4117.91 6476.19,4117.89 6472.38,4117.68 Z" fill="none" stroke="rgb(0, 0, 0)" stroke-width="35.57" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M6398.16,3795.15 C6398.87,3788.40 6403.13,3772.59 6406.14,3765.54 C6409.15,3758.54 6417.59,3745.78 6422.77,3739.92 C6428.46,3733.54 6438.98,3724.96 6446.38,3720.69 C6453.17,3716.80 6467.37,3711.12 6474.84,3709.58 C6482.35,3708.07 6498.69,3707.10 6505.45,3707.77 C6512.20,3708.48 6528.01,3712.74 6535.06,3715.74 C6542.06,3718.76 6554.82,3727.20 6560.68,3732.38 C6567.06,3738.06 6575.63,3748.59 6579.90,3755.99 C6583.80,3762.78 6589.47,3776.98 6591.01,3784.45 C6592.53,3791.96 6593.50,3808.30 6592.82,3815.06 C6592.12,3821.81 6587.86,3837.62 6584.85,3844.67 C6581.83,3851.67 6573.40,3864.42 6568.21,3870.29 C6562.53,3876.67 6552.00,3885.24 6544.61,3889.51 C6537.82,3893.40 6523.62,3899.08 6516.15,3900.62 C6508.63,3902.14 6492.29,3903.11 6485.54,3902.43 C6478.79,3901.73 6462.98,3897.47 6455.93,3894.46 C6448.93,3891.44 6436.17,3883.01 6430.31,3877.82 C6423.93,3872.14 6415.36,3861.61 6411.08,3854.22 C6407.19,3847.42 6401.52,3833.23 6399.98,3825.76 C6398.46,3818.24 6397.49,3801.90 6398.16,3795.15 Z" fill="none" stroke="rgb(0, 0, 0)" stroke-width="399.89" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M2154.04,3755.05 C2154.74,3748.31 2159.00,3732.52 2162.00,3725.48 C2165.01,3718.48 2173.44,3705.74 2178.62,3699.89 C2184.29,3693.52 2194.81,3684.95 2202.20,3680.68 C2208.98,3676.80 2223.16,3671.13 2230.62,3669.59 C2238.13,3668.08 2254.45,3667.10 2261.20,3667.78 C2267.94,3668.48 2283.73,3672.74 2290.78,3675.74 C2297.77,3678.76 2310.51,3687.18 2316.36,3692.36 C2322.74,3698.04 2331.30,3708.55 2335.57,3715.94 C2339.45,3722.72 2345.12,3736.91 2346.66,3744.36 C2348.18,3751.87 2349.15,3768.19 2348.47,3774.94 C2347.77,3781.69 2343.51,3797.47 2340.51,3804.52 C2337.49,3811.51 2329.07,3824.25 2323.89,3830.11 C2318.22,3836.48 2307.70,3845.04 2300.31,3849.31 C2293.53,3853.20 2279.35,3858.87 2271.89,3860.41 C2264.38,3861.92 2248.06,3862.89 2241.31,3862.22 C2234.57,3861.51 2218.78,3857.26 2211.73,3854.25 C2204.74,3851.24 2192.00,3842.81 2186.14,3837.63 C2179.77,3831.96 2171.21,3821.44 2166.94,3814.05 C2163.05,3807.27 2157.38,3793.09 2155.85,3785.63 C2154.33,3778.12 2153.36,3761.80 2154.04,3755.05 Z" fill="none" stroke="rgb(0, 0, 0)" stroke-width="399.43" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M3496.00,2112.00 Q4756.00,2086.00 4756.00,2086.00" fill="none" stroke="rgb(3, 3, 2)" stroke-width="16.70" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M3490.00,2112.00 Q3504.00,2862.00 3504.00,2862.00" fill="none" stroke="rgb(3, 3, 2)" stroke-width="16.70" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M3504.00,2862.00 Q4640.00,2874.00 4640.00,2874.00" fill="none" stroke="rgb(3, 3, 2)" stroke-width="16.70" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M4755.82,2084.90 Q4893.31,2082.64 4894.44,2191.96" fill="none" stroke="rgb(3, 3, 2)" stroke-width="16.70" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M4640.87,2872.65 Q4833.58,2867.02 4844.85,2656.27" fill="none" stroke="rgb(3, 3, 2)" stroke-width="16.70" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M4894.21,2189.25 Q4842.14,2690.08 4842.14,2690.08" fill="none" stroke="rgb(3, 3, 2)" stroke-width="16.70" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M5257.83,2087.97 Q5116.45,2074.31 5152.65,2215.01" fill="none" stroke="rgb(3, 3, 2)" stroke-width="16.70" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M5256.47,2087.29 Q5663.54,2135.10 5663.54,2135.10" fill="none" stroke="rgb(3, 3, 2)" stroke-width="16.70" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M5660.00,2134.55 Q5750.00,2142.73 5870.91,2272.73" fill="none" stroke="rgb(3, 3, 2)" stroke-width="16.70" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M5868.59,2269.83 Q6428.92,2856.20 6441.32,2883.47" fill="none" stroke="rgb(3, 3, 2)" stroke-width="16.70" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M6441.32,2880.99 Q6498.34,2946.69 6423.96,2924.38" fill="none" stroke="rgb(3, 3, 2)" stroke-width="16.70" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M6425.20,2925.62 Q5365.28,2742.15 5365.28,2742.15" fill="none" stroke="rgb(3, 3, 2)" stroke-width="16.70" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M5366.52,2742.15 Q5249.99,2723.55 5207.85,2593.39" fill="none" stroke="rgb(3, 3, 2)" stroke-width="16.70" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M5149.58,2209.09 Q5209.09,2599.58 5209.09,2600.82" fill="none" stroke="rgb(3, 3, 2)" stroke-width="16.70" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M5741.92,2170.55 Q5816.30,2811.79 5816.30,2811.79" fill="none" stroke="rgb(3, 3, 2)" stroke-width="16.70" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M5009.97,1653.11 Q5005.02,3674.20 5002.33,3848.18" fill="none" stroke="rgb(3, 3, 2)" stroke-width="10.40" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M3499.48,1704.94 Q3467.26,2401.16 3545.06,3816.72" fill="none" stroke="rgb(0, 0, 0)" stroke-width="10.40" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M3489.01,2109.01 Q1332.00,2157.01 1332.00,2154.01" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M1332.00,2154.01 Q1104.00,2148.01 1104.00,2313.01" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M1104.00,2301.01 Q1080.00,2748.01 1077.00,2871.01" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M1074.00,2871.01 Q3528.01,2862.01 3528.01,2862.01" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M918.59,2887.19 Q1017.77,2883.47 1017.77,2883.47" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M1016.53,2888.43 Q1021.49,3164.87 1085.95,3396.69" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M971.90,3383.05 Q994.21,3399.17 1085.95,3392.97" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M7027.00,2941.00 Q7311.00,3014.00 7296.00,3359.00" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M7024.00,2939.00 Q7347.00,2959.00 7325.00,3283.00" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M7294.00,3359.00 Q7317.00,3346.00 7327.00,3273.00" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M6225.62,2657.02 Q6238.02,2795.04 6225.62,2888.43" fill="none" stroke="rgb(0, 0, 0)" stroke-width="26.80" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M6257.85,2731.40 Q6376.86,2881.82 6376.86,2881.82" fill="none" stroke="rgb(0, 0, 0)" stroke-width="84.30" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M6261.98,2861.98 Q6276.86,2790.91 6278.51,2790.08" fill="none" stroke="rgb(0, 0, 0)" stroke-width="84.30" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M6262.81,2864.46 Q6396.69,2881.82 6396.69,2881.82" fill="none" stroke="rgb(0, 0, 0)" stroke-width="84.30" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M6241.32,2683.47 Q6247.93,2703.31 6247.93,2703.31" fill="none" stroke="rgb(0, 0, 0)" stroke-width="20.50" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M6451.99,2917.71 Q6431.50,2900.32 6420.01,2902.81" fill="none" stroke="rgb(0, 0, 0)" stroke-width="20.50" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M3671.67,2290.01 L4724.26,2271.97" fill="none" stroke="rgb(0, 0, 0)" stroke-width="371.20" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M4507.89,2697.97 L3678.91,2681.35 L3664.91,2395.94 L4685.94,2405.08 L4649.88,2706.99" fill="none" stroke="rgb(0, 0, 0)" stroke-width="371.20" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M4767.27,2096.36 Q4878.18,2130.91 4878.18,2192.73" fill="none" stroke="rgb(0, 0, 0)" stroke-width="40.20" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M3500.00,2207.00 Q3513.00,2144.00 3589.00,2124.00" fill="none" stroke="rgb(0, 0, 0)" stroke-width="40.20" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M3501.00,2150.00 Q3528.00,2126.00 3528.00,2126.00" fill="none" stroke="rgb(0, 0, 0)" stroke-width="40.20" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M3508.64,2762.79 Q3525.71,2845.43 3597.43,2846.80" fill="none" stroke="rgb(0, 0, 0)" stroke-width="40.20" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M3513.42,2828.36 Q3528.44,2850.90 3528.44,2850.90" fill="none" stroke="rgb(0, 0, 0)" stroke-width="40.20" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M940.91,2902.73 L999.09,3372.73" fill="none" stroke="rgb(0, 0, 0)" stroke-width="56.30" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M988.18,2899.09 L1045.45,3373.64" fill="none" stroke="rgb(0, 0, 0)" stroke-width="56.30" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M7121.00,2968.00 Q7333.00,3060.00 7306.00,3307.00" fill="none" stroke="rgb(0, 0, 0)" stroke-width="40.20" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M1245.82,1736.56 Q1230.45,1631.04 1277.58,1628.99" fill="none" stroke="rgb(0, 0, 0)" stroke-width="40.00" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M1276.36,1629.09 Q5014.55,1505.45 5014.55,1505.45" fill="none" stroke="rgb(0, 0, 0)" stroke-width="40.00" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M5015.77,1505.63 Q5030.80,1586.78 4993.23,1621.34" fill="none" stroke="rgb(0, 0, 0)" stroke-width="40.00" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M4694.35,2965.99 Q4937.16,2971.11 4936.14,2971.11" fill="none" stroke="rgb(0, 0, 0)" stroke-width="40.00" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M5156.41,2975.21 Q5404.34,2977.25 5404.34,2977.25" fill="none" stroke="rgb(0, 0, 0)" stroke-width="40.00" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M1099.10,3403.65 Q1876.37,3428.20 1914.55,3422.74" fill="none" stroke="rgb(0, 0, 0)" stroke-width="28.60" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M2550.01,3430.92 Q6226.39,3460.92 6226.39,3460.92" fill="none" stroke="rgb(0, 0, 0)" stroke-width="28.60" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M1207.27,3574.55 Q1803.64,3592.73 1803.64,3592.73" fill="none" stroke="rgb(0, 0, 0)" stroke-width="28.60" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M2669.09,3603.64 Q6065.45,3636.36 6065.45,3636.36" fill="none" stroke="rgb(0, 0, 0)" stroke-width="28.60" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M1096.24,3401.40 Q1071.65,3564.64 1209.62,3574.21" fill="none" stroke="rgb(0, 0, 0)" stroke-width="28.60" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M5884.49,3891.00 Q5912.99,3621.00 6057.74,3439.50 Q6202.49,3258.00 6461.24,3240.00 Q6719.99,3222.00 6967.49,3454.50" fill="none" stroke="rgb(0, 0, 0)" stroke-width="28.60" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M6815.99,3484.50 Q6965.99,3454.50 6965.99,3454.50" fill="none" stroke="rgb(0, 0, 0)" stroke-width="28.60" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M6965.99,3454.50 Q7318.49,3472.50 7351.49,3394.50" fill="none" stroke="rgb(0, 0, 0)" stroke-width="28.60" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M6921.80,3664.08 Q7244.98,3655.90 7244.98,3655.90" fill="none" stroke="rgb(0, 0, 0)" stroke-width="28.60" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M5884.49,3891.00 Q5912.99,3621.00 6057.74,3439.50 Q6202.49,3258.00 6461.24,3240.00 Q6719.99,3222.00 6967.49,3454.50" fill="none" stroke="rgb(0, 0, 0)" stroke-width="28.60" stroke-opacity="1.00" stroke-linejoin="round"/>
<path d="M2857.69,3805.65 Q2817.03,3537.21 2664.23,3362.44 Q2511.43,3187.66 2252.13,3181.36 Q1992.83,3175.07 1756.09,3418.51" fill="none" stroke="rgb(0, 0, 0)" stroke-width="28.60" stroke-opacity="1.00" stroke-linejoin="round"/>
</g>
</svg></template>

<script>
"use strict";

const HOST = (location.hostname && location.protocol !== "file:")
  ? location.hostname + (location.port ? ":" + location.port : "")
  : "192.168.4.1";
const BASE = "http://" + HOST;

// Inject SVGs into correct slots
document.getElementById('svg-s-slot').innerHTML = document.getElementById('crafter-top').innerHTML;
document.getElementById('svg-f-slot').innerHTML = document.getElementById('crafter-side').innerHTML;

let _roll = 0, _pitch = 0;

// =========================================
// LOGGING SYSTEM (localStorage based)
// =========================================
const LOG_KEY = 'kemperis_logs_v1';
const LOG_MAX = 10000;
const LOG_THROTTLE_MS = 2000;
let _logs = [];
let _logsPaused = false;
let _lastLogTime = {};
let _logFilter = '';

function logsLoad() {
  try {
    const raw = localStorage.getItem(LOG_KEY);
    if (raw) _logs = JSON.parse(raw);
  } catch (e) { console.warn('Log load failed:', e); _logs = []; }
}

let _saveTimer = null;
function logsSaveDeferred() {
  if (_saveTimer) return;
  _saveTimer = setTimeout(() => {
    _saveTimer = null;
    try { localStorage.setItem(LOG_KEY, JSON.stringify(_logs)); }
    catch (e) {
      console.warn('Log save failed:', e);
      _logs.splice(0, Math.floor(_logs.length * 0.25));
      try { localStorage.setItem(LOG_KEY, JSON.stringify(_logs)); } catch(e2){}
    }
  }, 5000);
}

function logsAdd(id, value, type) {
  if (_logsPaused) return;
  const now = Date.now();
  if (_lastLogTime[id] && now - _lastLogTime[id] < LOG_THROTTLE_MS) return;
  _lastLogTime[id] = now;
  _logs.push({ t: now, i: id, v: value, k: type });
  if (_logs.length > LOG_MAX) _logs.shift();
  logsSaveDeferred();
  logsUpdateUI();
}

function logsClear() {
  if (!confirm('Tikrai išvalyti visus logus?')) return;
  _logs = [];
  _lastLogTime = {};
  try { localStorage.removeItem(LOG_KEY); } catch (e) {}
  logsUpdateUI();
}

function logsUpdateUI() {
  const c = _logs.length;
  document.getElementById('log-count').textContent = c.toLocaleString('lt');
  document.getElementById('chip-logs').textContent = '📊 ' + c.toLocaleString('lt');
  const bytes = JSON.stringify(_logs).length;
  document.getElementById('log-size').textContent = bytes < 1024 ? bytes + ' B' :
    bytes < 1024*1024 ? (bytes/1024).toFixed(1) + ' KB' :
    (bytes/1024/1024).toFixed(2) + ' MB';
  if (c > 0) {
    const d = new Date(_logs[0].t);
    document.getElementById('log-start').textContent =
      d.toLocaleDateString('lt') + ' ' + d.toLocaleTimeString('lt', { hour: '2-digit', minute: '2-digit' });
  } else {
    document.getElementById('log-start').textContent = '—';
  }
  logsRenderList();
}

function logsRenderList() {
  const list = document.getElementById('log-list');
  if (!list) return;
  let toShow = _logs;
  if (_logFilter) {
    const f = _logFilter.toLowerCase();
    toShow = _logs.filter(l => (l.i + ' ' + l.v).toLowerCase().includes(f));
  }
  toShow = toShow.slice(-200).reverse();
  if (toShow.length === 0) {
    list.innerHTML = '<div class="log-empty">' +
      (_logs.length === 0 ? 'Logų sistema veikia. Įrašai pildomi automatiškai realiu laiku.' : 'Nieko nerasta pagal filtrą') +
      '</div>';
    return;
  }
  let h = '';
  for (const l of toShow) {
    const d = new Date(l.t);
    const ts = d.toLocaleTimeString('lt', { hour: '2-digit', minute: '2-digit', second: '2-digit' });
    const cls = l.k === 'text_sensor' ? 'txt' : (l.k === 'binary_sensor' ? 'bin' : '');
    h += '<div class="log-entry ' + cls + '"><span class="ts">' + ts + '</span><span class="id">' +
         l.i + '</span><span class="v">' + String(l.v).slice(0, 80) + '</span></div>';
  }
  list.innerHTML = h;
}

function logsExportCSV() {
  const rows = ['timestamp,iso_time,entity_id,kind,value'];
  for (const l of _logs) {
    const d = new Date(l.t);
    const v = String(l.v).replace(/"/g, '""');
    rows.push(`${l.t},${d.toISOString()},${l.i},${l.k || ''},"${v}"`);
  }
  const blob = new Blob([rows.join('\n')], { type: 'text/csv;charset=utf-8' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = 'kemperis-logs-' + new Date().toISOString().slice(0, 19).replace(/[:T]/g, '-') + '.csv';
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  setTimeout(() => URL.revokeObjectURL(url), 1000);
}

function logsExportJSON() {
  const blob = new Blob([JSON.stringify(_logs, null, 1)], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = 'kemperis-logs-' + new Date().toISOString().slice(0, 19).replace(/[:T]/g, '-') + '.json';
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  setTimeout(() => URL.revokeObjectURL(url), 1000);
}

document.getElementById('btn-export-csv').addEventListener('click', logsExportCSV);
document.getElementById('btn-export-json').addEventListener('click', logsExportJSON);
document.getElementById('btn-clear').addEventListener('click', logsClear);
document.getElementById('btn-pause').addEventListener('click', () => {
  _logsPaused = !_logsPaused;
  const b = document.getElementById('btn-pause');
  b.textContent = _logsPaused ? '▶ Tęsti' : '⏸ Pauzė';
  b.classList.toggle('warn', _logsPaused);
});
document.getElementById('log-filter').addEventListener('input', e => {
  _logFilter = e.target.value;
  logsRenderList();
});

// =========================================
// TAB SWITCHING
// =========================================
document.querySelectorAll('.tab').forEach(t => {
  t.addEventListener('click', () => {
    const id = t.dataset.tab;
    document.querySelectorAll('.tab').forEach(x => x.classList.remove('active'));
    document.querySelectorAll('.page').forEach(x => x.classList.remove('active'));
    t.classList.add('active');
    document.getElementById(id).classList.add('active');
    if (id === 'log') logsRenderList();
  });
});

// =========================================
// HELPERS
// =========================================
function tiltColor(v) {
  const a = Math.abs(v);
  const hue = Math.max(0, 120 - a * 15);
  return "hsl(" + hue + ",80%,55%)";
}

function setTilt(which, v) {
  const c = tiltColor(v);
  const rot = document.getElementById('rot-' + which);
  const ang = document.getElementById('ang-' + which);
  const ndl = document.getElementById('ndl-' + which);
  const dir = document.getElementById('dir-' + which);
  if (rot) rot.style.transform = "translateX(-50%) rotate(" + v + "deg)";
  if (ang) { ang.textContent = Math.abs(v).toFixed(1) + "°"; ang.style.color = c; }
  if (ndl) { ndl.style.background = c; ndl.style.boxShadow = "0 0 10px " + c; }
  if (dir) {
    if (which === 's') dir.textContent = v > 0.3 ? "Kairė žemiau" : (v < -0.3 ? "Dešinė žemiau" : "Lygu");
    else               dir.textContent = v > 0.3 ? "Priekis žemiau" : (v < -0.3 ? "Galas žemiau" : "Lygu");
  }
}

function setBubble() {
  const b = document.getElementById('bubble');
  if (!b) return;
  const x = Math.max(5, Math.min(83, 44 + _roll * 3));
  const y = Math.max(5, Math.min(83, 44 + _pitch * 3));
  b.style.left = x + "px";
  b.style.top = y + "px";
}

function fmtUptime(sec) {
  sec = parseInt(sec);
  if (isNaN(sec)) return "—";
  const d = Math.floor(sec / 86400);
  const h = Math.floor((sec % 86400) / 3600);
  const m = Math.floor((sec % 3600) / 60);
  if (d > 0) return d + "d " + h + "h";
  if (h > 0) return h + "h " + m + "m";
  return m + "m";
}

function battColor(p) {
  if (p < 20) return 'var(--red)';
  if (p < 50) return 'var(--yellow)';
  return 'var(--green)';
}

function weatherIcon(t) {
  if (!t) return '⛅';
  const s = t.toLowerCase();
  if (s.includes('audra')) return '⛈️';
  if (s.includes('bloge') || s.includes('lietus')) return '🌧️';
  if (s.includes('sparci') || s.includes('saule')) return '☀️';
  if (s.includes('gere')) return '🌤️';
  if (s.includes('stabil')) return '⛅';
  if (s.includes('kaupia') || s.includes('krau')) return '⏳';
  return '⛅';
}

function setSigBars(p) {
  const n = Math.min(5, Math.floor(p / 20));
  document.querySelectorAll('#sig-bars span').forEach((b, i) => b.classList.toggle('on', i < n));
}

function setHTML(id, val, unit) {
  const e = document.getElementById(id);
  if (!e) return;
  e.innerHTML = val + (unit ? ' <span class="stat-u">' + unit + '</span>' : '');
}

function isOn(v) {
  return v === true || v === 'true' || v === 'ON' || v === 'on' || v === '1' || v === 1;
}

// =========================================
// ENTITY HANDLERS (angliški ID)
// =========================================
const H = {
  "sensor-roll_sensor":       v => { _roll = parseFloat(v); setTilt('s', _roll); setBubble(); },
  "sensor-pitch_sensor":      v => { _pitch = parseFloat(v); setTilt('f', _pitch); setBubble(); },
  "sensor-water_cm":          v => { document.getElementById('v-water-cm').textContent = parseFloat(v).toFixed(1) + ' cm'; },
  "sensor-water_pct":         v => {
    const p = parseFloat(v);
    document.getElementById('v-water-pct').textContent = p.toFixed(0) + '%';
    document.getElementById('tank-fill').style.height = p + '%';
  },
  "sensor-water_tds":         v => { document.getElementById('v-water-tds').textContent = parseFloat(v).toFixed(0) + ' PPM'; },
  "sensor-gas_total":         v => { document.getElementById('v-gas-total').textContent = parseFloat(v).toFixed(2) + ' kg'; },
  "sensor-gas_net":           v => {
    const kg = parseFloat(v);
    document.getElementById('v-gas-net').textContent = kg.toFixed(2) + ' kg';
    const pct = Math.max(0, Math.min(100, (kg / 11) * 100));
    document.getElementById('gas-fill').style.height = pct + '%';
    document.getElementById('v-gas-pct').textContent = pct.toFixed(0) + '%';
  },
  "sensor-cabin_temp":        v => setHTML('v-cabin-temp', parseFloat(v).toFixed(1), '°C'),
  "sensor-cabin_pres":        v => setHTML('v-pressure', parseFloat(v).toFixed(1), 'hPa'),
  "sensor-altitude_baro":     v => setHTML('v-altitude-baro', parseFloat(v).toFixed(1), 'm'),
  "sensor-pressure_trend":    v => {
    const x = parseFloat(v);
    const e = document.getElementById('v-pressure-trend');
    if (isNaN(x)) { e.textContent = 'Tendencija: kaupiama...'; return; }
    const arrow = x > 0.5 ? '↑' : (x < -0.5 ? '↓' : '→');
    e.textContent = "Tendencija: " + arrow + " " + (x > 0 ? '+' : '') + x.toFixed(2) + " hPa / 3h";
  },
  "sensor-sea_level_pressure": v => {
    document.getElementById('v-qnh').textContent = parseFloat(v).toFixed(2) + ' hPa';
    updNum('sea_level_pressure', v);
  },
  "sensor-esp_temp":          v => { document.getElementById('v-esp-temp').textContent = parseFloat(v).toFixed(1) + ' °C'; },
  "sensor-uptime_sensor":     v => {
    const t = fmtUptime(v);
    document.getElementById('v-uptime').textContent = t;
    document.getElementById('chip-uptime').textContent = '⏱ ' + t;
  },
  "text_sensor-gsm_op":       v => { document.getElementById('v-gsm-op').textContent = v; },
  "sensor-gsm_signal":        v => {
    const p = parseFloat(v);
    document.getElementById('v-gsm-signal').textContent = p.toFixed(0) + '%';
    setSigBars(p);
  },
  "sensor-gps_alt_sensor":    v => {
    const m = parseFloat(v);
    if (isNaN(m)) {
      document.getElementById('v-gps-alt').textContent = '— m';
      document.getElementById('v-gps-alt2').textContent = '— m';
    } else {
      document.getElementById('v-gps-alt').textContent = m.toFixed(1) + ' m';
      document.getElementById('v-gps-alt2').textContent = m.toFixed(1) + ' m';
    }
  },
  "sensor-gps_speed":         v => setHTML('v-gps-speed', parseFloat(v).toFixed(0), 'km/h'),
  "sensor-gps_course":        v => {
    const c = parseFloat(v);
    document.getElementById('v-gps-course').textContent = isNaN(c) ? '—°' : c.toFixed(0) + '°';
  },
  "sensor-gps_sats":          v => { document.getElementById('v-gps-sats').textContent = parseInt(v) + ' vnt'; },
  "sensor-odometer":          v => setHTML('v-odometer', parseFloat(v).toFixed(2), 'km'),
  "text_sensor-gps_coords":   v => { document.getElementById('v-gps-coords').textContent = v; },
  "text_sensor-weather_forecast": v => {
    document.getElementById('v-weather').textContent = v;
    document.getElementById('v-weather-ico').textContent = weatherIcon(v);
  },
  "binary_sensor-power_220v": v => {
    const on = isOn(v);
    const big = document.getElementById('status-220v');
    const chip = document.getElementById('chip-220v');
    big.textContent = on ? '✓ 220V PRIJUNGTAS' : '220V nėra';
    big.className = 'big-status ' + (on ? 'on' : 'off');
    chip.textContent = on ? '220V ON' : '220V OFF';
    chip.className = 'chip ' + (on ? 'green' : 'dim');
  },
  "binary_sensor-gps_fix_sensor": v => {
    const on = isOn(v);
    document.getElementById('gps-dot').className = 'gps-dot' + (on ? ' fix' : '');
    const pill = document.getElementById('v-gps-fix');
    pill.textContent = on ? 'Prisijungta' : 'Ieškoma...';
    pill.className = 'chip ' + (on ? 'green' : 'yellow');
  },
  "number-tank_fresh_empty":   v => updNum('tank_fresh_empty', v),
  "number-tank_fresh_full":    v => updNum('tank_fresh_full', v),
  "number-cylinder_tare":      v => updNum('cylinder_tare', v),
  "number-temp_offset":        v => updNum('temp_offset', v),
  "number-gas_ref_weight":     v => updNum('gas_ref_weight', v),
};

function updNum(name, val) {
  const row = document.querySelector('[data-num="' + name + '"]');
  if (!row) return;
  const step = parseFloat(row.dataset.step);
  const dec = step < 1 ? 1 : 0;
  const u = row.dataset.unit;
  const ve = row.querySelector('.num-val');
  ve.innerHTML = parseFloat(val).toFixed(dec) + '<span class="num-u">' + u + '</span>';
  row.dataset.cur = val;
}

// =========================================
// CONTROLS
// =========================================
document.querySelectorAll('.num-btn').forEach(btn => {
  btn.addEventListener('click', () => {
    const row = btn.closest('[data-num]');
    if (!row) return;
    const name = row.dataset.num;
    const step = parseFloat(row.dataset.step);
    const min = parseFloat(row.dataset.min);
    const max = parseFloat(row.dataset.max);
    const dir = parseFloat(btn.dataset.stepDir);
    const cur = parseFloat(row.dataset.cur || '0');
    let next = cur + dir * step;
    next = Math.max(min, Math.min(max, next));
    next = Math.round(next / step) * step;
    fetch(BASE + '/number/' + name + '/set?value=' + next, { method: 'POST' })
      .catch(e => console.warn('num set fail', e));
  });
});

document.querySelectorAll('[data-press]').forEach(btn => {
  btn.addEventListener('click', () => {
    const name = btn.dataset.press;
    fetch(BASE + '/button/' + name + '/press', { method: 'POST' })
      .then(() => {
        const orig = btn.style.background;
        btn.style.background = 'var(--accent)';
        setTimeout(() => { btn.style.background = orig; }, 200);
      })
      .catch(e => console.warn('btn fail', e));
  });
});

// =========================================
// SSE CONNECTION
// =========================================
let evt;
const seenIds = new Set();

let _diagInfo = { url: BASE, attempts: 0, lastError: null, mode: 'detecting' };

function showDiag(msg, isErr) {
  const txt = document.getElementById('conn-txt');
  if (txt) txt.textContent = msg;
  const dot = document.getElementById('conn-dot');
  if (dot) dot.className = 'dot' + (isErr ? '' : ' ok');
  console.log('[KEMPERIS]', msg, _diagInfo);
}

async function detectMode() {
  try {
    const r = await fetch(BASE + '/', { method: 'GET', mode: 'cors' });
    if (r.ok) {
      _diagInfo.mode = 'cors-ok';
      showDiag('ESP pasiekiamas, jungiamasi prie /events...', false);
      return true;
    } else {
      _diagInfo.mode = 'http-error';
      _diagInfo.lastError = 'HTTP ' + r.status;
      showDiag('ESP atsako klaida: HTTP ' + r.status, true);
    }
  } catch (e) {
    _diagInfo.lastError = String(e);
    if (e.message && e.message.includes('CORS')) {
      _diagInfo.mode = 'cors-blocked';
      showDiag('CORS blokuoja - atidaryk per http://192.168.4.1/ vietoj file://', true);
    } else if (e.message && (e.message.includes('Failed to fetch') || e.message.includes('NetworkError'))) {
      _diagInfo.mode = 'network';
      showDiag('Nepasiekiamas ' + BASE + ' - tikrink WiFi prisijungima', true);
    } else {
      _diagInfo.mode = 'unknown';
      showDiag('Klaida: ' + e.message, true);
    }
  }
  return false;
}

function connect() {
  _diagInfo.attempts++;
  showDiag('Bandymas #' + _diagInfo.attempts + '...', true);

  try {
    evt = new EventSource(BASE + '/events');
  } catch (e) {
    _diagInfo.lastError = String(e);
    showDiag('EventSource klaida: ' + e.message, true);
    setTimeout(connect, 5000);
    return;
  }

  evt.onopen = () => {
    _diagInfo.mode = 'connected';
    showDiag('✓ Prisijungta', false);
  };

  let _rawEventCount = 0;
  evt.addEventListener('state', e => {
    try {
      const d = JSON.parse(e.data);
      const id = d.id;

      if (_rawEventCount < 30) {
        _rawEventCount++;
        console.log('[RAW SSE #' + _rawEventCount + ']', e.data);
        logsAdd('__raw__-' + id, JSON.stringify(d).substring(0, 200), 'debug');
      }

      let v = d.value !== undefined ? d.value : (d.state !== undefined ? d.state : d);
      let kind = 'sensor';
      if (id.startsWith('text_sensor-')) kind = 'text_sensor';
      else if (id.startsWith('binary_sensor-')) kind = 'binary_sensor';
      else if (id.startsWith('number-')) kind = 'number';

      logsAdd(id, v, kind);

      const handler = H[id];
      if (handler) handler(v);
      else if (!seenIds.has(id)) { console.log('[unhandled]', id, v); seenIds.add(id); }

      const dot = document.getElementById('conn-dot');
      const txt = document.getElementById('conn-txt');
      if (dot) dot.className = 'dot ok';
      if (txt) txt.textContent = '✓ Prisijungta';
    } catch (err) { console.warn('SSE parse error:', err, 'data:', e.data); }
  });

  evt.addEventListener('ping', e => {
    const dot = document.getElementById('conn-dot');
    if (dot) dot.className = 'dot ok';
  });

  evt.onerror = (e) => {
    _diagInfo.lastError = 'SSE error event';
    const state = evt ? evt.readyState : -1;
    let stateName = ['CONNECTING','OPEN','CLOSED'][state] || 'unknown';
    showDiag('Atsijungta (' + stateName + ') - bandoma...', true);
    try { evt.close(); } catch (e) {}
    setTimeout(connect, 3000);
  };
}

if (location.protocol === 'file:') {
  showDiag('⚠ file:// - jungimasis gali nepavykti (CORS)', true);
}
detectMode().then(() => connect());

logsLoad();
logsUpdateUI();

setInterval(() => { if (document.getElementById('log').classList.contains('active')) logsRenderList(); }, 30000);
</script>
</body>
</html>
)KEMPERIS";

static const size_t KEMPERIS_DASHBOARD_HTML_SIZE = sizeof(KEMPERIS_DASHBOARD_HTML) - 1;