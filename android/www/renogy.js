// Renogy BLE Implementation for Kemperis App (v33.0+)
// Protocol: Modbus RTU over BLE GATT
// Based on: https://github.com/cyrils/renogy-bt

const RenogyBLE = (function() {
    const SERVICE_WRITE = '0000ffd0-0000-1000-8000-00805f9b34fb';
    const CHAR_WRITE    = '0000ffd1-0000-1000-8000-00805f9b34fb';
    const SERVICE_NOTIFY = '0000fff0-0000-1000-8000-00805f9b34fb';
    const CHAR_NOTIFY    = '0000fff1-0000-1000-8000-00805f9b34fb';

    const STATE = {
        enabled: false,
        scanning: false,
        lastScanTime: 0,
        devices: {
            battery: { id: null, name: null, connected: false, lastSync: 0, buffer: [], retryCount: 0, lastQuery: 0, queryIndex: 0 },
            dcc:     { id: null, name: null, connected: false, lastSync: 0, buffer: [], retryCount: 0, lastQuery: 0 }
        },
        pollingInterval: null,
        btEnabled: true,
        lastSkipReason: null
    };

    // --- Helpers ---

    function getBle() {
        return window.capacitorCommunityBluetoothLe ? window.capacitorCommunityBluetoothLe.BleClient : null;
    }

    function debug(msg, type = 'info') {
        if (type === 'error') {
            window.sysLog && window.sysLog('[Renogy] ' + msg, 'error');
        } else if (window.isDebugEnabled && window.isDebugEnabled()) {
            window.sysLog && window.sysLog('[Renogy] ' + msg, type);
        }
    }

    function hexLog(type, data) {
        if (window.isDebugEnabled && window.isDebugEnabled()) {
            const hex = Array.from(data).map(b => b.toString(16).padStart(2, '0')).join(' ');
            window.sysLog && window.sysLog(`[Renogy HEX] ${type}: ${hex}`, 'data');
        }
    }

    function crc16(buffer) {
        let crc = 0xFFFF;
        for (let i = 0; i < buffer.length; i++) {
            crc ^= buffer[i];
            for (let j = 0; j < 8; j++) {
                if (crc & 0x0001) crc = (crc >> 1) ^ 0xA001;
                else crc >>= 1;
            }
        }
        return [crc & 0xFF, (crc >> 8) & 0xFF]; // Lo, Hi
    }

    function u16(b1, b2) { return (b1 << 8) | b2; }
    function s16(b1, b2) { let v = u16(b1, b2); return v > 32767 ? v - 65536 : v; }
    function u32(b1, b2, b3, b4) { return ((b1 << 24) | (b2 << 16) | (b3 << 8) | b4) >>> 0; }

    // --- BLE Core ---

    async function init() {
        const BleClient = getBle();
        if (!BleClient) {
            const err = 'Bluetooth plugin NOT FOUND';
            debug(err, 'error');
            const container = document.getElementById('renogy-scan-list');
            if (container) container.innerHTML = '<div style="color:#ff7b72;padding:10px;">❌ BLE neinicijuotas: ' + err + '</div>';
            return;
        }
        try {
            await BleClient.initialize({ androidNeverForLocation: true });
            STATE.btEnabled = await BleClient.isEnabled();

            BleClient.startEnabledNotifications((enabled) => {
                STATE.btEnabled = enabled;
                debug('Bluetooth ' + (enabled ? 'ON' : 'OFF'), enabled ? 'success' : 'warn');
                if (enabled && STATE.enabled) startPolling();
                if (window.updateUI) window.updateUI();
            });

            STATE.enabled = localStorage.getItem('renogy_enabled') !== 'false';

            // Load saved devices
            const savedBat = localStorage.getItem('ren_dev_battery');
            if (savedBat) {
                const d = JSON.parse(savedBat);
                STATE.devices.battery.id = d.id;
                STATE.devices.battery.name = d.name;
            }
            const savedDcc = localStorage.getItem('ren_dev_dcc');
            if (savedDcc) {
                const d = JSON.parse(savedDcc);
                STATE.devices.dcc.id = d.id;
                STATE.devices.dcc.name = d.name;
            }

            if (STATE.enabled) startPolling();
        } catch (e) {
            debug('Init error: ' + e.message, 'error');
        }
    }

    async function requestEnableBT() {
        const BleClient = getBle();
        if (!BleClient) return;
        try {
            await BleClient.requestEnable();
        } catch (e) {
            debug('Enable BT failed: ' + e.message, 'error');
        }
    }

    async function toggleGlobal(on) {
        STATE.enabled = on;
        localStorage.setItem('renogy_enabled', on);
        if (on) startPolling();
        else stopPolling();
    }

    function startPolling() {
        if (STATE.pollingInterval) return;
        pollCycle();
        STATE.pollingInterval = setInterval(pollCycle, 5000); // Check every 5s, query every 10s
    }

    function stopPolling() {
        if (STATE.pollingInterval) clearInterval(STATE.pollingInterval);
        STATE.pollingInterval = null;
        disconnectDevice('battery');
        disconnectDevice('dcc');
    }

    async function pollCycle() {
        const BleClient = getBle();
        let skip = null;
        if (!STATE.enabled) skip = 'pollCycle skip: modulis OFF';
        else if (!BleClient || !STATE.btEnabled) skip = 'pollCycle skip: BT isjungtas';
        else if (!STATE.devices.battery.id && !STATE.devices.dcc.id) skip = 'pollCycle skip: nepriskirtas irenginys';

        if (skip) {
            if (STATE.lastSkipReason !== skip) {
                debug(skip, 'warn');
                STATE.lastSkipReason = skip;
            }
            return;
        }
        STATE.lastSkipReason = null;

        const now = Date.now();

        // Poll battery (Query alternates between cells and dynamic data)
        if (STATE.devices.battery.id && (now - STATE.devices.battery.lastQuery > 10000)) {
            if (!STATE.devices.battery.connected) {
                if (STATE.devices.battery.retryCount < 5 || (now - STATE.devices.battery.lastSync > 60000)) {
                    connectAndNotify('battery');
                }
            } else {
                if (STATE.devices.battery.queryIndex === 0) {
                    // Cells & Temp: 0x1388 (5000), count 0x22
                    queryDevice('battery', [0xFF, 0x03, 0x13, 0x88, 0x00, 0x22]);
                    STATE.devices.battery.queryIndex = 1;
                } else {
                    // V/A/Ah: 0x13B2 (5042), count 0x06
                    queryDevice('battery', [0xFF, 0x03, 0x13, 0xB2, 0x00, 0x06]);
                    STATE.devices.battery.queryIndex = 0;
                }
                STATE.devices.battery.lastQuery = now;
            }
        }

        // Poll DCC (wait 2.5s offset)
        setTimeout(async () => {
            if (STATE.devices.dcc.id && (Date.now() - STATE.devices.dcc.lastQuery > 10000)) {
                if (!STATE.devices.dcc.connected) {
                    if (STATE.devices.dcc.retryCount < 5 || (Date.now() - STATE.devices.dcc.lastSync > 60000)) {
                        connectAndNotify('dcc');
                    }
                } else {
                    // DCC Main: 0x0100, count 0x21
                    queryDevice('dcc', [0xFF, 0x03, 0x01, 0x00, 0x00, 0x21]);
                    STATE.devices.dcc.lastQuery = Date.now();
                }
            }
        }, 2500);
    }

    async function connectAndNotify(type) {
        const dev = STATE.devices[type];
        if (dev.connected || !dev.id) return;

        const BleClient = getBle();
        try {
            debug(`Connecting to ${type} (${dev.name})...`);
            await BleClient.connect(dev.id, (id) => onDisconnect(type, id));
            dev.connected = true;
            dev.retryCount = 0;
            debug(`Connected to ${type}`, 'success');

            await BleClient.startNotifications(dev.id, SERVICE_NOTIFY, CHAR_NOTIFY, (data) => {
                onData(type, new Uint8Array(data.buffer));
            });
        } catch (e) {
            dev.connected = false;
            dev.retryCount++;
            debug(`${type} connect failed: ${e.message}`, 'warn');
            if (window.updateUI) window.updateUI();
        }
    }

    async function disconnectDevice(type) {
        const dev = STATE.devices[type];
        if (!dev.id) return;
        const BleClient = getBle();
        try {
            await BleClient.stopNotifications(dev.id, SERVICE_NOTIFY, CHAR_NOTIFY);
            await BleClient.disconnect(dev.id);
        } catch (e) {}
        dev.connected = false;
    }

    function onDisconnect(type, id) {
        debug(`${type} disconnected`, 'warn');
        STATE.devices[type].connected = false;
        if (window.updateUI) window.updateUI();
    }

    async function queryDevice(type, cmdBase) {
        const dev = STATE.devices[type];
        if (!dev.connected) return;

        const BleClient = getBle();
        const fullCmd = new Uint8Array([...cmdBase, ...crc16(cmdBase)]);
        try {
            dev.buffer = []; // Reset buffer for new query
            await BleClient.write(dev.id, SERVICE_WRITE, CHAR_WRITE, fullCmd);
        } catch (e) {
            debug(`${type} query failed: ${e.message}`, 'error');
        }
    }

    function onData(type, data) {
        const dev = STATE.devices[type];
        dev.buffer.push(...data);

        // Modbus RTU Response: [id, func, len, ...data, crcL, crcH]
        while (dev.buffer.length >= 5 && dev.buffer.length >= dev.buffer[2] + 5) {
            const expectedLen = dev.buffer[2] + 5;
            const frame = dev.buffer.slice(0, expectedLen);
            dev.buffer = dev.buffer.slice(expectedLen);

            // Check CRC
            const payload = frame.slice(0, -2);
            const crcReceived = [frame[frame.length-2], frame[frame.length-1]];
            const crcCalc = crc16(payload);

            if (crcCalc[0] === crcReceived[0] && crcCalc[1] === crcReceived[1]) {
                hexLog(type, frame);
                parseFrame(type, frame);
                dev.lastSync = Date.now();
                if (window.updateUI) window.updateUI();
            } else {
                debug(`${type} CRC error`, 'error');
            }
        }
    }

    function parseFrame(type, frame) {
        const data = frame.slice(3, -2); // Remove ID, Func, Len and CRC
        const len = frame[2];

        if (type === 'battery') {
            if (len === 0x44) { // 0x1388 (5000) - Cells/Temp
                // data[0..1] = cell count
                // data[2..13] = cell voltages (v1..v6) -> we only need 1..4
                // R6: Scale 0.1
                window.sensorCache['ren_cell_0'] = u16(data[2], data[3]) / 10.0;
                window.sensorCache['ren_cell_1'] = u16(data[4], data[5]) / 10.0;
                window.sensorCache['ren_cell_2'] = u16(data[6], data[7]) / 10.0;
                window.sensorCache['ren_cell_3'] = u16(data[8], data[9]) / 10.0;

                // R2: Temperatures (reg 5018 / data[36..37])
                window.sensorCache['ren_temp'] = s16(data[36], data[37]) / 10.0;
            } else if (len === 0x0C) { // 0x13B2 (5042) - V/A/Ah/SOC
                const currentRaw = s16(data[0], data[1]); // 5042
                window.sensorCache['ren_a'] = currentRaw / 100.0;
                window.sensorCache['ren_v'] = u16(data[2], data[3]) / 10.0; // 5043

                const remAh = u32(data[4], data[5], data[6], data[7]) / 1000.0; // 5044-45
                const capAh = u32(data[8], data[9], data[10], data[11]) / 1000.0; // 5046-47

                if (capAh > 0) window.sensorCache['ren_soc'] = (remAh / capAh) * 100.0;
            } else {
                debug(`Nezinomas baterijos blokas: len=${len}`, 'warn');
            }
        } else if (type === 'dcc') {
            if (len === 0x42) { // Register 0x0100 base
                window.sensorCache['dcc_batt_v'] = u16(data[2], data[3]) / 10.0; // 0x101
                window.sensorCache['dcc_charge_a'] = u16(data[4], data[5]) / 100.0; // 0x102 (Scale /100 based on renogy-bt)

                // R2: Alternator (0x104..0x106) -> data[8..13]
                window.sensorCache['dcc_alt_v'] = u16(data[8], data[9]) / 10.0;
                window.sensorCache['dcc_alt_a'] = u16(data[10], data[11]) / 100.0;
                window.sensorCache['dcc_alt_w'] = u16(data[12], data[13]);

                // PV (0x107..0x109) -> data[14..19]
                window.sensorCache['dcc_pv_v'] = u16(data[14], data[15]) / 10.0;
                window.sensorCache['dcc_pv_a'] = u16(data[16], data[17]) / 100.0;
                window.sensorCache['dcc_solar_w'] = u16(data[18], data[19]);

                // R3: Status (0x120 / data[64..65])
                const stageCode = data[65];
                const stages = {
                    0: "Neaktyvus", 1: "Aktyvuotas", 2: "MPPT", 3: "Islyginimas",
                    4: "Boost", 5: "Palaikymas", 6: "Sroves ribojimas", 8: "Tiesiai is generatoriaus"
                };
                window.sensorCache['dcc_stage'] = stages[stageCode] || ("Kodas " + stageCode);
            } else {
                debug(`Nezinomas DCC blokas: len=${len}`, 'warn');
            }
        }
    }

    // --- UI Methods ---

    async function startScan() {
        if (Date.now() - STATE.lastScanTime < 10000) {
            debug('Scan cooldown active', 'warn');
            return;
        }
        const BleClient = getBle();
        if (!BleClient) return;

        const container = document.getElementById('renogy-scan-list');
        if (container) container.innerHTML = '<div style="color:#8b949e;padding:10px;">🔍 Skenuojama...</div>';

        try {
            const isBtOn = await BleClient.isEnabled(); // R4: Rename hasLocation
            if (!isBtOn) {
                await requestEnableBT();
                if (container) container.innerHTML = '';
                return;
            }

            STATE.scanning = true;
            STATE.lastScanTime = Date.now();
            if (window.updateUI) window.updateUI();

            const results = [];
            await BleClient.requestLEScan({}, (result) => {
                if (result.device.deviceId === '38:3B:26:79:DB:64') return; // Skip Junctek
                results.push(result);
                renderScanResults(results); // Live update
            });

            setTimeout(async () => {
                await BleClient.stopLEScan();
                STATE.scanning = false;
                if (results.length === 0 && container) {
                    container.innerHTML = '<div style="color:#8b949e;padding:10px;">Nieko nerasta. Patikrinkite BT leidimus (žr. Logs)</div>';
                }
                if (window.updateUI) window.updateUI();
            }, 8000);
        } catch (e) {
            STATE.scanning = false;
            const msg = e.message || e;
            debug('Scan error: ' + msg, 'error');
            if (container) container.innerHTML = '<div style="color:#ff7b72;padding:10px;">❌ Klaida: ' + msg + '</div>';
            if (window.updateUI) window.updateUI();
        }
    }

    function renderScanResults(results) {
        const container = document.getElementById('renogy-scan-list');
        if (!container) return;

        // R4: Sort Renogy devices to top
        const unique = Array.from(new Map(results.map(r => [r.device.deviceId, r])).values());
        unique.sort((a, b) => {
            const nameA = (a.device.name || '').toUpperCase();
            const nameB = (b.device.name || '').toUpperCase();
            const isRenA = nameA.startsWith('BT-TH') || nameA.startsWith('RNGRBP');
            const isRenB = nameB.startsWith('BT-TH') || nameB.startsWith('RNGRBP');
            if (isRenA && !isRenB) return -1;
            if (!isRenA && isRenB) return 1;
            return 0;
        });

        container.innerHTML = unique.map(r => `
            <div class="card" style="margin-bottom:8px; padding:12px; background:rgba(255,255,255,0.05);">
                <div style="display:flex; justify-content:space-between; align-items:center;">
                    <div>
                        <div style="font-weight:bold;">${r.device.name || 'Unknown Device'}</div>
                        <div style="font-size:11px; color:#8b949e;">${r.device.deviceId}</div>
                    </div>
                    <div style="display:flex; gap:8px;">
                        <button class="btn btn-primary" style="width:auto; padding:6px 10px; font-size:11px;" onclick="RenogyBLE.assign('${r.device.deviceId}', '${r.device.name}', 'battery')">Baterija</button>
                        <button class="btn btn-success" style="width:auto; padding:6px 10px; font-size:11px;" onclick="RenogyBLE.assign('${r.device.deviceId}', '${r.device.name}', 'dcc')">DCC50S</button>
                    </div>
                </div>
            </div>
        `).join('');
    }

    function assign(id, name, type) {
        STATE.devices[type].id = id;
        STATE.devices[type].name = name || 'Renogy Device';
        localStorage.setItem(`ren_dev_${type}`, JSON.stringify({id, name: STATE.devices[type].name}));
        debug(`Assigned ${name} as ${type}`, 'success');
        document.getElementById('renogy-scan-list').innerHTML = '';
        if (STATE.enabled) pollCycle();
        if (window.updateUI) window.updateUI();
    }

    function forget(type) {
        disconnectDevice(type);
        STATE.devices[type].id = null;
        STATE.devices[type].name = null;
        localStorage.removeItem(`ren_dev_${type}`);
        if (window.updateUI) window.updateUI();
    }

    return {
        init, toggleGlobal, startScan, assign, forget, requestEnableBT,
        getState: () => STATE
    };
})();

// Auto-init
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', () => RenogyBLE.init());
} else {
    RenogyBLE.init();
}
