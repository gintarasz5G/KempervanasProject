# Užduotis: Pilno kodo audito pataisos (OBD + Renogy + native bridge)

**Kam:** Android asistentui (faktinis kodo taisymas). Cowork Claude = tik auditas, kodo nekeičia.
**Data:** 2026-07-09.
**Kontekstas:** `docs/audits/auditas_2026-07-09_pilnas_kodo_ir_veikimo_auditas.md` — pilnas
kodo+veikimo auditas (`obd.js`, `renogy.js`, `index.html`, native `MainActivity.java`).
Šis dokumentas paverčia to audito radinius į konkrečius, atliekamus pataisymus.

---

## ⚠️ PRIVALOMA prieš pradedant — teisingas failas

Redaguoti **TIK**:
- `android/www/obd.js`
- `android/www/renogy.js`
- `android/www/index.html`
- `android/android/app/src/main/java/lt/kemperis/app/MainActivity.java`

**NE** `android/android/app/src/main/assets/public/*` — tai build artefaktas
(`capacitor.config.ts` → `webDir: 'www'`), kurį perrašo `npx cap sync`. Bet kokie pakeitimai ten
bus tyliai prarasti. Baigus VISUS punktus žemiau: paleisti `npx cap sync`, tada build/flash.

---

## P0 — be šitų OBD funkcija realiai neveikia

### 1. PID bitmap ciklo bug — `android/www/obd.js:277`

**Dabar (sulaužyta):**
```js
for (let bit = 0; b < 8; bit++) {
```

**Pataisyti į:**
```js
for (let bit = 0; bit < 8; bit++) {
```

Priežastis: `b` yra baito reikšmė (0–255), ne skaitiklis — sąlyga niekad nekeičiama, todėl
`STATE.supportedPids` niekad neužsipildo (arba, jei `b<8`, begalinis ciklas užšaldo WebView).
Be šito nė vienas PID niekad nepoll'inamas, nepriklausomai nuo kitų pataisymų.

### 2. `consecutiveTimeouts` neinicijuotas — `android/www/obd.js:8-25`

**Dabar:** `STATE` objekte nėra šio lauko → `STATE.consecutiveTimeouts++` (136 eil.) ant
`undefined` duoda `NaN` → `NaN >= 3` (139 eil.) niekad `true` → auto-ATZ atsigavimas po
pasikartojančių timeout'ų niekad nesuveikia.

**Pataisyti:** į `STATE` objekto inicializaciją (šalia `autoResetCount: 0,`) pridėti:
```js
consecutiveTimeouts: 0,
```

### 3. BT atsijungimas niekad neišsprendžia laukiančio `sendCmd` — `android/www/obd.js:566-574`

**Dabar:**
```js
window.onObdDisconnected = function(reason) {
    if (STATE.connected) obdLog('Atsijungta: ' + (reason || ''), 'warn');
    STATE.connected = false;
    STATE.connecting = false;
    stopPolling();
    STATE.pending = null;
    STATE.queue = [];
    renderAssigned();
};
```

**Pataisyti į** (prieš `STATE.pending = null` — atlaisvinti laukiantį Promise):
```js
window.onObdDisconnected = function(reason) {
    if (STATE.connected) obdLog('Atsijungta: ' + (reason || ''), 'warn');
    STATE.connected = false;
    STATE.connecting = false;
    stopPolling();
    if (STATE.pending) {
        clearTimeout(STATE.pending.timer);
        const p = STATE.pending;
        STATE.pending = null;
        p.resolve(null); // <-- BŪTINA: kitaip laukiantis await sendCmd(...) niekad nebaigia
    }
    STATE.queue = [];
    STATE.scanBusy = false; // <-- atlaisvinti, kitaip run* funkcijos amžinai užblokuotos
    renderAssigned();
};
```

Priežastis: be šito bet koks BT ryšio nutrūkimas vidury `initSequence()` / `runFullCollection()`
/ `readDtcCodes()` amžinai pakabina tą async grandinę ir `STATE.scanBusy` lieka `true` —
visi tolimesni OBD veiksmai užblokuoti iki app perkrovimo.

---

## HIGH — native Android bridge

### 4. `connect()` be re-entry apsaugos — `MainActivity.java:986-1014`

Prieš naujos jungties kūrimą, jei jau yra aktyvus `socket`, jį reikia tvarkingai uždaryti ir
palaukti, kol sena read-loop gija pasibaigs, prieš perrašant `socket`/`in`/`out` laukus. Minimalus
saugus sprendimas:

```java
@JavascriptInterface
public void connect(String mac) {
    disconnectInternal(); // v54: uždaryti/išvalyti bet kokią senesnę jungtį PRIEŠ naują connect
    new Thread(() -> {
        try {
            if (adapter == null || !hasConnectPerm()) throw new java.io.IOException("Trūksta Bluetooth leidimo");
            BluetoothDevice device = adapter.getRemoteDevice(mac);
            if (hasScanPerm() && adapter.isDiscovering()) adapter.cancelDiscovery();
            BluetoothSocket s;
            try {
                s = device.createRfcommSocketToServiceRecord(SPP_UUID);
                s.connect();
            } catch (Exception firstErr) {
                s = (BluetoothSocket) device.getClass()
                        .getMethod("createRfcommSocket", int.class).invoke(device, 1);
                s.connect();
            }
            socket = s;
            in = socket.getInputStream();
            out = socket.getOutputStream();
            connected = true;
            evalJs("window.onObdConnected && window.onObdConnected()");
            startReadLoop();
        } catch (Exception e) {
            connected = false;
            final String msg = e.getMessage() == null ? "nezinoma klaida" : e.getMessage();
            evalJs("window.onObdDisconnected && window.onObdDisconnected(" + JSONObject.quote(msg) + ")");
        }
    }).start();
}
```

`startReadLoop()` savo `while (connected)` cikle skaito instance lauką `in` — kadangi
`disconnectInternal()` dabar visada iškviečiamas prieš naują `connect()`, senoji read-loop gija
pamatys `connected == false` ir korektiškai išsijungs prieš `in` bus perrašytas.

### 5. Socket/stream neuždaromi „iš išorės" atsijungus — `MainActivity.java:1067-1072`

**Dabar:**
```java
} finally {
    if (connected) {
        connected = false;
        evalJs("window.onObdDisconnected && window.onObdDisconnected('Rysys nutruko')");
    }
}
```

**Pataisyti į** (uždaryti resursus, ne tik pažymėti):
```java
} finally {
    if (connected) {
        connected = false;
        try { if (in != null) in.close(); } catch (Exception ignored) {}
        try { if (out != null) out.close(); } catch (Exception ignored) {}
        try { if (socket != null) socket.close(); } catch (Exception ignored) {}
        in = null; out = null; socket = null;
        evalJs("window.onObdDisconnected && window.onObdDisconnected('Rysys nutruko')");
    }
}
```

---

## MEDIUM — duomenų patikimumas (Renogy/OBD UI)

### 6. Sensorių reikšmės niekad neišvalomos atsijungus — `renogy.js:238-243` + `obd.js` (žr. 3 punktą aukščiau)

**`renogy.js` `onDisconnect()` dabar:**
```js
function onDisconnect(type, id) {
    debug(`${type} disconnected`, 'warn');
    STATE.devices[type].connected = false;
    if (type === 'battery') STATE.devices.battery.modelFetched = false;
    if (window.updateUI) window.updateUI();
}
```

**Pataisyti į** (išvalyti to įrenginio sensorių cache raktus, kad UI rodytų „—", ne seną reikšmę):
```js
function onDisconnect(type, id) {
    debug(`${type} disconnected`, 'warn');
    STATE.devices[type].connected = false;
    if (type === 'battery') {
        STATE.devices.battery.modelFetched = false;
        ['ren_soc','ren_v','ren_a','ren_cell_0','ren_cell_1','ren_cell_2','ren_cell_3',
         'ren_temp','ren_temp_1','ren_model'].forEach(k => delete window.sensorCache[k]);
    } else if (type === 'dcc') {
        Object.keys(window.sensorCache).filter(k => k.startsWith('dcc_')).forEach(k => delete window.sensorCache[k]);
    }
    if (window.updateUI) window.updateUI();
}
```

**`obd.js` `onObdDisconnected`** — į tą pačią funkciją (žr. 3 punktą) papildomai pridėti prieš
`renderAssigned();`:
```js
    ['obd_rpm','obd_speed','obd_coolant','obd_load','obd_map','obd_iat','obd_maf','obd_runtime',
     'obd_baro','obd_ecu_v','obd_ambient','obd_oil_t','obd_fuel_rate','obd_torque','obd_torque_ref',
     'obd_battery_v'].forEach(k => { if (window.sensorCache) delete window.sensorCache[k]; });
```

Tikslas: `index.html` `setSensorValue` bindingai (2211–2290 eil.) po atjungimo turi rodyti tuščią/
„—" būseną, o ne paskutinę žinomą „gyvą" reikšmę.

### 7. SOC 0% gali būti tyliai perrašytas — `index.html:1717-1725`

**Dabar:**
```js
(function() {
    let ah = parseFloat(sensorCache['junc_ah_remaining']);
    let cap = parseFloat(sensorCache['num_bat_capacity'] || 110);
    let soc = parseFloat(sensorCache['junc_soc']);
    if ((isNaN(soc) || soc <= 0) && !isNaN(ah) && ah > 0.5 && cap > 1) {
        sensorCache['junc_soc'] = Math.max(0, Math.min(100, Math.round((ah / cap) * 100)));
    }
})();
```

**Pataisyti** — taikyti apskaičiuotą fallback tik kai reikšmė tikrai trūksta (`NaN`), NE kai
Junctek sąmoningai praneša tikslų `0`:
```js
(function() {
    let ah = parseFloat(sensorCache['junc_ah_remaining']);
    let cap = parseFloat(sensorCache['num_bat_capacity'] || 110);
    let soc = parseFloat(sensorCache['junc_soc']);
    if (isNaN(soc) && !isNaN(ah) && ah > 0.5 && cap > 1) {
        sensorCache['junc_soc'] = Math.max(0, Math.min(100, Math.round((ah / cap) * 100)));
    }
})();
```

Priežastis: realus `0%` iš Junctek yra kritinis signalas ir turi pasiekti `soc_critical` aliarmą
nepakeistas; skaičiuoti pakaitalą prasminga tik kai duomens iš viso nėra (`NaN`).

### 8. Aliarmų slenksčių tarpusavio priklausomybė — `index.html:3767-3770`

`updateAlarmThreshold()` šiuo metu tikrina tik `!isNaN(val)`. Pridėti kryžminę patikrą tarp
susijusių porų (`water_low`/`water_critical`, `soc_low`/`soc_critical`, `co2_elevated`/
`co2_danger`), kad vartotojas negalėtų per Nustatymus nustatyti žemesnio-lygio slenksčio
(pvz. `water_low`) blogesnio nei kritinio (`water_critical`), nes tada žemo-lygio aliarmas tampa
nepasiekiamas. Konkreti implementacija priklauso nuo `_siblingThreshold()` funkcijos signatūros
tame faile — patikrinti prieš keičiant ir arba (a) atmesti/apkarpyti neteisingą reikšmę su
įspėjimu, arba (b) automatiškai koreguoti susijusį slenkstį, kad `low > critical` visada
išliktų tiesa.

---

## Testo protokolas

1. **OBD (su VEIKIANČIU varikliu):** prijungti ELM327, palaukti init. Patikrinti loge
   `Palaikomi PID: ...` — turi būti NETUŠČIAS sąrašas (patvirtina #1 pataisą). Palaukti kelis
   poll ciklus — RPM/greitis/temp turi kaitaliotis realiu laiku UI.
2. **BT atsijungimo atsparumas:** vykstant „Surinkti VISKĄ" arba poll'inimui, fiziškai išjungti
   ELM327 adapterį (arba nutolti). Patikrinti: (a) app UI neužsikimba, (b) galima vėl paspausti
   „Surinkti VISKĄ" po persijungimo be reikalo perkrauti app'ą (patvirtina #3), (c) OBD kortelės
   po kelių sekundžių rodo „—"/tuščia, ne paskutinę reikšmę (patvirtina #6).
3. **Renogy atsijungimas:** išjungti Renogy BT modulį/nutolti — patikrinti, kad `ren_*`/`dcc_*`
   kortelės išsivalo, ne rodo senos reikšmės.
4. **SOC testas:** jei įmanoma, patikrinti elgesį prie tikro 0% SOC (arba peržiūrėti kodą rankiniu
   būdu) — įsitikinti, kad realus `0` nebekeičiamas apskaičiuota reikšme.
5. `esphome`/build nelieciamas — šis darbas tik `android/www/*` + `MainActivity.java`. Baigus:
   `npx cap sync` iš `android/` aplanko, tada rebuild/flash APK.

## Šaltinis
- `docs/audits/auditas_2026-07-09_pilnas_kodo_ir_veikimo_auditas.md` — pilnas radinių sąrašas su
  papildomais žemesnio prioriteto punktais (SSE parse edge case, BME680 fallback, formatter
  defensyvumas), kurie čia neįtraukti kaip P0/HIGH, bet verta peržiūrėti vėliau.
