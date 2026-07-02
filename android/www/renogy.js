// Renogy BLE Implementation for Kemperis App (v32.3+)
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
        devices: {
            battery: { id: null, name: null, connected: false, lastSync: 0, buffer: [], retryCount: 0 },
            dcc:     { id: null, name: null, connected: false, lastSync: 0, buffer: [], retryCount: 0 }
        },
        pollingInterval: null,
        reconnectTimers: {}
    };

    // --- Helpers ---

    function debug(msg, type = 'info') {
        if (window.isDebugEnabled && window.isDebugEnabled()) {
            window.sysLog && window.sysLog('[Renogy] ' + msg, type);
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

    // --- BLE Core ---

    async function init() {
        const { BleClient } = Capacitor.Plugins.BluetoothLe;
        try {
            await BleClient.initialize();
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

    async function toggleGlobal(on) {
        STATE.enabled = on;
        localStorage.setItem('renogy_enabled', on);
        if (on) startPolling();
        else stopPolling();
    }

    function startPolling() {
        if (STATE.pollingInterval) return;
        pollCycle();
        STATE.pollingInterval = setInterval(pollCycle, 10000);
    }

    function stopPolling() {
        if (STATE.pollingInterval) clearInterval(STATE.pollingInterval);
        STATE.pollingInterval = null;
        disconnectDevice('battery');
        disconnectDevice('dcc');
    }

    async function pollCycle() {
        if (!STATE.enabled) return;
        const { BleClient } = Capacitor.Plugins.BluetoothLe;
        if (!(await BleClient.isEnabled())) {
            debug('Bluetooth is OFF', 'warn');
            return;
        }

        // Poll battery
        if (STATE.devices.battery.id) {
            if (!STATE.devices.battery.connected) connectAndNotify('battery');
            else queryDevice('battery', [0xFF, 0x03, 0x13, 0x88, 0x00, 0x22]); // Basic block
        }

        // Poll DCC (wait 2s)
        setTimeout(async () => {
            if (STATE.devices.dcc.id) {
                if (!STATE.devices.dcc.connected) connectAndNotify('dcc');
                else queryDevice('dcc', [0xFF, 0x03, 0x01, 0x00, 0x00, 0x21]); // Basic block
            }
        }, 2000);
    }

    async function connectAndNotify(type) {
        const dev = STATE.devices[type];
        if (dev.connected || !dev.id) return;

        const { BleClient } = Capacitor.Plugins.BluetoothLe;
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
        }
    }

    async function disconnectDevice(type) {
        const dev = STATE.devices[type];
        if (!dev.id) return;
        const { BleClient } = Capacitor.Plugins.BluetoothLe;
        try {
            await BleClient.stopNotifications(dev.id, SERVICE_NOTIFY, CHAR_NOTIFY);
            await BleClient.disconnect(dev.id);
        } catch (e) {}
        dev.connected = false;
    }

    function onDisconnect(type, id) {
        debug(`${type} disconnected`, 'warn');
        STATE.devices[type].connected = false;
    }

    async function queryDevice(type, cmdBase) {
        const dev = STATE.devices[type];
        if (!dev.connected) return;

        const { BleClient } = Capacitor.Plugins.BluetoothLe;
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
        if (dev.buffer.length < 5) return;
        const expectedLen = dev.buffer[2] + 5;
        if (dev.buffer.length >= expectedLen) {
            const frame = dev.buffer.slice(0, expectedLen);
            dev.buffer = dev.buffer.slice(expectedLen);

            // Check CRC
            const payload = frame.slice(0, -2);
            const crcReceived = [frame[frame.length-2], frame[frame.length-1]];
            const crcCalc = crc16(payload);

            if (crcCalc[0] === crcReceived[0] && crcCalc[1] === crcReceived[1]) {
                parseFrame(type, frame.slice(3, -2));
                dev.lastSync = Date.now();
                if (window.updateUI) window.updateUI();
            } else {
                debug(`${type} CRC error`, 'error');
            }
        }
    }

    function parseFrame(type, data) {
        if (type === 'battery') {
            // Register 0x1388 (5000) base
            window.sensorCache['ren_soc']  = u16(data[0], data[1]) / 10.0;
            window.sensorCache['ren_v']    = u16(data[2], data[3]) / 10.0;
            window.sensorCache['ren_a']    = s16(data[4], data[5]) / 100.0;

            // Temp (5005-5006 range)
            window.sensorCache['ren_temp'] = s16(data[10], data[11]);

            // Cell voltages (5007...)
            window.sensorCache['ren_cell_0'] = u16(data[14], data[15]) / 1000.0;
            window.sensorCache['ren_cell_1'] = u16(data[16], data[17]) / 1000.0;
            window.sensorCache['ren_cell_2'] = u16(data[18], data[19]) / 1000.0;
            window.sensorCache['ren_cell_3'] = u16(data[20], data[21]) / 1000.0;
        } else if (type === 'dcc') {
            // Register 0x0100 base
            window.sensorCache['dcc_batt_v'] = u16(data[2], data[3]) / 10.0;
            window.sensorCache['dcc_charge_a'] = u16(data[4], data[5]) / 10.0;

            // PV (0x0107...)
            window.sensorCache['dcc_pv_v'] = u16(data[14], data[15]) / 10.0;
            window.sensorCache['dcc_pv_a'] = u16(data[16], data[17]) / 10.0;
            window.sensorCache['dcc_solar_w'] = u16(data[18], data[19]);

            // Alternator (0x0117...)
            window.sensorCache['dcc_alt_v'] = u16(data[46], data[47]) / 10.0;
            window.sensorCache['dcc_alt_a'] = u16(data[48], data[49]) / 10.0;
            window.sensorCache['dcc_alt_w'] = u16(data[50], data[51]);

            // Status (0x0120)
            const stageCode = data[65];
            const stages = ["Inactive", "Normal", "Boost", "Float", "Equalize", "Direct", "Limit"];
            window.sensorCache['dcc_stage'] = stages[stageCode] || "Unknown";
        }
    }

    // --- UI Methods ---

    async function startScan() {
        const { BleClient } = Capacitor.Plugins.BluetoothLe;
        STATE.scanning = true;
        const results = [];
        try {
            await BleClient.requestLEScan({}, (result) => {
                if (result.device.deviceId === '38:3B:26:79:DB:64') return; // Skip Junctek
                results.push(result);
            });
            setTimeout(async () => {
                await BleClient.stopLEScan();
                STATE.scanning = false;
                renderScanResults(results);
            }, 5000);
        } catch (e) {
            STATE.scanning = false;
            debug('Scan error: ' + e.message, 'error');
        }
    }

    function renderScanResults(results) {
        const container = document.getElementById('renogy-scan-list');
        if (!container) return;

        // Dedup by ID
        const unique = Array.from(new Map(results.map(r => [r.device.deviceId, r])).values());

        container.innerHTML = unique.map(r => `
            <div class="card" style="margin-bottom:8px; padding:12px;">
                <div style="display:flex; justify-content:space-between; align-items:center;">
                    <div>
                        <div style="font-weight:bold;">${r.device.name || 'Unknown'}</div>
                        <div style="font-size:11px; color:#8b949e;">${r.device.deviceId}</div>
                    </div>
                    <div style="display:flex; gap:8px;">
                        <button class="btn btn-primary" style="width:auto; padding:4px 8px;" onclick="RenogyBLE.assign('${r.device.deviceId}', '${r.device.name}', 'battery')">Baterija</button>
                        <button class="btn btn-success" style="width:auto; padding:4px 8px;" onclick="RenogyBLE.assign('${r.device.deviceId}', '${r.device.name}', 'dcc')">DCC50S</button>
                    </div>
                </div>
            </div>
        `).join('');
    }

    function assign(id, name, type) {
        STATE.devices[type].id = id;
        STATE.devices[type].name = name;
        localStorage.setItem(`ren_dev_${type}`, JSON.stringify({id, name}));
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
        init, toggleGlobal, startScan, assign, forget,
        getState: () => STATE
    };
})();

// Auto-init
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', () => RenogyBLE.init());
} else {
    RenogyBLE.init();
}
