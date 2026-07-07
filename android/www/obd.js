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
        pollTimer: null,
        scanBusy: false,
        flushing: false,
        consecutiveTimeouts: 0,
        autoResetCount: 0,
        safeMode: localStorage.getItem('obd_safe_mode') === 'true',
        lastEngineRunning: null
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
        { did: '114E', label: 'DPF suodziai matuoti (22 11 4E)' },
        { did: '114F', label: 'DPF suodziai apskaiciuoti (22 11 4F)' },
        { did: '178C', label: 'DPF pelenai (Ash, 22 17 8C)' },
        { did: '1191', label: 'Turbo slegis faktinis (22 11 91)' },
        { did: '1235', label: 'Purkstukas 1 korekcija (22 12 35)' },
        { did: '1236', label: 'Purkstukas 2 korekcija (22 12 36)' },
        { did: '1156', label: 'Atstumas nuo regeneracijos (22 11 56)' },
        { did: '111F', label: 'Alyvos temp (Gemini kandidatas)' },
        { did: '1310', label: 'Alyvos temp (XGauge kandidatas)' },
        { did: '116B', label: 'Turbo praeasomas (Gemini kandidatas)' },
        { did: '116C', label: 'Turbo faktinis (Gemini kandidatas)' },
        { did: '1193', label: 'Rail slegis (Gemini kandidatas)' },
        { did: '0380', label: 'Visu purkstuku korekcijos (2-as kandidatas)' },
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
        if (STATE.pending || STATE.queue.length === 0 || STATE.flushing) return;
        if (!STATE.connected) { STATE.queue = []; return; }
        const item = STATE.queue.shift();
        const bridge = getBridge();
        if (!bridge) { item.resolve(null); return; }
        STATE.pending = { cmd: item.cmd, resolve: item.resolve, sentAt: Date.now() };
        obdLog('TX: ' + item.cmd);
        bridge.send(item.cmd);
    // Timeout callback'e (dabartinis kodas):
    STATE.pending.timer = setTimeout(async () => {
        if (STATE.pending && STATE.pending.cmd === item.cmd) {
            obdLog('RX: (timeout, be atsakymo)', 'warn');
            const p = STATE.pending; STATE.pending = null;
            STATE.consecutiveTimeouts++;

            // Taisymas 6b: Auto-ATZ atsigavimas po 3 timeoutų
            if (STATE.consecutiveTimeouts >= 3 && STATE.autoResetCount < 2) {
                STATE.flushing = true;
                await handleAutoRecovery();
                STATE.flushing = false;
                pumpQueue();
            } else {
                STATE.flushing = true;
                setTimeout(() => {
                    STATE.flushing = false;
                    pumpQueue();
                }, 250); // settle window
            }
            p.resolve(null);
        }
    }, item.timeoutMs);
}

async function handleAutoRecovery() {
    obdLog('⚠️ Pasikartojantys timeoutai — bandoma perkrauti adapterį (ATZ)...', 'error');
    STATE.autoResetCount++;
    const bridge = getBridge();
    if (bridge) bridge.send('ATZ');
    await new Promise(r => setTimeout(r, 1500));
    STATE.flushing = false; // <-- BŪTINA (Bug #2 fix): leisti eilei apdoroti initSequence komandas žemiau
    await initSequence(true); // silent init be polling restarto
    STATE.consecutiveTimeouts = 0;
}

    function onDataLine(line) {
        if (STATE.flushing) {
            obdLog('(ignoruotas pasenęs atsakymas po timeout): ' + line, 'warn');
            return;
        }
        if (!STATE.pending) { obdLog('RX (nelaukta): ' + line, 'warn'); return; }
        const p = STATE.pending;
        clearTimeout(p.timer);
        STATE.pending = null;
        STATE.consecutiveTimeouts = 0; // reset timeouts on success
        const dt = Date.now() - p.sentAt;
        obdLog('RX: ' + line.replace(/[\r\n]+/g, ' ') + ' (Δ' + dt + 'ms)', 'data');
        p.resolve(line);

        // Taisymas 7: Saugus režimas (pauzė tarp komandų)
        const delay = STATE.safeMode ? 300 : 0;
        setTimeout(pumpQueue, delay);
    }

    // --- Parsinimas ---

    function parseHexBytes(line) {
        const clean = line.replace(/[\r\n]/g, ' ').trim();
        const matches = clean.match(/\b[0-9A-Fa-f]{2}\b/g);
        return matches ? matches.map(h => parseInt(h, 16)) : [];
    }

    function isNoData(line) {
        return /NO DATA|UNABLE TO CONNECT|ERROR|STOPPED|CAN ERROR|\?|TIMEOUT/i.test(line);
    }

    // --- ELM327 init + polling ---

    async function initSequence(isRecovery = false) {
        if (!isRecovery) obdLog('ELM327 inicijavimas...', 'info');
        await sendCmd('ATZ', 3000);
        await sendCmd('ATE0');
        await sendCmd('ATL0');
        await sendCmd('ATH0');
        await sendCmd('ATSP6');
        await sendCmd('ATSTFF'); // Taisymas 6a: padidintas vidinis timeout
        await sendCmd('ATAT1');  // Adaptyvus laikas
        await sendCmd('ATCFC1'); // Auto Flow Control (Taisymas 7)

        if (!isRecovery) {
            obdLog('Inicijavimas baigtas, laukiama vartotojo veiksmo.', 'success');
            STATE.autoResetCount = 0;
            // startPolling() - isjungta v51 (neveikia EDC16)
        }
    }

    function startPolling() {
        stopPolling();
        let idx = 0;
        STATE.pollTimer = setInterval(async () => {
            if (!STATE.connected || ALL_PIDS.length === 0 || STATE.scanBusy) return;
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

            // Taisymas 8: Automatinis būsenos žymėjimas pagal RPM
            if (def.pid === '0C') {
                const running = (value > 0);
                if (running !== STATE.lastEngineRunning) {
                    STATE.lastEngineRunning = running;
                    obdLog('=== BŪSENA (auto): ' + (running ? ('Variklis veikia, RPM=' + Math.round(value)) : 'Variklis sustabdytas') + ' ===', 'warn');
                }
            }

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
            obdLog('Pasirinktas įrenginys nėra suporuotas. Suporuokite jį Android nustatymuose.', 'warn');
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

    function toggleSafeMode(on) {
        STATE.safeMode = on;
        localStorage.setItem('obd_safe_mode', on);
        obdLog('🐢 Saugus režimas: ' + (on ? 'ĮJUNGTAS' : 'IŠJUNGTAS'), on ? 'success' : 'warn');
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
                    <button class="btn btn-primary" style="width:auto; padding:6px 10px; font-size:11px;" onclick="ObdBLE.pairAndConnect('${d.mac}')">${d.paired ? 'Pasirinkti' : '---'}</button>
                </div>
            </div>
        `).join('');
    }

    function renderAssigned() {
        const el = document.getElementById('obd-assigned');
        if (el) el.textContent = STATE.connectedMac ? (STATE.connectedName || STATE.connectedMac) : '---';
        const statusEl = document.getElementById('obd-status');
        if (statusEl) statusEl.textContent = STATE.connected ? '🟢 Prijungta' : (STATE.connecting ? '🟡 Jungiamasi...' : '⚪ Atjungta');

        // Disable buttons if busy
        const btns = document.querySelectorAll('.obd-scan-btn');
        btns.forEach(b => b.disabled = STATE.scanBusy || !STATE.connected);
    }

    // --- Native callback'ai (kviečiami iš KemperisObdBridge.java per evaluateJavascript) ---

    window.onObdDeviceFound = function(mac, name, rssi) {
        STATE.devices[mac] = Object.assign(STATE.devices[mac] || {}, { mac: mac, name: name, rssi: rssi });
        renderScanList();
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

    // --- Pakopa B/C/C+ — gilus skenavimas ---

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
        if (STATE.scanBusy) return;
        STATE.scanBusy = true;
        renderAssigned();
        try {
            await runIdentityInternal();
        } finally {
            STATE.scanBusy = false;
            renderAssigned();
        }
    }

    async function runIdentityInternal() {
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
        if (STATE.scanBusy) return;
        STATE.scanBusy = true;
        renderAssigned();
        try {
            return await runProtocolProbeInternal();
        } finally {
            STATE.scanBusy = false;
            renderAssigned();
        }
    }

    async function runProtocolProbeInternal() {
        obdLog('=== PROTOKOLO ZONDAS (KWP vs UDS) ===', 'info');
        const kwp = await sendCmd('2101', 2000);
        const kwpOk = !!kwp && !isNoData(kwp);

        // UDS 22F190 patvirtinta NEVEIKIA (2026-07-06/07 žurnalai) — laikoma tik dokumentacijai/regresijai, NEBŪTINA siųsti kiekvieną kartą
        // const uds = await sendCmd('22F190', 2000);
        // const udsOk = !!uds && !isNoData(uds);
        const udsOk = false;

        obdLog('Service 21 (KWP): ' + (kwpOk ? ('ATSAKO - ' + kwp.replace(/[\r\n]+/g, ' ')) : 'NO DATA'), kwpOk ? 'success' : 'warn');
        obdLog('Service 22 (UDS): ' + (udsOk ? ('ATSAKO - ' + uds.replace(/[\r\n]+/g, ' ')) : 'NEVEIKIA (v51 bypass)'), udsOk ? 'success' : 'warn');
        return { kwpOk: kwpOk, udsOk: udsOk };
    }

    // Pakopa B — Mode 22 DID skenavimas variklio adresui + automatinis saugiklis į Service 21
    async function runDidScan() {
        if (STATE.scanBusy) return;
        STATE.scanBusy = true;
        renderAssigned();
        try {
            await runDidScanInternal();
            await sendCmd('1001', 1000); // Taisymas 1: grąžinti default sesiją
        } finally {
            STATE.scanBusy = false;
            renderAssigned();
        }
    }

    async function runDidScanInternal() {
        obdLog('=== DID SKENAVIMAS (Service 22, variklis 0x7E0) ===', 'info');
        await sendCmd('1003', 2000);
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
            obdLog('Visi DID be teigiamo atsakymo (62...) - automatiškai perjungiama į Service 21 block sweep.', 'warn');
            await runBlockSweepInternal();
        }
    }

    // Automatinis saugiklis / Planas B — Service 21 (KWP measuring blocks) 0x01-0xFF
    async function runBlockSweep() {
        if (STATE.scanBusy) return;
        STATE.scanBusy = true;
        renderAssigned();
        try {
            await runBlockSweepInternal();
            await sendCmd('1001', 1000);
        } finally {
            STATE.scanBusy = false;
            renderAssigned();
        }
    }

    async function runBlockSweepInternal() {
        obdLog('=== SERVICE 21 BLOKŲ SWEEP (0x01-0xFF) ===', 'info');
        for (let i = 1; i <= 255; i++) {
            const hex = i.toString(16).padStart(2, '0').toUpperCase();
            const resp = await sendCmd('21' + hex, 800);
            if (resp && !isNoData(resp)) {
                obdLog('Grupė ' + i + ' (21' + hex + '): ' + resp.replace(/[\r\n]+/g, ' '), 'success');
            }
        }
    }

    // Pakopa C — modulių adresų skenavimas (0x700-0x7FF) su dvigubu 10 01 / 10 03 zondu + ATCS stebėjimas
    async function runModuleScan() {
        if (STATE.scanBusy) return;
        if (!window.confirm('Ar automobilis STOVI? Skenavimas siunčia diagnostikos užklausas į nežinomus modulius.')) return;
        STATE.scanBusy = true;
        renderAssigned();
        try {
            await runModuleScanInternal();
        } finally {
            STATE.scanBusy = false;
            renderAssigned();
        }
    }

    async function runModuleScanInternal() {
        obdLog('=== MODULIŲ SKENAVIMAS (0x700-0x7FF, 10 01 + 10 03) ===', 'info');
        const startCs = await sendCmd('ATCS', 600);
        obdLog('CAN būsena (ATCS) pradžioje: ' + (startCs || '-'), 'info');

        for (let addr = 0x700; addr <= 0x7FF; addr++) {
            const hex = addr.toString(16).toUpperCase();
            await sendCmd('ATSH' + hex, 600);
            const r1 = await sendCmd('1001', 600);
            const r2 = await sendCmd('1003', 600);
            const ok1 = !!r1 && !isNoData(r1) && r1.trim() !== 'OK';
            const ok2 = !!r2 && !isNoData(r2) && r2.trim() !== 'OK';
            if (ok1 || ok2) {
                obdLog('Adresas 0x' + hex + ' ATSAKO: 10 01=' + (r1 || '-').replace(/[\r\n]+/g, ' ') + ' | 10 03=' + (r2 || '-').replace(/[\r\n]+/g, ' '), 'success');
            }
        }

        const endCs = await sendCmd('ATCS', 600);
        obdLog('CAN būsena (ATCS) pabaigoje: ' + (endCs || '-'), 'info');

        await sendCmd('ATSH7E0', 1000);
        await sendCmd('ATSP6', 1000);
        await sendCmd('1001', 1000); // Taisymas 1
        obdLog('Modulių skenavimas baigtas.', 'success');
    }

    function tagState(label) {
        obdLog('=== BŪSENA: ' + label + ' ===', 'warn');
    }

    // "Surinkti viską" — vienas mygtukas, viena kelionė prie automobilio, max duomenų
    async function runFullCollection() {
        if (STATE.scanBusy || !STATE.connected) { obdLog('Nėra ryšio arba skenavimas jau vyksta.', 'error'); return; }
        if (!window.confirm('Ar automobilis STOVI? Vyks pilnas skenavimas.')) return;
        STATE.scanBusy = true;
        renderAssigned();
        try {
            obdLog('########## PRADEDAMAS OPTIMIZUOTAS DUOMENŲ RINKIMAS ##########', 'success');
            stopPolling();
            // await runIdentityInternal(); // Isjungta v51 (neveikia EDC16)
            await runProtocolProbeInternal();
            await runDidScanInternal();
            // await runModuleScanInternal(); // Isjungta v51 (neveikia EDC16)
            await sendCmd('1001', 1000); // Taisymas 1 final
            obdLog('########## RINKIMAS BAIGTAS - žurnalas išsaugomas automatiškai ##########', 'success');
            saveObdLog();
        } finally {
            STATE.scanBusy = false;
            renderAssigned();
            // startPolling(); // Isjungta v51
        }
    }

    return {
        init, toggleGlobal, startScan, pairAndConnect, forget, toggleSafeMode,
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
