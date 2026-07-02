# v32.3 — užduotis asistentui: apk_url, dubliuotas onopen, cloud aliarmai

---

## 1. apk_url / APK failo vardo suderinimas
Dabar `version.json` `apk_url` rodo į `kemperis_v34.apk`, o repo yra `kemperis_v33.apk` (nesutampa → in-app update download lūžtų).

**Sprendimas (pasirinkti vieną, kad sutaptų):**
- **Rekomenduojama:** pervadinti `android/kemperis_v33.apk` → `android/kemperis_v34.apk` (versionCode ir version.json jau 34), `git add -f`.
- Arba: `version.json` `apk_url` nukreipti į `kemperis_v33.apk`.

**Taisyklė ateičiai:** APK failo vardas turi atitikti `apk_url` ir `versionCode`.

---

## 2. Dubliuotas `_sseSource.onopen` (sujungti)
`www/index.html`: yra du `_sseSource.onopen` priskyrimai (~2459 ir ~2590). Antras **perrašo** pirmą, tad pirmojo unikali logika **prarandama** — svarbiausia `_sseRetryCount = 0;` (retry skaitiklio atstatymas po sėkmingo prisijungimo).

**Sprendimas:** palikti **vieną** onopen, kuriame yra visa logika (ypač `_sseRetryCount = 0`), o dublį pašalinti:
```javascript
_sseSource.onopen = () => {
    _lastSseMsg = Date.now();
    _sseRetryCount = 0;                                   // <-- iš #1, būtina
    document.getElementById('status-dot').classList.remove('offline');
    document.getElementById('status-text').textContent = 'Prisijungta';
    if (_isOffline) sysLog('✅ Ryšys su ESP32 atkurtas!', 'success');
    setOfflineMode(false);
};
```
Pašalinti antrą (dubliuotą) `_sseSource.onopen = () => { ... }` (~2590).

---

## 3. Cloud aliarmai (funkcija su saugikliais)
Tikslas: kai nėra tiesioginio ESP ryšio (offline/cloud), įspėti iš Google duomenų — **bet tik saugiai** (švieži duomenys, be vagystės, su „debesys" žyme). **Numatyta IŠJUNGTA** (vartotojas įsijungia).

**Saugikliai:**
- Įjungiama tik per nustatymą (default off).
- Tik jei cloud duomenys **švieži** (≤ 20 min pagal `json.timestamp`).
- Tik **whitelist** aliarmai (vanduo, SOC, dujos, kondensatas) — **NE vagystė** (jos duomenų cloud'e nėra ir ji reikalauja gyvo ryšio).
- Balse pridedama „(debesys, prieš X min)", kad vartotojas žinotų, jog vėluota.
- Naudoja tuos pačius `alarmFlags` (dedup, be spam'o).

### 3a. Nustatymo jungiklis (`www/index.html`, Nustatymų skiltis)
```html
<div class="card"><h3>☁️ Aliarmai iš debesų</h3>
  <div style="display:flex; align-items:center; gap:10px;">
    <label class="switch"><input type="checkbox" id="chk_cloud_alarms" onchange="toggleCloudAlarms(this)"><span class="slider round"></span></label>
    <span style="font-size:13px;">Įspėti iš Google duomenų, kai nėra tiesioginio ESP ryšio (tik švieži ≤20 min).</span>
  </div>
</div>
```
JS:
```javascript
function toggleCloudAlarms(el){ localStorage.setItem('cloud_alarms_enabled', el.checked ? 'true' : 'false'); }
```
`init()` gale: `const cc = document.getElementById('chk_cloud_alarms'); if (cc) cc.checked = localStorage.getItem('cloud_alarms_enabled') === 'true';`

### 3b. Šviežumo pagalbinė + cloud aliarmų tikrinimas (`www/index.html`)
```javascript
const CLOUD_ALARM_IDS = ['water_critical','water_low','soc_critical','soc_low','gas_low','condensation']; // BE theft_alarm
const CLOUD_ALARM_MAX_AGE_MIN = 20;

function _cloudDataAgeMin(ts) {
    if (!ts) return null;
    try {
        const d = new Date(String(ts).replace(' ', 'T'));   // "YYYY-MM-DD HH:MM:SS" (lapų TZ = įrenginio TZ)
        if (isNaN(d.getTime())) return null;
        const age = (Date.now() - d.getTime()) / 60000;
        return (age >= 0 && age < 1440) ? age : null;        // atmesti neigiamą/absurdišką
    } catch(e) { return null; }
}

function checkCloudAlarms(cache, ageMin) {
    const now = Date.now();
    ALARM_CONFIG.forEach(alarm => {
        if (!CLOUD_ALARM_IDS.includes(alarm.id)) return;      // tik whitelist, be vagystės
        const thr = alarm.threshold;
        const isSnoozed = alarmSnooze[alarm.id] && alarmSnooze[alarm.id] > now;
        if (alarm.condition(cache, thr)) {
            if (!alarmFlags[alarm.id]) {
                alarmFlags[alarm.id] = true;
                alarmLastTriggered[alarm.id] = now;
                _addAlarmHistory(alarm, cache);
                if (!isSnoozed) speakText(alarmMsg(alarm) + ' (debesys, pries ' + Math.round(ageMin) + ' min)', true);
            } else if (alarm.repeatInterval && (now - (alarmLastTriggered[alarm.id]||0) >= alarm.repeatInterval)) {
                alarmLastTriggered[alarm.id] = now;
                if (!isSnoozed) speakText(alarmMsg(alarm) + ' (debesys)', true);
            }
        } else if (alarmFlags[alarm.id] && alarm.reset(cache, thr)) {
            alarmFlags[alarm.id] = false;
            if (alarmSnooze[alarm.id]) { delete alarmSnooze[alarm.id]; try { localStorage.setItem('_alarmSnooze', JSON.stringify(alarmSnooze)); } catch(e){} }
        }
    });
    try { localStorage.setItem('_alarmFlags', JSON.stringify(alarmFlags)); } catch(e){}
    try { localStorage.setItem('_alarmLastTriggered', JSON.stringify(alarmLastTriggered)); } catch(e){}
}
```

### 3c. Iškvietimas `_fetchSheetsNow` pabaigoje (po `updateUI()`)
```javascript
try {
    if (localStorage.getItem('cloud_alarms_enabled') === 'true') {
        const ageMin = _cloudDataAgeMin(json.timestamp || json.received);
        if (ageMin !== null && ageMin <= CLOUD_ALARM_MAX_AGE_MIN) checkCloudAlarms(sensorCache, ageMin);
    }
} catch(e){}
```

**NEKEISTI:** `checkAlarms` (gyvų duomenų kelias su `if (_isOffline) return`) — lieka kaip yra. Cloud aliarmai — atskiras kelias, tik kai įjungta.

**Priėmimas:**
- Numatyta: cloud aliarmai išjungti — elgesys nepakinta.
- Įjungus + esant offline su šviežiais (≤20 min) cloud duomenimis, kur reikšmė virš ribos (pvz. vanduo < riba) → balsas su „(debesys, prieš X min)".
- Pasenę (>20 min) cloud duomenys → **jokio** aliarmo.
- Vagystė per cloud **niekada** nesuveikia.

---

## Užbaigimas
1. Versijos: `CURRENT_VERSION` == `versionCode` == `version.json version` (pvz. 35); `versionName` „32.3". APK vardas = apk_url = versionCode.
2. `cd android/android && ./gradlew assembleRelease`
3. APK → `android/kemperis_v35.apk`, `version.json apk_url` → tas pats, `git add -f`, commit + push.
