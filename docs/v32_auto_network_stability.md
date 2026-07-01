# v32 — užduotis asistentui: Auto režimo tinklo stabilumas (ESP per WiFi + internetas per LTE vienu metu)

## Tikslas
Auto režime įrenginys turi **stabiliai** imti ESP duomenis per ESP WiFi (SSE nemirksi), o interneto užklausas (Google Sheets, GitHub) leisti per mobilų internetą (Tele2/LTE) — **abu vienu metu**, be „orkestracijos" blaškymosi.

## Šaknis (dabartinė problema)
Auto režime app naudoja `bindProcessToNetwork` unbind→internetas→rebind ciklą (`netUnbindForInternet`/`netRebindEsp`). Kiekvienas perjungimas **nutraukia SSE** (WebView). Paleidžiant procesas ~25 s laikomas atrištas versijos patikrai → SSE krenta. Logai: „🔄 Tinklo orkestracija: Grįžta prie ESP32 WiFi", „SSE ryšys nutrūko", „⚡ Priverstinis prisijungimas gyvai".

## Kodėl blaškymasis nereikalingas
Visos interneto užklausos **jau** eina per native `getSafeConnection` (v31.1), kuris atskirai atidaro ryšį per **interneto tinklą (LTE)**, nepriklausomai nuo proceso pririšimo:
- Sheets skaitymas: `_fetchSheetsNow` → `nativeHttpGet` → `KemperisNet.httpGet` → `doHttp` → `getSafeConnection`.
- Versijos patikra: `KemperisUpdate.checkUpdate` → `getSafeConnection`.

Tad atrišinėti visą procesą — beprasmiška ir žalinga. **Dual‑network modelis:** WebView/SSE lieka pririšti prie ESP WiFi (lokalus 192.168.4.1 subnet'as), o internetą native bridge'ai atidaro per LTE.

## Principas
1. Procesą pririšti prie ESP WiFi **vieną kartą ir laikyti** (auto `NetworkCallback` MainActivity.java jau tai daro `onAvailable`; `onLost` atriša kai ESP WiFi dingsta — palikti).
2. Visą internetą leisti **tik per native `getSafeConnection` bridge'us** (`KemperisNet.httpGet`/`httpPost`).
3. **Pašalinti** JS `netUnbindForInternet`/`netRebindEsp` blaškymąsi.

## Keisti — `www/index.html` (native Java NELIESTI, ji jau teisinga)

### 1. Paleidimo versijos patikra — pašalinti unbind/rebind
`init()` (~2406–2418 eil.). Dabartinis blokas atriša, laukia 25 s, kviečia checkUpdate, tada rebind. Pakeisti į paprastą kvietimą (checkUpdate jau naudoja getSafeConnection):
```javascript
setTimeout(() => {
    if (window.KemperisUpdate) {
        try { window.KemperisUpdate.checkUpdate(); } catch(e){}
    }
}, 15000);
```
(Pašalinti `netUnbindForInternet()`, `await ...`, `netRebindEsp()`, „Tinklo orkestracija" sysLog.)

### 2. `uploadLocalDataToGoogle` — per native httpPost, be unbind
(~1240 eil.) Vietoj WebView `fetch(url, {method:'POST', mode:'no-cors', ...})` naudoti native bridge (maršrutizuoja per LTE per getSafeConnection). Pašalinti `netUnbindForInternet()`/`netRebindEsp()`:
```javascript
async function uploadLocalDataToGoogle() {
    if (localLogRows.length === 0) { sysLog('Nėra duomenų.', 'warn'); return; }
    const scriptId = document.getElementById('txt_app_script_id').value;
    if (!scriptId || scriptId.includes('...')) { sysLog('Nenurodytas Script ID.', 'error'); return; }
    const csvContent = localLogHeader + "\n" + localLogRows.join("\n");
    const url = `https://script.google.com/macros/s/${scriptId}/exec`;
    sysLog(`Siunčiama ${localLogRows.length} eilučių...`, 'info');
    const btn = document.getElementById('btn-upload-google'); if (btn) btn.disabled = true;
    if (window.KemperisNet && KemperisNet.httpPost) {
        const cb = 'cb_up_' + Math.random().toString(36).slice(2);
        window[cb] = function(res) {
            sysLog('✅ Duomenys išsiųsti (per LTE).', 'success');
            if (btn) btn.disabled = false; try { delete window[cb]; } catch(e){}
        };
        KemperisNet.httpPost(url, csvContent, cb);
    } else {
        // web fallback (naršyklė / testas)
        try { await fetch(url, { method:'POST', mode:'no-cors', headers:{'Content-Type':'text/plain'}, body: csvContent }); sysLog('✅ Duomenys išsiųsti.', 'success'); }
        catch(e) { sysLog('❌ Klaida: ' + e.message, 'error'); }
        finally { if (btn) btn.disabled = false; }
    }
}
```

### 3. `checkInternetAndEnableUpload` — interneto testas per native, be unbind
(~1222 eil.) Vietoj WebView `fetch('https://www.google.com')` naudoti native httpGet per getSafeConnection; pašalinti unbind/rebind. Pvz.:
```javascript
async function checkInternetAndEnableUpload() {
    sysLog('Tikrinamas interneto ryšys...', 'info');
    const btn = document.getElementById('btn-upload-google');
    if (window.KemperisNet && KemperisNet.httpGet) {
        const cb = 'cb_net_' + Math.random().toString(36).slice(2);
        window[cb] = function(res) {
            const ok = res && !res.includes('"error"');
            sysLog(ok ? '✅ Internetas veikia.' : '❌ Nėra interneto.', ok ? 'success' : 'error');
            if (btn) btn.disabled = !ok; try { delete window[cb]; } catch(e){}
        };
        KemperisNet.httpGet('https://www.gstatic.com/generate_204', cb);
    } else if (btn) { btn.disabled = false; }
}
```

### 4. `netUnbindForInternet` / `netRebindEsp`
- Pašalinti **visus** `netUnbindForInternet()` kvietimus.
- `netRebindEsp()` kvietimą `startSSE()` pradžioje (~2450 eil.) **palikti** (užtikrina, kad procesas pririštas prie ESP WiFi prieš SSE).
- `_netOrchestrationLock` — kadangi orkestracijos nebelieka, jis lieka `false`; su tuo susiję saugikliai (`if (_netOrchestrationLock) return`) netrukdys. Galima palikti kaip yra.

## Priėmimas (testuoti telefone su LTE + ESP WiFi vienu metu)
- Auto režime: SSE stabilus (nemirksi), kortelės pilnos, pokrypis atsinaujina gyvai; po „Kalibruoti nulį" pokrypis → ~0.
- Google Sheets skaitymas/rašymas veikia per LTE, kol prisijungęs prie ESP WiFi (nereikia atsijungti nuo ESP).
- Loge nebėra „Tinklo orkestracija" unbind/rebind ciklų per normalų veikimą.
- Įrenginyje **be** interneto (planšetė): SSE vis tiek stabilus per ESP WiFi (internetinės funkcijos tyliai nepavyksta, bet ESP duomenys rodomi).

## Užbaigimas
1. `versionName` → „32" (`versionCode` 32).
2. `cd android/android && ./gradlew assembleRelease`
3. `app-release.apk` → `android/kemperis_v31.apk` (arba pervadinti į v32; jei keiti pavadinimą — atnaujinti `version.json` apk_url), `git add -f`, commit + push.

> Pastaba: native `MainActivity.java` (getSafeConnection, doHttp, auto NetworkCallback, unbind/rebind bridge metodai) **NEKEISTI** — jie teisingi; keičiasi tik kada JS juos kviečia.
