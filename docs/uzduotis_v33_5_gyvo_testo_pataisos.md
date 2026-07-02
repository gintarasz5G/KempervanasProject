# Užduotis v33.5 (v41): Renogy init fix + gyvo testo pataisos

**Data:** 2026-07-02 (perrašyta po pakartotinės kodo peržiūros — visi eilučių numeriai
ir kodo fragmentai patikrinti prieš HEAD v40) · **Bazė:** commit 1df88f8
**Kontekstas:** `docs/audits/gyvo_testo_analize_2026-07-02.md`. Gyvas testas patvirtino
SSE ir WiFi bind; Renogy tab'as tyliai neveikė — priežastis rasta ir patvirtinta
plugin'o Kotlin šaltinyje (`BluetoothLe.kt` 106/116/139 eil.).

## 0. Prieš pradedant (privaloma)

`git status` švarus; patikrinti, kad failai nenukirsti (index.html baigiasi `</html>`,
renogy.js — `RenogyBLE.init()` bloku). Po darbų prieš commit: `git diff --stat` —
jei kurio failo diff'as vien ištrynimai gale, failas nukirstas, necommitinti.

## 1. 🔴 Renogy init fix (svarbiausia)

### 1a. Viena eilutė — `renogy.js:67`
```js
// buvo: await BleClient.initialize();
await BleClient.initialize({ androidNeverForLocation: true });
```
**Kodėl:** be šio parametro plugin'as Android 12+ reikalauja `ACCESS_FINE_LOCATION`
(BluetoothLe.kt 116 eil.), kurios manifest'as nedeklaruoja SDK 31+ (`maxSdkVersion=30`)
→ Android atmeta be dialogo → `call.reject("Permission denied.")` (139 eil.) → init
lūžta. Originali Renogy app jungiasi, nes naudoja neverForLocation kelią.
**Manifest'o NELIESTI — jis teisingas.**

### 1b. Klaidų matomumas — per BENDRĄ logą + esamus elementus (BE naujų UI)
Visos renogy.js klaidos dabar matomos tik su debug jungikliu — todėl vartotojas matė
„nieko nevyksta". Naujų UI elementų NEKURTI (vartotojo sprendimas) — naudoti tai, kas yra:

1. **Klaidos — VISADA į bendrą logą** (ne tik debug). `renogy.js` `debug()` helper'į
   papildyti antru variantu arba tiesiog visose `catch` šakose vietoj `debug(...)`
   kviesti `window.sysLog && window.sysLog('[Renogy] ' + msg, 'error')` be
   `isDebugEnabled` sąlygos. Informaciniai žingsniai (polling, kadrai) lieka
   debug-gated kaip dabar — kad bendro logo neužterštų.
2. **Skenavimo atsakas — į esamą `renogy-scan-list` konteinerį** (kur vartotojas žiūri
   paspaudęs mygtuką): pradžioje `container.innerHTML = '🔍 Skenuojama…'`; pasibaigus be
   rezultatų — `'Nieko nerasta. Patikrinkite BT leidimus (žr. Logs)'`; klaidos atveju —
   `'❌ Klaida: <žinutė>'`. Radus įrenginius — esamas sąrašo renderis (neliesti).
3. **Ryšio būsenos — esami `ren_lbl_bat` / `ren_lbl_dcc`** elementai: „Jungiamasi…" /
   „Prisijungta" / „Atjungta" / „Klaida" (jei jie dar to nerodo — papildyti updateUI).
4. `init()` nesėkmės atveju (plugin nerastas / Permission denied) — vienkartinis
   `sysLog` įrašas IR `renogy-scan-list` konteineryje statinis tekstas
   `'❌ BLE neinicijuotas: <priežastis>'`, kad tab'as niekada neatrodytų „tuščiai miręs".

## 2. CSV header — `index.html:780`
```js
const localLogHeader = "Laikas,,Platuma,Ilguma,Aukštis,Greitis,Kryptis,SOC,Įtampa,Srovė,Vanduo%,TDS,Dujos%,Temp,Drėgmė,Slėgis,CO2,Ren SOC,Ren V,Ren A,Ren Temp,DCC Saulė W,DCC Gen W,DCC Stadija";
```
(17 → 24 pavadinimai; tvarka atitinka eilutės laukus: ren_soc, ren_v, ren_a, ren_temp,
dcc_solar_w, dcc_alt_w, dcc_stage. R5 padding tada veiks teisinga kryptimi.)

## 3. SOC=101 fix — `index.html` ~1561–1569 („#SOC FIX" blokas)
Blokas visada perrašo firmware SOC be ribos (komentaras žada perrašyti tik kai SOC
tuščias, bet sąlyga to netikrina). Pakeisti bloko vidų:
```js
let ah = parseFloat(sensorCache['junc_ah_remaining']);
let cap = parseFloat(sensorCache['num_bat_capacity'] || 110);
let soc = parseFloat(sensorCache['junc_soc']);
if ((isNaN(soc) || soc <= 0) && !isNaN(ah) && ah > 0.5 && cap > 1) {
    sensorCache['junc_soc'] = Math.min(100, Math.round((ah / cap) * 100));
}
```

## 4. `[SSE BRANCH]` šalinimas — `index.html:2754`
SSE gyvai patvirtinta (2026-07-02 21:31 logas) — pašalinti vienintelę eilutę
`debugLog('[SSE BRANCH] ' + rawId, 'success');`. **`[SSE UNMATCHED]` PALIKTI** (naudingas).

## 5. `loadAllNumbers` triukšmo mažinimas — `index.html` ~2158+
Guard'ai (`_isOffline`, `_loadInProgress`) JAU yra — spam kyla tik ~90 s tarpe, kol
watchdog paskelbia offline: 17 id nuosekliai fail'ina su „Failed to fetch".
Taisymas — po pirmos `Failed to fetch` klaidos nutraukti VISĄ ciklą (jei vienas id
nepasiekiamas, nepasiekiami visi):
```js
} catch(e) {
    if (String(e && e.message).includes('Failed to fetch')) {
        debugLog('loadAllNumbers: ryšio nėra, ciklas nutrauktas');
        break;
    }
}
```
ir „❌ Nepavyko gauti…" sysLog perkelti į `debugLog` (nebūtinas vartotojui).

## 6. SSE šakos (modemo_busena / paskutine_klaida) — **NEDARYTI**
Patikrinta: UI tų kortelių neturi. UNMATCHED loge jiems — norma.

## Versijos
`version.json 41` == `versionCode 41` == `CURRENT_VERSION 41` (MainActivity) ==
`android/kemperis_v41.apk` == `apk_url`; `versionName "33.5"`; notes su changelog.

## Priėmimo kriterijai
1. **BE debug jungiklio**: paspaudus „Ieškoti" scan-list konteineryje matosi
   „Skenuojama…" → rezultatai arba aiški klaida; init nesėkmė matosi ir konteineryje,
   ir bendrame loge (`[Renogy]` su 'error'); jokia klaida nebėra nebyli.
2. Vartotojo planšetėje: scan randa `BT-TH-xxxx` + bateriją, priskyrus — gyvos reikšmės
   ≤15 s; celės ~3.3–3.6 V; palyginimas su Renogy app per OFF jungiklį (≤5 s).
3. CSV eksporto header = 24 pavadinimai; visos eilutės lygios (padding veikia).
4. UI/CSV SOC niekada nerodo >100.
5. Loge nebėra `[SSE BRANCH]`; `[SSE UNMATCHED]` liko; offline nėra num_* klaidų spam.
6. Versijų grandinė pilna; `git diff --stat` be nukirstų failų; commit + push +
   audito įrašas `docs/audits/`.

## NEDARYTI
Firmware neliesti. Renogy parserio offsetų/mastelių neliesti (validuoti 16/16 testais).
Manifest'o neliesti. `ren-bt-alert` kortelės neliesti (veikia). Apps Script neliesti.
