// OBD/ELM327 Bluetooth Classic (SPP) Implementation for Kemperis App
// Transportas: native KemperisObdBridge (BluetoothSocket/RFCOMM per MainActivity.java),
// NE @capacitor-community/bluetooth-le (tas naudojamas tik Renogy BLE, čia netinka).
// Žr. docs/uzduotis_obd2_elm327_tab.md — Fazė 1 (MVP): prisijungimas, poravimas,
// Mode 0 (bazinis) + Pakopa A (standartiniai išplėstiniai) PID, dedikuotas OBD žurnalas.

const ObdBLE = (function() {
    const STATE = {
        enabled: false,
        scanning: false,
        devices: {},        // mac -> {mac, name, rssi, paired}
        connectedMac: null,
        connectedName: null,
        connected: false,
        connecting: false,
        pending: null,       // {cmd, sentAt, resolve, timer}
        queue: [],
        pollTimer: null
    };

    // Pakopa 0 — bazinis Mode 01 (visada veikia, žr. research/obd2/ESPobd/src/main.cpp)
    const PID0 = [
        { pid: '0C', key: 'obd_rpm',     fmt: (A, B) => (A * 256 + B) / 4 },
        { pid: '0D', key: 'obd_speed',   fmt: (A)    => A },
        { pid: '05', key: 'obd_coolant', fmt: (A)    => A - 40 },
        { pid: '04', key: 'obd_load',    fmt: (A)    => A * 100 / 255 },
        { pid: '0B', key: 'obd_map',     fmt: (A)    => A },
        { pid: '10', key: 'obd_maf',     fmt: (A, B) => (A * 256 + B) / 100 }
    ];
    // Pakopa A — standartiniai SAE J1979 išplėstiniai (ne gamintojo-specifiniai, bandyti be tyrimo)
    const PID_A = [
        { pid: '23', key: 'obd_rail_p',  fmt: (A, B) => (A * 256 + B) * 10 },
        { pid: '5C', key: 'obd_oil_t',   fmt: (A)    => A - 40 },
        { pid: '2C', key: 'obd_egr_cmd', fmt: (A)    => A * 100 / 255 },
        { pid: '2D', key: 'obd_egr_err', fmt: (A)    => (A - 128) * 100 / 128 },
        { pid: '7A', key: 'obd_dpf_p',   fmt: (A, B) => (A * 256 + B) / 100 },
        { pid: '7C', key: 'obd_dpf_t',   fmt: (A, B) => (A * 256 + B) / 10 - 40 }
    ];
    const ALL_PIDS = PID0.concat(PID_A);

    // Pakopa B — Mode 22 (UDS) DID kandidatai variklio adresui (0x7E0). NEVERIFIKUOTI —
    // testavimo kandidatai iš kelių LLM pasiūlymų + standartizuoti F1xx (žr. docs/uzduotis_obd2_elm327_tab.md).
    const DID_CANDIDATES = [
        { did: 'F190', label: 'VIN (sanity-check, standartizuota)' },
        { did: 'F18C', label: 'ECU serijos nr. (standartizuota)' },
        { did: 'F19F', label: 'SW versija (standartizuota)' },
        { did: '114C', label: 'DPF suodziai (Gemini kandidatas)' },
        { did: '111F', label: 'Alyvos temp (Gemini kandidatas)' },
        { did: '1310', label: 'Alyvos temp (XGauge kandidatas)' },
        { did: '116B', label: 'Turbo praeasomas (Gemini kandidatas)' },
        { did: '116C', label: 'Turbo faktinis (Gemini kandidatas)' },
        { did: '1193', label: 'Rail slegis (Gemini kandidatas)' },
        { did: '12A0', label: 'Purkstukas 1 korekcija (Gemini)' },
        { did: '12A1', label: 'Purkstukas 2 korekcija (Gemini)' },
        { did: '12A2', label: 'Purkstukas 3 korekcija (Gemini)' },
        { did: '12A3', label: 'Purkstukas 4 korekcija (Gemini)' },
        { did: '12A4', label: 'Purkstukas 5 korekcija (Gemini)' },
        { did: '0380', label: 'Visu purkstuku korekcijos (2-as kandidatas)' },
        { did: '0381', label: 'Purkstukas 1 korekcija (2-as kandidatas)' },
        { did: '0382', label: 'Purkstukas 2 korekcija (2-as kandidatas)' },
        { did: '0383', label: 'Purkstukas 3 korekcija (2-as kandidatas)' },
        { did: '0384', label: 'Purkstukas 4 korekcija (2-as kandidatas)' },
        { did: '0385', label: 'Purkstukas 5 korekcija (2-as kandidatas)' },
        { did: '0370', label: 'Smooth running / RPM deviation' },
        { did: '0108', label: 'Fuel injection quantity' },
        { did: '0390', label: 'Ismoktos purkstuku vertes (adaptacija)' }
    ];

    function getBridge() { return window.KemperisObd || null; }

    // --- OBD-only dedikuotas žurnalas (atskiras nuo bendro app sysLog, žr. §4) ---

    function obdLog(msg, type) {
        type = type || 'info';
        const out = document.getElementById('obd-log-output');
        if (!out) return;
        const now = new Date().toLocaleTimeString('lt-LT');
        let color = '#c9d1d9';
        if (type === 'error') color = '#ff7b72';
        else if (type === 'success') color = '#3fb950';
        else if (type === 'warn') color = '#d29922';
        else if (type === 'data') color = '#58a6ff';
        const esc = window.escapeHtml || function(s) { return s; };
        out.innerHTML += '<div style="color:' + color + '">[' + now + '] ' + esc(msg) + '</div>';
        out.scrollTop = out.scrollHeight;
        if (out.children.length > 800) out.removeChild(out.children[0]);
    }

    function clearObdLog() {
        const out = document.getElementById('obd-log-output');
        if (out) out.innerHTML = '';
        obdLog('OBD žurnalas išvalytas.', 'warn');
    }

    function saveObdLog() {
        const out = document.getElementById('obd-log-output');
        if (!out) return;
        const content = out.innerText;
        const filename = 'obd_log_' + new Date().toISOString().slice(0, 19).replace(/:/g, '-') + '.txt';
        if (window.KemperisFile) {
            const result = KemperisFile.saveTextFile(filename, 'text/plain', content);
            obdLog(result === 'ok' ? ('Žurnalas išsaugotas į Downloads/' + filename + '.') : ('Klaida: ' + result), result === 'ok' ? 'success' : 'error');
        } else {
            const blob = new Blob([content], { type: 'text/plain' });
            const a = document.createElement('a');
            a.href = URL.createObjectURL(blob); a.download = filename; a.click();
            URL.revokeObjectURL(a.href);
        }
    }

    // --- Komandų eilė: viena komanda -> vienas atsakymas vienu metu (§3 buferio apribojimas) ---

    function sendCmd(cmd, timeoutMs) {
        timeoutMs = timeoutMs || 2000;
        return new Promise((resolve) => {
            STATE.queue.push({ cmd: cmd, resolve: resolve, timeoutMs: timeoutMs });
            pumpQueue();
        });
    }

    function pumpQueue() {
        if (STATE.pending || STATE.queue.length === 0) return;
        if (!STATE.connected) { STATE.queue = []; return; }
        const item = STATE.queue.shift();
        const bridge = getBridge();
        if (!bridge) { item.resolve(null); return; }
        STATE.pending = { cmd: item.cmd, resolve: item.resolve, sentAt: Date.now() };
        obdLog('TX: ' + item.cmd);
        bridge.send(item.cmd);
        STATE.pending.timer = setTimeout(() => {
            if (STATE.pending && STATE.pending.cmd === item.cmd) {
                obdLog('RX: (timeout, be atsakymo)', 'warn');
                const p = STATE.pending; STATE.pending = null;
                p.resolve(null);
                pumpQueue();
            }
        }, item.timeoutMs);
    }

    function onDataLine(line) {
        if (!STATE.pending) { obdLog('RX (nelaukta): ' + line, 'warn'); return; }
        const p = STATE.pending;
        clearTimeout(p.timer);
        STATE.pending = null;
        const dt = Date.now() - p.sentAt;
        obdLog('RX: ' + line.replace(/[\r\n]+/g, ' ') + ' (Δ' + dt + 'ms)', 'data');
        p.resolve(line);
        pumpQueue();
    }

    // --- Parsinimas ---

    function parseHexBytes(line) {
        const clean = line.replace(/[\r\n]/g, ' ').trim();
        const matches = clean.match(/\b[0-9A-Fa-f]{2}\b/g);
        return matches ? matches.map(h => parseInt(h, 16)) : [];
    }

    function isNoData(line) {
        return /NO DATA|UNABLE TO CONNECT|ERROR|STOPPED|CAN ERROR|\?/i.test(line);
    }

    // --- ELM327 init + polling ---

    async function initSequence() {
        obdLog('ELM327 inicijavimas...', 'info');
        await sendCmd('ATZ', 3000);
        await sendCmd('ATE0');
        await sendCmd('ATL0');
        await sendCmd('ATH0');
        await sendCmd('ATSP6');
        obdLog('Inicijavimas baigtas, pradedamas PID skaitymas.', 'success');
        startPolling();
    }

    function startPolling() {
        stopPolling();
        let idx = 0;
        STATE.pollTimer = setInterval(async () => {
            if (!STATE.connected || ALL_PIDS.length === 0) return;
            const def = ALL_PIDS[idx % ALL_PIDS.length];
            idx++;
            const resp = await sendCmd('01' + def.pid);
            if (!resp || isNoData(resp)) return;
            const bytes = parseHexBytes(resp);
            const pidByte = parseInt(def.pid, 16);
            let hdrIdx = -1;
            for (let i = 0; i < bytes.length - 1; i++) {
                if (bytes[i] === 0x41 && bytes[i + 1] === pidByte) { hdrIdx = i; break; }
            }
            if (hdrIdx === -1) return;
            const A = bytes[hdrIdx + 2], B = bytes[hdrIdx + 3];
            if (A === undefined) return;
            const value = def.fmt(A, B);
            if (window.sensorCache) window.sensorCache[def.key] = value;
            if (window.updateUI) window.updateUI();
        }, 600);
    }

    function stopPolling() {
        if (STATE.pollTimer) clearInterval(STATE.pollTimer);
        STATE.pollTimer = null;
    }

    // --- Prisijungimas / poravimas / skenavimas ---

    function init() {
        STATE.enabled = localStorage.getItem('obd_enabled') !== 'false';
        const savedMac = localStorage.getItem('obd_dev_mac');
        const savedName = localStorage.getItem('obd_dev_name');
        if (savedMac) { STATE.connectedMac = savedMac; STATE.connectedName = savedName || ''; }
        renderAssigned();
        if (STATE.enabled && STATE.connectedMac) connectSaved();
    }

    function connectSaved() {
        const bridge = getBridge();
        if (!bridge || !STATE.connectedMac) return;
        STATE.connecting = true;
        renderAssigned();
        obdLog('Jungiamasi prie ' + STATE.connectedMac + '...', 'info');
        bridge.connect(STATE.connectedMac);
    }

    function startScan() {
        const bridge = getBridge();
        if (!bridge) { obdLog('KemperisObd tiltelis nerastas.', 'error'); return; }
        STATE.devices = {};
        STATE.scanning = true;
        try {
            const paired = JSON.parse(bridge.listPairedDevices() || '[]');
            paired.forEach(d => { STATE.devices[d.mac] = { mac: d.mac, name: d.name, paired: true }; });
        } catch (e) {}
        renderScanList();
        bridge.startDiscovery();
        setTimeout(() => {
            bridge.stopDiscovery();
            STATE.scanning = false;
            renderScanList();
        }, 12000);
    }

    function pairAndConnect(mac) {
        const bridge = getBridge();
        if (!bridge) return;
        const dev = STATE.devices[mac];
        if (dev && dev.paired) {
            STATE.connectedMac = mac;
            STATE.connectedName = dev.name || '';
            localStorage.setItem('obd_dev_mac', mac);
            localStorage.setItem('obd_dev_name', STATE.connectedName);
            renderAssigned();
            connectSaved();
        } else {
            obdLog('Poruojama su ' + mac + '...', 'info');
            bridge.pair(mac);
        }
    }

    function toggleGlobal(on) {
        STATE.enabled = on;
        localStorage.setItem('obd_enabled', on);
        if (on) {
            if (STATE.connectedMac) connectSaved();
        } else {
            stopPolling();
            const bridge = getBridge();
            if (bridge) bridge.disconnect();
            STATE.connected = false;
            renderAssigned();
        }
    }

    function forget() {
        stopPolling();
        const bridge = getBridge();
        if (bridge) bridge.disconnect();
        STATE.connected = false;
        STATE.connectedMac = null;
        STATE.connectedName = null;
        localStorage.removeItem('obd_dev_mac');
        localStorage.removeItem('obd_dev_name');
        renderAssigned();
    }

    // --- UI atvaizdavimas ---

    function renderScanList() {
        const container = document.getElementById('obd-scan-list');
        if (!container) return;
        const items = Object.values(STATE.devices);
        if (items.length === 0) {
            container.innerHTML = STATE.scanning
                ? '<div style="color:#8b949e;padding:10px;">🔍 Skenuojama...</div>'
                : '<div style="color:#8b949e;padding:10px;">Nieko nerasta.</div>';
            return;
        }
        container.innerHTML = items.map(d => `
            <div class="card" style="margin-bottom:8px; padding:12px; background:rgba(255,255,255,0.05);">
                <div style="display:flex; justify-content:space-between; align-items:center;">
                    <div>
                        <div style="font-weight:bold;">${d.name || 'Nežinomas'} ${d.paired ? '(Susietas)' : ''}</div>
                        <div style="font-size:11px; color:#8b949e;">${d.mac}</div>
                    </div>
                    <button class="btn btn-primary" style="width:auto; padding:6px 10px; font-size:11px;" onclick="ObdBLE.pairAndConnect('${d.mac}')">${d.paired ? 'Prisijungti' : 'Suporuoti'}</button>
                </div>
            </div>
        `).join('');
    }

    function renderAssigned() {
        const el = document.getElementById('obd-assigned');
        if (el) el.textContent = STATE.connectedMac ? (STATE.connectedName || STATE.connectedMac) : '---';
        const statusEl = document.getElementById('obd-status');
        if (statusEl) statusEl.textContent = STATE.connected ? '🟢 Prijungta' : (STATE.connecting ? '🟡 Jungiamasi...' : '⚪ Atjungta');
    }

    // --- Native callback'ai (kviečiami iš KemperisObdBridge.java per evaluateJavascript) ---

    window.onObdDeviceFound = function(mac, name, rssi) {
        STATE.devices[mac] = Object.assign(STATE.devices[mac] || {}, { mac: mac, name: name, rssi: rssi });
        renderScanList();
    };
    window.onObdPaired = function(mac, name) {
        obdLog('Suporuota: ' + (name || mac), 'success');
        STATE.connectedMac = mac;
        STATE.connectedName = name;
        localStorage.setItem('obd_dev_mac', mac);
        localStorage.setItem('obd_dev_name', name || '');
        renderAssigned();
        connectSaved();
    };
    window.onObdPairFailed = function(mac, reason) {
        obdLog('Poravimo klaida (' + mac + '): ' + reason, 'error');
        STATE.connecting = false;
        renderAssigned();
    };
    window.onObdConnected = function() {
        STATE.connected = true;
        STATE.connecting = false;
        obdLog('Prijungta prie ELM327.', 'success');
        renderAssigned();
        initSequence();
    };
    window.onObdData = function(line) {
        onDataLine(line);
    };
    window.onObdDisconnected = function(reason) {
        if (STATE.connected) obdLog('Atsijungta: ' + (reason || ''), 'warn');
        STATE.connected = false;
        STATE.connecting = false;
        stopPolling();
        STATE.pending = null;
        STATE.queue = [];
        renderAssigned();
    };

    // --- Pakopa B/C/C+ — gilus skenavimas (žr. docs/uzduotis_obd2_elm327_tab.md) ---

    function decodeAsciiFromBytes(bytes, skipCount) {
        let text = '';
        for (let i = skipCount; i < bytes.length; i++) {
            const b = bytes[i];
            text += (b >= 0x20 && b < 0x7F) ? String.fromCharCode(b) : '';
        }
        return text.trim();
    }

    function logDecoded(label, resp, headerBytes) {
        if (!resp || isNoData(resp)) { obdLog(label + ': nepalaikoma/NO DATA', 'warn'); return; }
        const bytes = parseHexBytes(resp);
        const text = decodeAsciiFromBytes(bytes, headerBytes);
        obdLog(label + ': ' + text + '  [raw: ' + resp.replace(/[\r\n]+/g, ' ') + ']', 'success');
    }

    // 3. ECU identitetas (Mode 09) — standartinis SAE J1979 servisas
    async function runIdentity() {
        obdLog('=== ECU IDENTITETAS (Mode 09) ===', 'info');
        const vin = await sendCmd('0902', 3000);
        logDecoded('VIN', vin, 3);
        const calid = await sendCmd('0904', 3000);
        logDecoded('Calibration ID', calid, 3);
        const ecuname = await sendCmd('090A', 3000);
        logDecoded('ECU Name', ecuname, 3);
    }

    // 4. Protokolo zondas — KWP (Service 21) vs UDS (Service 22)
    async function runProtocolProbe() {
        obdLog('=== PROTOKOLO ZONDAS (KWP vs UDS) ===', 'info');
        const kwp = await sendCmd('2101', 2000);
        const uds = await sendCmd('22F190', 2000);
        const kwpOk = !!kwp && !isNoData(kwp);
        const udsOk = !!uds && !isNoData(uds);
        obdLog('Service 21 (KWP): ' + (kwpOk ? ('ATSAKO - ' + kwp.replace(/[\r\n]+/g, ' ')) : 'NO DATA'), kwpOk ? 'success' : 'warn');
        obdLog('Service 22 (UDS): ' + (udsOk ? ('ATSAKO - ' + uds.replace(/[\r\n]+/g, ' ')) : 'NO DATA'), udsOk ? 'success' : 'warn');
        return { kwpOk: kwpOk, udsOk: udsOk };
    }

    // Pakopa B — Mode 22 DID skenavimas variklio adresui + automatinis saugiklis į Service 21
    async function runDidScan() {
        obdLog('=== DID SKENAVIMAS (Service 22, variklis 0x7E0) ===', 'info');
        await sendCmd('1003', 2000); // extended diagnostic session - kai kurie DID reikalauja (pasala #5)
        let anyRealData = false;
        for (const c of DID_CANDIDATES) {
            const resp = await sendCmd('22' + c.did, 1500);
            const clean = resp ? resp.replace(/\s/g, '') : '';
            const isPositive = /^62/i.test(clean);
            if (isPositive) anyRealData = true;
            const status = resp ? resp.replace(/[\r\n]+/g, ' ') : 'be atsakymo';
            obdLog(c.label + ' (22' + c.did + '): ' + status, isPositive ? 'success' : (resp && !isNoData(resp) ? 'warn' : 'info'));
        }
        if (!anyRealData) {
            obdLog('Visi DID be teigiamo atsakymo (62...) - automatiškai perjungiama į Service 21 block sweep (saugiklis).', 'warn');
            await runBlockSweep();
        }
    }

    // Automatinis saugiklis / Planas B — Service 21 (KWP measuring blocks) 0x01-0xFF
    async function runBlockSweep() {
        obdLog('=== SERVICE 21 BLOKŲ SWEEP (0x01-0xFF) ===', 'info');
        for (let i = 1; i <= 255; i++) {
            const hex = i.toString(16).padStart(2, '0').toUpperCase();
            const resp = await sendCmd('21' + hex, 800);
            if (resp && !isNoData(resp)) {
                obdLog('Grupė ' + i + ' (21' + hex + '): ' + resp.replace(/[\r\n]+/g, ' '), 'success');
            }
        }
        obdLog('Service 21 sweep baigtas.', 'info');
    }

    // Pakopa C — modulių adresų skenavimas (0x700-0x7FF) su dvigubu 10 01 / 10 03 zondu + ATCS stebėjimas
    async function runModuleScan() {
        if (!window.confirm('Ar automobilis STOVI (variklis išjungtas arba laisva eiga, automobilis nejuda)? ' +
                'Skenavimas siunčia diagnostikos užklausas į nežinomus modulius (ABS/SRS ir kt.) - tęskite TIK jei automobilis stovi.')) {
            obdLog('Modulių skenavimas atšauktas (vartotojas nepatvirtino saugumo sąlygos).', 'warn');
            return;
        }
        obdLog('=== MODULIŲ SKENAVIMAS (0x700-0x7FF, 10 01 + 10 03) ===', 'info');
        for (let addr = 0x700; addr <= 0x7FF; addr++) {
            const hex = addr.toString(16).toUpperCase();
            await sendCmd('ATSH' + hex, 500);
            const r1 = await sendCmd('1001', 400);
            const r2 = await sendCmd('1003', 400);
            const ok1 = !!r1 && !isNoData(r1);
            const ok2 = !!r2 && !isNoData(r2);
            if (ok1 || ok2) {
                obdLog('Adresas 0x' + hex + ' ATSAKO: 10 01=' + (r1 || '-').replace(/[\r\n]+/g, ' ') + ' | 10 03=' + (r2 || '-').replace(/[\r\n]+/g, ' '), 'success');
            }
            if ((addr - 0x700) % 25 === 0) {
                const cs = await sendCmd('ATCS', 500);
                obdLog('CAN būsena (ATCS) ties 0x' + hex + ': ' + (cs || '-'), 'info');
            }
        }
        await sendCmd('ATSH7E0', 500);
        await sendCmd('ATSP6', 500);
        obdLog('Modulių skenavimas baigtas, grąžinta prie variklio adreso (0x7E0).', 'success');
    }

    // 8. Būsenos žymėjimas loge (analizei — koreliuoti HEX su realia variklio būsena)
    function tagState(label) {
        obdLog('=== BŪSENA: ' + label + ' ===', 'warn');
    }

    // "Surinkti viską" — vienas mygtukas, viena kelionė prie automobilio, max duomenų
    async function runFullCollection() {
        if (!STATE.connected) { obdLog('Nėra ryšio - pirma prisijunkite prie ELM327.', 'error'); return; }
        obdLog('########## PRADEDAMAS PILNAS DUOMENŲ RINKIMAS ##########', 'success');
        stopPolling();
        await runIdentity();
        await runProtocolProbe();
        await runDidScan();
        await runModuleScan();
        obdLog('########## RINKIMAS BAIGTAS - žurnalas išsaugomas automatiškai ##########', 'success');
        saveObdLog();
        startPolling();
    }

    return {
        init, toggleGlobal, startScan, pairAndConnect, forget,
        obdLog, clearObdLog, saveObdLog,
        runIdentity, runProtocolProbe, runDidScan, runBlockSweep, runModuleScan,
        tagState, runFullCollection,
        getState: () => STATE
    };
})();

// Auto-init
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', () => ObdBLE.init());
} else {
    ObdBLE.init();
}
