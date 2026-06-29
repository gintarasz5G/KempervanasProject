# KemperisApp v14 — Giluminis Audito Ataskaita
**Data:** 2026-06-15  
**Versija:** v14 (CURRENT_VERSION = 14, APK)  
**Firmware:** kempervanas_v22.yaml (įdiegtas kempetyje — **nekeičiamas**)  
**Auditoriuius:** Claude Sonnet 4.6 (Cowork session)

---

## 1. Architektūra ir technologijų krūvas

### 1.1 Sluoksniai

```
┌─────────────────────────────────────────────┐
│  www/index.html  (vienas ~2520 eilučių failas)
│  HTML + CSS + 2 000+ eilučių JS monolitas   │
├─────────────────────────────────────────────┤
│  Capacitor 8.x WebView wrapper              │
│  androidScheme: 'http', cleartext: true     │
├─────────────────────────────────────────────┤
│  Java bridges (MainActivity.java)           │
│  KemperisSms · KempTts · KemperisWifi       │
│  KemperisVol · KemperisFile · UpdateBridge  │
├─────────────────────────────────────────────┤
│  KemperisService.java  (Native FG Service)  │
│  FOREGROUND_SERVICE_TYPE_SPECIAL_USE        │
├─────────────────────────────────────────────┤
│  cordova-plugin-background-mode             │
│  cordova-plugin-device                      │
├─────────────────────────────────────────────┤
│  ESP32-S3 / ESPHome v22 (192.168.4.1)      │
│  SSE /events · REST /sensor /switch ...     │
└─────────────────────────────────────────────┘
```

### 1.2 Duomenų srautai

- **Online:** SSE EventSource → `state` event → `sensorCache` → `updateUI()`
- **Offline:** Google Sheets REST (`?action=latest`) → `sensorCache` → `updateUI()`
- **SMS:** Android `BroadcastReceiver` → `evaluateJavascript("onSmsReceived(...)")` → JS handler
- **Komandos → ESP32:** `fetch(ESPHOME_URL + /button|switch|number|text/...)` REST POST

---

## 2. Failų inventorius ir atsakomybės

| Failas | Paskirtis |
|--------|-----------|
| `www/index.html` | Vienintelis UI/JS failas — visos 10 skirtukų, SSE, aliarmai, TTS, žemėlapis, CSV/GPX |
| `android/app/src/main/java/lt/kemperis/app/MainActivity.java` | BridgeActivity, 7 native bridges, SMS gavimas, WiFi bind, versijų atnaujinimas |
| `android/app/src/main/java/lt/kemperis/app/KemperisService.java` | Native FG Service, Android 14 `specialUse` tipas, START_STICKY |
| `android/app/src/main/AndroidManifest.xml` | Leidimai, FG service deklaracijos, FileProvider |
| `capacitor.config.ts` | appId: lt.kemperis.app, androidScheme: http, cleartext: true |
| `package.json` | Capacitor 8.3.4, cordova plugins, @capacitor-community/text-to-speech |

---

## 3. UI — Skirtukų funkcionalumas

### 3.1 Skirtukų sąrašas

| Skirtukas | Turinys | Offline? |
|-----------|---------|----------|
| 📊 Apžvalga | 12 priority kortelių: SOC, akum. statusas, vanduo, dujos, temp, drėgmė, oro kokybė, CO₂, patarimas, oras, 220V, apsauga | Dalinis (Sheets) |
| 📍 GPS | Koordinatės, greitis, aukštis, kryptis, 4 palydovų sistemos, maršrutas, GPX eksportas, dienos statistika | Dalinis |
| 🔋 Energija | SOC, įtampa, srovė, galia, Ah likutis, akum. temp, laikas iki tuščio, 220V šaltinis, dienos Wh apskaita | Taip (Sheets) |
| 🌡️ Aplinka | BME680 temp/drėgmė/slėgis, rasos taškas, oro prognozė, vanduo %, TDS, dujų %, CO₂ miegamasis, žvejybos kortelė | Taip (Sheets) |
| 📡 Ryšys | GSM operatorius, roaming, signalo % ir dBm, Google buferis, rankinis siuntimas | Ne (ESP tik) |
| ⚖️ Lygiavimas | Šoninis ir išilginis posvyris (interaktyvūs SVG), domkratų rekomendacija, burbulų nivyras, kalibracija | Ne (ESP tik) |
| 💬 SMS | ESP32 gaunamų SMS istorija (sms_1–sms_5 text sensoriai) | Taip (cache) |
| ⚙️ Nustatymai | ESP IP, TTS kalba/garsumas/pyptelėjimas, PIN apsauga, Google Sheets ID, vandens/dujų kalibravimas, vibracija, SMS, APN, atnaujinimas | PIN saugomas |
| 🚨 Aliarmai | 10 aliarmų konfigūracija (slenkstis, žodinis tekstas), vagystės balso tekstai (LT/EN) | PIN saugomas |
| 🛠️ Logai | Alarmų istorija, ESP32 būklė, vietinis CSV kaupiklis, sistemos terminalas | Taip |

### 3.2 Navigacija
- Dropdown `<select>` skirtukams + `‹ ›` rodyklės
- Swipe gestai (touchstart/touchend, min. 60px horizontaliai, filtruoja valdiklius)
- Swipe ciklas neapima `settings` ir `alarms` (PIN saugomi)

---

## 4. SSE mechanizmas (kritinis kelias)

### 4.1 Veikimas

```javascript
_sseSource = new EventSource(ESPHOME_URL + '/events');
_sseSource.addEventListener('state', handler);  // duomenų įvykiai
_sseSource.addEventListener('ping', handler);   // 10s heartbeat
_sseSource.addEventListener('log', handler);    // ESP32 logai terminale
```

### 4.2 Sensorių maišymas (ID → cache)

Du lygiai:
1. **SENSOR_ID_MAP** — deterministinis žemėlapis (61 įrašas). `id:` turintys sensoriai.
2. **NFKD fallback grandinė** — ~50 `else if (rawId.includes(...))` eilučių. Reikalingas sensoriais be `id:` (lietuviški slugifikuoti pavadinimai).

Vienu metu naudojami du laukai:
```javascript
let rawId = (data.name_id || data.id).toLowerCase(); // domenui/maišymui
let keyId = (data.id || data.name_id).toLowerCase();  // raktui (švaresnė forma)
```

### 4.3 Watchdog
- 30s be žinučių → `setOfflineMode(true)` → 3s pauze → `startSSE()` perpaleidimas
- Watchdog tikrina kas 15s
- `ping` atnaujina `_lastSseMsg` ir grąžina online jei reikia

---

## 5. Offline Mode

### 5.1 Logika

```
App paleidimas → setOfflineMode(true) [iš karto]
    → _fetchSheetsNow() [Sheets duomenys]
    → startSSE() [po Sheets, per .finally()]
    → onopen → setOfflineMode(false) [kai SSE prisijungia]
```

### 5.2 Offline efektai
- Body pridedama `.offline-mode` klasė (rudas fonas, kortelės su `📵 OFFLINE` ženklu)
- `[data-offline-dim]` kortelės tampa pilkos ir neaktyvios
- `[data-esp-btn]` mygtukai: `disabled=true`
- `[data-esp-switch]` jungikliai: `pointerEvents: none`
- Offline banner su 5 SMS komandų mygtukais: +status, +lokacija, +arm, +disarm, +ikelk

### 5.3 Google Sheets offline sinchronizacija
- Ima tik paskutinę eilutę (`?action=latest`)
- Atnaujina `sensorCache` su ~14 rodiklių
- Rodo timestamp iš `json.timestamp || json.received`
- Kartojama kas 60s kol offline
- Prieš kreipiantis: `netUnbindForInternet()` → 300ms pauzė → `fetch` → `netRebindEsp()`

---

## 6. Balso aliarmų sistema

### 6.1 Aliarmų sąrašas (DEFAULT_ALARMS)

| ID | Grupė | Sąlyga | Reset | Kartojimas |
|----|-------|--------|-------|-----------|
| `water_critical` | water | water_pct < 5% | ≥ 8% | — |
| `water_low` | water | water_pct < 15% | ≥ 20% | — |
| `co2_danger` | co2 | CO₂ > 2500 ppm | < 2000 ppm | 60s |
| `co2_elevated` | co2 | CO₂ 1800–2500 ppm | < 1500 ppm | 60s |
| `soc_critical` | soc | SOC < 15% | ≥ 18% | — |
| `soc_low` | soc | SOC < 25% | ≥ 28% | — |
| `gas_low` | gas | gas_pct < 10% | ≥ 15% | — |
| `theft_alarm` | — | alert_movement_sent === 'true' | === 'false' | 120s važiuojant |
| `high_vibration` | vibration | armed=on + vib_level > 0.4 | < 0.2 | 20s |
| `condensation` | condensation | temp < 8°C + humidity > 65% | temp ≥ 10°C arba hum ≤ 60% | — |

### 6.2 Snooze sistema
- 6 grupės: water, co2, soc, gas, vibration, condensation
- Laikotarpiai: 15/30/60 min (mygtukais)
- Saugoma `localStorage`
- Laikmatukas atnaujina skaičiavimą kas 1s tiksliai

### 6.3 TTS eilė
```javascript
_speechQueue = []  →  _processSpeechQueue()
  → playChime() (jei reikia)
  → speakNow() [KempTts native arba web SpeechSynthesis fallback]
  → 300ms pauzė
  → kitas pranešimas
```
Vagystės aliarmas išvalo eilę ir pertraukia viską (`_speechQueue = []`).

### 6.4 TTS prioritetai
1. Native `KempTts.speak()` (Java `TextToSpeech`)
2. Capacitor `TextToSpeech` plugin
3. Web `SpeechSynthesis` (fallback)

Garsas prieš kalbą pakeliamas iki nustatyto `tts_vol_pct`, po kalbos grąžinamas.

---

## 7. Native Java Bridges

### 7.1 KemperisSms
- `hasPermission()` — SEND_SMS tikrinimas
- `send(number, text)` — SMS siuntimas, Android S+ `SmsManager.class`, senesnė — per subscriptionId
- `getOwnPhoneNumber()` — READ_PHONE_NUMBERS arba READ_PHONE_STATE

### 7.2 KempTts
- Inicializuojamas `MainActivity.onCreate()`
- `onInit()` nustato `UtteranceProgressListener` → JS `window.onTtsDone()`
- `speak(text)` — QUEUE_FLUSH (naujas sakinys pertraukia seną)
- `setLang("lt"|"en")` + išsaugojimas `SharedPreferences`
- `shutdown()` per `onDestroy()`

### 7.3 KemperisWifi
- `getCurrentSSID()` — Android Q+ per `NetworkCapabilities`, senesnė — WifiManager
- `suggestWifi(ssid, pass)` — `WifiNetworkSuggestion` (Android Q+)
- `unbindNetwork()` + `rebindNetwork()` — laikinas atsirišimas internetui (Sheets fetch)
- `openWifiSettings()` — atidaro Android WiFi nustatymus

### 7.4 UpdateBridge
- `getCurrentVersion()` → `CURRENT_VERSION = 14`
- `checkUpdate()` — async thread, unbind → GitHub `version.json` fetch → `window.onUpdateResult()`
- `downloadAndInstall(apkUrl)` — `DownloadManager.enqueue()`, `BroadcastReceiver` (RECEIVER_EXPORTED), notifikacija + `startActivity(installIntent)`
- Keičia tinklą prieš ir po: `bindProcessToNetwork(null)` → fetch → `bindProcessToNetwork(boundNetwork)`

### 7.5 KemperisFileBridge
- `saveTextFile(filename, mimeType, content)` — Android 10+: `MediaStore.Downloads`; senesnė: tiesiai į `DIRECTORY_DOWNLOADS`
- Naudojamas CSV, GPX, terminalo logui išsaugoti

### 7.6 KemperisVol
- `getVolumePct()` / `setVolumePct(pct)` — `STREAM_MUSIC` garsumo valdymas

### 7.7 Auto WiFi bind (MainActivity)
- `NetworkCallback.onAvailable()` — jei WiFi be interneto ir SSID = "Kemperis-Valdymas" (arba `<unknown ssid>`) → `bindProcessToNetwork(network)`
- `onLost()` → `bindProcessToNetwork(null)`

---

## 8. KemperisService (Native FG Service)

```java
// onCreate → startForeground() su IMPORTANCE_LOW notifikacija
// Android 14: ServiceInfo.FOREGROUND_SERVICE_TYPE_SPECIAL_USE
// onStartCommand → START_STICKY (sistema paleidžia iš naujo)
// onBind → null (ne Bound service)
```

**Tikslas:** užtikrinti, kad Android 14+ nelaikytų app kaip nedeklaruotos fono paslaugos. Paleidiamas `MainActivity.onCreate()` pirmiau už Cordova background plugin.

---

## 9. Žemėlapio integracija (Leaflet.js)

- CDN: `https://unpkg.com/leaflet@1.9.4/` (integrity hash tikrinamas)
- **Veikia tik su internetu** — kietai koduotas pranešimas jei `typeof L === 'undefined'`
- Overlay `#map-container` (position:fixed, z-index:2000)
- Maršrutas iš `localLogRows` (filtruojami 0,0 taškai)
- Paskutinis taškas: `L.divIcon` su SVG rodykle pagal `heading`
- **Live markeris** — atnaujinamas kiekvieno `updateUI()` metu kai žemėlapis atidarytas

---

## 10. Vietinis duomenų kaupiklis (CSV)

- Auto-paleidimas `init()` metu (be log pranešimo)
- Adaptyvus intervalas: juda (>3 km/h) → 10s, stovi → 60s
- 4000 eilučių FIFO riba (~420 KB)
- localStorage `kemper_daily_log` + `kemper_log_date`
- 23:59 automatinis išsaugojimas į Downloads
- Nauja diena: auto-save vakarykščiai, `localLogRows = []`
- GPX eksportas iš tų pačių `localLogRows`

---

## 11. Žvejybos kortelė

**Algoritmas (bendra formulė, maks. 100 taškų):**

| Komponentas | Reikšmė | Taškų diapazonas |
|-------------|---------|-----------------|
| Slėgio tendencija | hPa/30min, progresyvinė | 20–90 |
| Bazinis slėgis | nuokrypis nuo 1013 hPa | -25 iki +5 |
| Temperatūra | 5 intervalai | -20 iki +5 |
| Paros laikas | 5 intervalai | -12 iki +20 |
| Mėnulio fazė | jaunatis/pilnatis/ketvirtis | 0–15 |

Spalvos: 85+ = 🔥 Fantastiškas (žalias), 55+ = Geras (mėlynas), <40 = Prastas (raudonas).

---

## 12. Identifikuotos klaidos ir rizikos

### 🔴 Kritinės klaidos

**[K1] `onUpdateResult` — `notesBox` elemento nėra HTML**
```javascript
const notesBox = document.getElementById('update-notes'); // → null
notesBox.style.display = ...  // TypeError: Cannot set properties of null
```
Modalo HTML turi `<div class="notes">` (CSS klasė), bet ne `id="update-notes"`. Kai serveris grąžina versiją su `notes !== ""` — `TypeError` nutraukia `onUpdateResult()`, modalo neberodom.

**[K2] Camper phone saugojimas tik `localStorage`, ne ESP32**
`saveCamperPhone()` rašo tik į localStorage. Jei vartotojas įdiegia naują APK (po atnaujinimo) ir `localStorage` ištrinamas — numerio nebėra. `init()` nustato `_DEFAULT_CAMPER_PHONE = '+37065758821'` bet tai HARDCODED ir gali nesutapti.

**[K3] `_fetchSheetsNow()` — netgi jei offline, `netUnbindForInternet()` iškviečiamas BEZ `await`**
```javascript
// Eilutė 1912:
if (typeof netUnbindForInternet === 'function') netUnbindForInternet(); // atbindame PO 600ms
await new Promise(r => setTimeout(r, 300)); // leidžiame Android apsirūpinti
```
Praeina tik 300ms po unbind. Bet `autoBindPaused` flag lygiagrečiai keičia `UpdateBridge.checkUpdate()` (15s delay). Jei abu vyksta vienu metu — `rebindNetwork()` gali būti iškviestas per anksti arba per vėlai. Tikimybė nedidelė, bet race condition galimas.

**[K4] SSE `keyId` extraction — `number/` prefix stripping netinkamas**
```javascript
let rawId = (data.name_id || data.id).toLowerCase();
let keyId = (data.id || data.name_id).toLowerCase();
// ...
if (rawId.match(/^number[\/\-]/)) {
    sensorCache[keyId.substring(7)] = value;  // "number/" = 7 simboliai
```
Jei `id` formato `number-num_water_empty_cm` (su brūkšniu, ne pasvirtiniu), `substring(7)` grąžina `num_water_empty_cm` teisingai. Bet jei ESPHome ateityje keistų formatą į pvz. `number_num_...` — pjautu netinkamu indeksu. Šiuo metu veikia, bet nestabilu.

---

### 🟡 Vidutinės klaidos

**[V1] `co2_elevated` aliarmų sąlyga turi viršutinę ribą, bet `co2_danger` neturi apatinės**
```javascript
// co2_elevated: CO2 tarp 1800 ir 2500
condition: (c,t) => getFromCache(c,'bedroom_co2',0) > t && getFromCache(c,'bedroom_co2',0) <= 2500,
// co2_danger: CO2 > 2500 (be papildomos patikros)
condition: (c,t) => getFromCache(c,'bedroom_co2',0) > t,
```
Kai CO2 > 2500, abu aliarmai `co2_elevated` ir `co2_danger` gali aktyvuotis vienu metu (elevated sąlyga 1800 < co2 ≤ 2500 yra false >2500, taigi bent logiškai teisingai). Tačiau jei slenkstis pakeičiamas per UI, galimas sutapimas.

**[V2] `power_source_220` tekstų matching — tikslus string palyginimas**
```javascript
if (src220 === 'Isorine 220V') { ... }
else if (src220 === 'Keitiklis') { ... }
else if (src220 === 'Nera 220V') { ... }
```
Bet YAML `v22` siunčia: `"Išorinė 220V"` / `"Keitiklis"` / `"Nėra 220V"`. SSE fallback grandinė:
```javascript
else if (rawId.includes('altinis')) sensorCache['power_source_220'] = value;
```
Kai ESPHome siunčia reikšmę su lietuviška simboliais (`Išorinė`, `Nėra`) — SSE teisingai padeda į cache, bet JS palyginimas su `'Isorine 220V'` / `'Nera 220V'` NIEKADA nesutaps! Tai reiškia, kad 220V kortelė visada rodo `---` arba `undefined`.

> **Pastaba:** Tai galima patikrinti tik vietoje, nes v22.yaml reikšmės nustatomos `text_sensor` `lambda` kode. Jei ESP siunčia ASCII be diakritikų — tada veikia. Rekomenduojama patikrinti.

**[V3] `setNumber()` / `setText()` — atsakymas neverifikuojamas**
```javascript
await fetch(`${ESPHOME_URL}/number/${entityName}/set?value=${v}`, {method:'POST'});
sysLog('✅ Pakeista.', 'success');
```
Net jei ESP grąžina HTTP 400 ar 404 (netinkamas entity pavadinimas) — sysLog rašo "✅ Pakeista". `resp.ok` netikrinamas.

**[V4] `toggleSecurity()` naudoja `_entityRestNames` bet numatytasis gali būti neteisinga forma**
```javascript
const _ssId = _entityRestNames['sw:sw_security_armed'] || 'sw_security_armed';
const _ssResp = await fetch(`${ESPHOME_URL}/switch/${encodeURIComponent(_ssId)}/turn_on|off`...);
```
`_entityRestNames` užpildomas tik kai SSE gauna `switch` tipo įvykį su `data.name_id`. Jei `sw_security_armed` neturėjo `name_id` laukų SSE žinutėse (ESPHome v3 gali ar negali jų siųsti) — krenta į `'sw_security_armed'` fallback. Kuris gali ir neveikti jei REST endpoint tikisi `name_id` formos.

**[V5] Lygiavimo geometrija — domkratų aukščiai skaičiuojami su klaida**
```javascript
let fl = (roll > 0 ? Math.tan(rollRad) * track : 0) + (pitch > 0 ? Math.tan(pitchRad) * wheelbase : 0);
let fr = (roll < 0 ? Math.tan(-rollRad) * track : 0) + (pitch > 0 ? Math.tan(pitchRad) * wheelbase : 0);
```
Tai **netinkama formulė** — naudojamos pilnos `track` ir `wheelbase` reikšmės, bet realybėje FL kampas priklauso nuo pusės pločio (track/2) ir pusės ilgio. Taip pat skaičiavimas nenaudoja keturių kampų tinklelio — nerealistiškai dideli skaičiai. Vizualiai matosi, bet skaitiniai domkratų aukščiai yra netikslūs (x2 kartus per dideli ties šoniniais, x2 per dideli ties išilginiais).

**[V6] GSM dBm konversija yra netikslus aproksimavimas**
```javascript
setSensorValue('gsm_signal_pct', 'gsm-dbm', v=>{ let dbm = Math.round(-113 + v * 0.63); return dbm + ' dBm'; });
```
Tikroji RSSI formulė: `dBm = -113 + 2 * rssi_raw`. Bet ESP siunčia jau procentais, o procento→dBm konversija priklauso nuo originalaus skaičiavimo. Einamas aproksimavimas `-113 + v * 0.63` duoda ~-50 dBm ties 100% — tai rodo gerą signalą, bet standartinis 100% turėtų būti apie -51 dBm (31/31 RSSI). Taip, ±2 dBm klaida — kosmetinė, bet rodo skaičiaus kilmę.

---

### 🟢 Mažos problemos / pasiūlymai

**[M1] `package.json` nurodyta `@capacitor-community/text-to-speech: ^8.0.0`** bet app iš tikrųjų naudoja native `KempTts` Java bridge pirminiame prioritete, o Capacitor plugin tik kaip antrinis variantas. Dependency yra, bet retai naudojamas.

**[M2] Leaflet CDN tinklui nepasiekiamas = žemėlapio nėra visai** — nėra fallback (pvz., lokalus failas `www/lib/leaflet`). Changelog mini `www/lib/` egzistavimą, bet `index.html` naudoja CDN URL.

**[M3] `localLogHeader` CSV stulpeliai neatitinka visiems sensoriai**
```javascript
const localLogHeader = "Laikas,,Platuma,Ilguma,Aukštis,Greitis,Kryptis,SOC,Įtampa,Srovė,Vanduo%,TDS,Dujos%,Temp,Drėgmė,Slėgis,CO2";
```
Antrasis stulpelis tuščias (`,,`) — akivaizdu, kad tai GPS nuorodos vieta (kaip GoogleSheetsScript.js col[1]). Bet CSV faile ji visada bus tuščia (nuoroda generuojama tik Sheets skripte). Tai gali sukelti importo painiavą.

**[M4] `init()` hardcoded private duomenys**
```javascript
const _DEFAULT_SCRIPT_ID = 'AKfycbwavlvHOeFhw0rsnoUQEtBj98mKy4iowtL8U-1fyAetknCbtL4hpKCOfc8swcpW7E4cEQ';
const _DEFAULT_CAMPER_PHONE = '+37065758821';
const _DEFAULT_ADMIN_PHONE  = '+37064730025';
```
Visi trys yra atviri kode. Google Apps Script ID eksponuotas — teoriškai leidžia rašyti į Sheets be autentifikacijos (jei skriptas public). Tel. numeriai viešai matomi kode.

**[M5] `vib_level` slenkstis UI default 35, bet aliarmų threshold = 0.4**
Settings kortelėje: `getCache('num_vib_threshold', 35)` — numatytoji 35 EMA vienetai. Bet `DEFAULT_ALARMS.high_vibration.threshold = 0.4`. Skirtingos skalės! Tas pats sensorius `vib_level_sensor` → `vib_level` cache. Jei `vib_level` siunčia tikrus EMA vienetus (dešimtys), aliarmo sąlyga `> 0.4` visada bus `true` kai armed. Bet jei YAML siunčia normalizuotą (0–1 skalė) — tuomet UI setting `35` niekada nebus naudojamas.

> Reikia patikrinti v22.yaml `vib_level_sensor` reikšmės skalę.

**[M6] Žvejybos mėnulio fazė naudoja UTC 2000-01-06 kaip nulinę jaunatį** — matematiškai teisingai, bet nauji mėnuliai nesutampa su tikslesniais astronominiais skaičiavimais ±1 dieną. Pakankama žvejybai.

**[M7] `escapeHtml()` naudojama `renderAlarmHistory()` bet funkcija niekur neapibrėžta**
```javascript
return '<div>...' + escapeHtml(e.title) + '...'
```
Jei `escapeHtml` neapibrėžta — `ReferenceError` kiekvieną kartą rodant alarm historiją. Patikrinti ar ji apibrėžta kitur faile (po šios eilutės).

---

## 13. Firmware ↔ App atitikimas

| Sensorius | v22.yaml `id:` | App `SENSOR_ID_MAP` | Atitinka? |
|-----------|---------------|---------------------|-----------|
| `junc_soc` | ✅ | ✅ | ✓ |
| `bedroom_co2` | ✅ | ✅ | ✓ |
| `bedroom_temp` / `bedroom_humidity` | ✅ | ✅ | ✓ |
| `alert_movement_sent_sensor` | ✅ (v22 naujas) | ✅ | ✓ |
| `modem_state_sensor` | ✅ (v22 naujas) | ❌ **nėra SENSOR_ID_MAP** | ✗ |
| `last_error_sensor` | ✅ (v22 naujas) | ❌ **nėra SENSOR_ID_MAP** | ✗ |
| `sw_debug_log` | ✅ (v22 naujas) | ❌ **nėra UI** | ✗ |
| `txt_kemperis_number` | ✅ (v22 naujas) | ❌ **nėra UI** | ✗ |
| `num_timezone_offset` | ✅ | ✅ | ✓ |
| `power_source_220` | ✅ | ✅ (bet žr. [V2]) | Rizika |

**`modem_state_sensor` ir `last_error_sensor`** — v22 pridėti nauji text sensoriai kurie rodo modemo būseną ir paskutinę klaidą. App nėra šiems skirtų UI elementų ir jie nėra SENSOR_ID_MAP. Fallback grandinėje jie nebus atpažinti — duomenys prarandami. Tai verbalingo informacijos šaltinis debugging tikslais.

---

## 14. Saugumo pastabos

1. **Cleartext HTTP** — `androidScheme: 'http'` + `usesCleartextTraffic="true"` — visas ESP32 srautas nešifruotas. Pagrįsta lokaliam WiFi AP, bet svarbu žinoti.
2. **PIN apsauga tik JS** — PIN tikrinamas tik JavaScript lygmenyje. Native Android lygmeniu jokio papildomo apsaugos sluoksnio.
3. **SMS arm/disarm patikra** — `sendOfflineSms('+arm')` tikrina ar siuntėjo numeris = admin numeris. Jei `getOwnPhoneNumber()` grąžina `""` (daugelyje Android 10+ be leidimo) — komanda siunčiama be patikros su `⚠️` įspėjimu. Tai galimas saugumo plyšys.
4. **Google Apps Script ID kode** — žr. [M4].

---

## 15. Stipriosios pusės

- **Dvigubas TTS prioritetų kelias** — native → Capacitor → web fallback su eilės sistema
- **Snooze sistema** — 6 grupės, 3 trukmės, localStorage persistencija
- **SSE watchdog + auto-reconnect** — solidus 30s timeout, 3s rebind delay
- **Offline mode** — išsamus UI blokavimas, Sheets sinchronizacija, SMS komandos
- **WiFi auto-bind** — `NetworkCallback` susieja procesą su Kemperis AP net kai mobilūs duomenys įjungti
- **Alarmų konfigūracija** — slenkstis ir tekstas keičiamas per UI, saugoma localStorage
- **KemperisFileBridge** — teisingai naudoja `MediaStore` Android 10+ ir seną API senesnėse
- **UpdateBridge** — pilnas srautas: tikrinimas → parsisiuntimas → notifikacija → diegimas; teisingas `RECEIVER_EXPORTED`
- **Background mode** — `KemperisService` (START_STICKY) + Cordova plugin = aliarmų veikimas ekraną išjungus

---

## 16. Prioritetinis taisymų planas

### P1 — Iš karto (funkcionalo klaidos)

1. **[K1] `update-notes` ID** — pakeisti CSS klasę į `id="update-notes"` HTML modalo `<div class="notes">` elemente
2. **[V2] `power_source_220` tekstų palyginimas** — patikrinti tikras v22 siunčiamas reikšmes ir arba normalizuoti (`.trim().toLowerCase().replace(/[^a-z0-9]/g,'')`), arba suderinti hardcoded stringus
3. **[M7] `escapeHtml` apibrėžimas** — patikrinti ar funkcija egzistuoja, jei ne — pridėti

### P2 — Greitai (kokybė)

4. **[V3] `setNumber()`/`setText()` HTTP klaidų patikra** — pridėti `if (!resp.ok) throw new Error(...)` 
5. **[V5] Domkratų aukščių formulė** — naudoti `track/2` ir `wheelbase/2`
6. **`modem_state_sensor` ir `last_error_sensor`** — pridėti į SENSOR_ID_MAP ir bent parodyti Ryšio skirtuke

### P3 — Ateičiai

7. **[M4] Privatūs duomenys kode** — perkelti į `localStorage` defaults (nekeisti kode)
8. **[M5] `vib_level` skalės išaiškinimas** — suderinti UI slenkstį su aliarmo slenkščiu
9. **[K2] Camper phone** — sinchronizuoti su ESP32 `txt` entity (kaip `txt_admin_number`)
10. **Leaflet lokaliai** — nukopijuoti į `www/lib/` kad žemėlapis veiktų be interneto

---

## 17. Santrauka

KemperisApp v14 yra **brandus, funkcionalus projektas** su gerai apgalvota dvigubo režimo (online/offline) architektūra, išsamia aliarmų sistema ir solidžiais native Android bridges. Pagrindinis kodo srautas yra stabilus ir veikia.

Identifikuotos klaidos yra **lokalizuotos ir taisomos** — nė viena nekelia sistemos klaidų kas sekundę. Didžiausia rizika: `power_source_220` tekstų mismatch ([V2]) gali būti tylioji klaida (kortelė rodo `---` bet nesigirdi klaidos). Antra prioritetinė: `update-notes` null dereference ([K1]) — veikia tol, kol GitHub `notes` laukas tuščias.

Firmware v22 ir App v14 sankirta yra gerai — visi pagrindiniai sensoriai atpažįstami. Du nauji v22 text sensoriai (`modem_state`, `last_error`) nėra padengtai UI, bet tai ne kritinė spraga.
