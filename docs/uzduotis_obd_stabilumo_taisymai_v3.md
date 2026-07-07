# Užduotis: v51 patikrinimas ir Renogy BLE rašymo klaidos taisymas

**Statusas:** kodo pakeitimai (OBD apribojimas, Renogy temp. jutikliai, Renogy write fix'as)
YRA `main` šakoje (commit `a1efe0a`), bet **NĖ VIENAS DAR NEPATVIRTINTAS realiu telefono
testu** — telefone šiuo metu veikia SENA `35.3` (versionCode 50) APK, kurioje šių pakeitimų
NĖRA. Vykdyk žingsnius tiksliai šia tvarka.

---

## Žingsnis 0 (PRIVALOMAS PIRMAS): sugeneruoti ir patikrinti naują APK

1. `android/android/app/build.gradle` — `versionCode` 50 → **51**, `versionName` „35.3" → **„35.4"**.
2. `android/android/app/src/main/java/lt/kemperis/app/MainActivity.java` — `CURRENT_VERSION` → **51**.
3. `version.json` (repo šaknyje) — `"version"` → **51**, atnaujinti `notes` su changelog (žr. Žingsnis 5).
4. Vykdyti IŠ `android/` aplanko, TIKSLIAI šia tvarka:
   ```
   npx cap sync
   cd android
   ./gradlew assembleRelease
   ```
5. **Patikrinti, kad naujas web kodas realiai pateko į APK** (žinoma pasikartojanti klaida
   šiame projekte — jei praleisi `cap sync`, APK turės seną kodą nepaisant naujo versionCode):
   ```
   unzip -p android/kemperis_v51.apk assets/public/index.html | md5sum
   md5sum android/www/index.html
   ```
   MD5 sumos turi SUTAPTI. Jei nesutampa — pakartoti nuo 4 žingsnio.
6. Įdiegti APK telefone. Programėlės nustatymuose patikrinti, kad rodoma versija „35.4",
   NE „35.3".

**NEDARYTI** `git commit`/`push` po šio žingsnio be aiškaus vartotojo prašymo.

---

## Žingsnis 1: realus testas — OBD

Su nauja APK (35.4), prijungtu ELM327 adapteriu, paspausti „🚀 Surinkti VISKĄ".

**Tikrintina:**
- Žurnale NĖRA `0902`/`0904`/`090A` (Mode 09) komandų.
- Žurnale NĖRA `0x700-0x7FF` adresų sekos (modulių skenavimas).
- Jei modulių skenavimas vis dėlto paleidžiamas rankiniu mygtuku „4️⃣" — patikrinti, kad
  `ATCS` komanda pasirodo TIK du kartus (prieš ir po `for` ciklo), NE kas 25 adresus.
- Standartinis Mode 01 PID polling'as (`010C`, `010D` ir t.t.) NEPASILEIDŽIA automatiškai
  po inicijavimo ar po „Surinkti VISKĄ".

**Failas jei problema:** `android/www/obd.js`, funkcijos `runFullCollection()`,
`runModuleScanInternal()`, `startPolling()`.

---

## Žingsnis 2: realus testas — Renogy temperatūros jutikliai

Su nauja APK, prijungus prie Renogy baterijos per BLE, stebėti Renogy kortelę ≥10 min.

**Tikrintina:**
- Temperatūros jutikliai 3 ir 4 NEBERODOMI kaip „0.0°C" (arba paslėpti, arba nerodomi
  `sensorCache` viduje — patikrinti per debug žurnalą, kad `ren_temp_2`/`ren_temp_3`
  neegzistuoja objekte, kai `data[34..35]` registro reikšmė < 3/4).
- Jutikliai 1 ir 2 (arba tiek, kiek realiai yra) rodo prasmingas reikšmes (~15-30°C
  diapazone, ne 0).

**Failas jei problema:** `android/www/renogy.js`, `parseFrame()`, `len === 0x44` šaka.

---

## Žingsnis 3: realus testas — Renogy „Writing characteristic failed"

Su nauja APK, tuo pačiu ≥10 min. testu kaip Žingsnis 2, suskaičiuoti
`grep -c "battery query failed" <naujas terminal_log>`.

### Jei klaidų SKAIČIUS = 0 arba žymiai sumažėjęs (pvz. <2 per 10 min)

`writeWithoutResponse` sprendimas (jau kode) veikia pakankamai gerai. Taisymas baigtas,
pereiti prie Žingsnio 4.

### Jei klaida IŠLIEKA tuo pačiu dažniu (≥5 per 10 min)

**LEMIAMAS RADINYS (nauja paieška):** ta pati klaida, TIKSLIAI tuo pačiu elgesiu (rašymas
„klysta", bet duomenys vis tiek sėkmingai ateina), yra užfiksuota [cyrils/renogy-bt
issue #80](https://github.com/cyrils/renogy-bt/issues/80) — **VISIŠKAI KITOJE platformoje**
(Python, Raspberry Pi, ne Android/Capacitor) su **TIKSLIAI TA PAČIA Renogy PRO baterijų
serija** (ten `RBT12200LFP-BT`, mūsų atveju `RNGPRO12BATT49000437` — abu „Pro" serijos
modeliai). Ten pranešama apie `characteristic_write_value_failed` IR
`characteristic_enable_notifications_failed` klaidas, bet baterija VIS TIEK sėkmingai
grąžina pilnus duomenis (celių įtampas, temperatūras, srovę). Tai reiškia:

**Tai NĖRA nei mūsų kodo, nei konkrečiai `@capacitor-community/bluetooth-le` plugin'o
klaida** — tai **Renogy PRO baterijos BLE modulio (BMS mikroschemos) pačios aparatūros/
programinės įrangos ypatumas**: jos GATT rašymo patvirtinimo (write acknowledgment) atsakymas
ateina pavėluotai arba netinkamu formatu, todėl BET KURI kliento biblioteka (Python, Android,
ar bet kuri kita), reikalaujanti standartinio patvirtinimo, praneša klaidą — nepriklausomai
nuo platformos. Duomenys vis tiek pasiekia klientą per atskirą notifikacijos kanalą.

**Taigi rekomenduojama tvarka apversta atvirkščiai nei buvo manyta anksčiau:**

**3a (DABAR pagrindinis sprendimas). Priimti kaip žinomą aparatūros ypatumą, sumažinti
žurnalo triukšmą.** `android/www/renogy.js` `queryDevice()` — pakeisti klaidos žurnalavimo
lygį iš `'error'` į `'warn'` (arba visai nerodyti pagrindiniame vartotojo žurnale, palikti
tik debug režimui), su komentaru kode:
```js
// Žinomas Renogy PRO baterijos BLE modulio ypatumas (patvirtinta nepriklausomai kitoje
// platformoje: github.com/cyrils/renogy-bt issue #80, ta pati baterijų serija) — GATT
// rašymo patvirtinimas kartais "klysta", bet duomenys vis tiek sėkmingai ateina per
// notifikaciją. NE mūsų kodo ar Capacitor plugin'o klaida — nekenksminga, ignoruoti.
```
Priėmimo kriterijus: klaida nebematoma vartotojui kaip `'error'`, bet duomenys toliau
atsinaujina normaliai (patikrinti tuo pačiu testu kaip Žingsnis 2).

**3b (PAPILDOMA, NEBŪTINA dabar — bendra Android BLE gera praktika, atskira nuo šios
konkrečios klaidos).** `android/www/ble-plugin.js` (eilutės 186-227) turi VIENĄ GLOBALŲ
`this.queue` visiems BLE įrenginiams kartu, o ne atskirą kiekvienam — tai NEBEATRODO
tikėtina šios konkrečios klaidos priežastis (žr. aukščiau — priežastis nustatyta esanti
aparatūroje, ne eiliavime), bet vis tiek yra bendra Android BLE geros praktikos rekomendacija
([Making Android BLE work, part 3](https://medium.com/@martijn.van.welie/making-android-ble-work-part-3-117d3a8aee23)),
galinti pagerinti bendrą stabilumą (pvz. DCC50S ryšio laiko limito problemą). Daryti TIK jei
liks laiko po 3a ir pagrindinių žingsnių — NEBŪTINA šiam konkrečiam taisymui.

**DCC50S ryšio laiko limito problema** (6× `connect failed` ankstesniame teste) — pagal
[Renogy oficialų troubleshooting](https://www.renogy.com/troubleshooting/How-to-Solve-the-Connection-Issue-Between-BT-2-Bluetooth-Module-and-DC-DC-MPPT-Battery-Charger)
tai irgi žinomas BT-2/BT-1 modulio ypatumas — jei pasikartos STOVINT vietoje (ne važiuojant),
oficialus sprendimas: atjungti BT modulį nuo įkroviklio ~30s, prijungti akumuliatorių, tada
vėl prijungti BT modulį. Tai HARDWARE veiksmas, ne kodo pakeitimas.

---

## Žingsnis 4: Service 21 KWP duomenų dekoderis

**Failas:** `android/www/obd.js`. Pridėti dvi naujas funkcijas (pvz. šalia `isNoData()`):

```js
function decodeKwpField(type, a, b) {
    // Šaltinis: jazdw/vag-blocks (GPLv3) kwp2000.cpp decodeBlockData()
    switch (type) {
        case 0x01: return { v: a*b/5.0, u: 'rpm' };
        case 0x04: return { v: Math.abs(b-127)*0.01*a, u: (b>127?'° ATDC':'° BTDC') };
        case 0x07: return { v: 0.01*a*b, u: 'km/h' };
        case 0x08: case 0x10: case 0x25: return { v: (a<<8)|b, u: 'Binary' };
        case 0x11: return { v: String.fromCharCode(a,b), u: 'ASCII' };
        case 0x12: return { v: a*b/25.0, u: 'mbar' };
        case 0x14: return { v: a*b/128.0-1, u: '%' };
        case 0x15: return { v: a*b/1000.0, u: 'V' };
        case 0x16: return { v: 0.001*a*b, u: 'ms' };
        case 0x17: return { v: b*a/256.0, u: '%' };
        case 0x1A: return { v: b-a, u: '°C' };
        case 0x21: return { v: a===0 ? 100*b : 100*b/a, u: '%' };
        case 0x22: return { v: (b-128)*0.01*a, u: 'kW' };
        case 0x23: return { v: a*b/100.0, u: 'l/h' };
        case 0x27: return { v: a*b/256.0, u: 'mg/stk' };
        case 0x31: return { v: a*b/40.0, u: 'mg/stk' };
        case 0x33: return { v: ((b-128)/255.0)*a, u: 'mg/stk Δ' };
        case 0x36: return { v: a*256+b, u: 'Count' };
        case 0x37: return { v: a*b/200.0, u: 's' };
        case 0x51: return { v: (a*112000+b*436)/1000, u: '° CF' };
        case 0x5E: return { v: a*(b/50.0-1), u: 'Nm' };
        default:   return { v: (a<<8)|b, u: 'Raw (tipas 0x' + type.toString(16) + ')' };
    }
}

function decodeKwpBlockResponse(hexResp) {
    const bytes = parseHexBytes(hexResp);
    if (bytes.length < 14 || bytes[0] !== 0x61) return null;
    const block = bytes[1];
    const fields = [];
    for (let i = 0; i < 4; i++) {
        const off = 2 + i*3;
        if (off+2 >= bytes.length) break;
        fields.push(decodeKwpField(bytes[off], bytes[off+1], bytes[off+2]));
    }
    return { block, fields };
}
```

Panaudoti `decodeKwpBlockResponse()` `runBlockSweepInternal()` viduje
([obd.js:517-526](../android/www/obd.js#L517)) — kiekvienam `21 XX` atsakymui papildomai
žurnale rodyti dekoduotą (tipas+reikšmė+vienetai) formą greta žalio hex. Papildomai — kaupti
rezultatus į `STATE.blockResults` (Map arba objektas: `blockNum -> {timestamp, fields}`),
kad juos galėtų nuskaityti UI (žr. Žingsnis 5).

**Patikrinimo pastaba (jau atlikta, nebekartoti):** dekoderis buvo paleistas prieš esamus
`docs/logs/obd_log_2026-07-06T16-*.txt` duomenis (offline, Node/Python skriptu) —
**95 iš 100 pagautų grupių turėjo bent vieną atpažįstamą tipo baitą**, ir rezultatai atrodo
prasmingi (pvz. `0x01` RPM laukai konsekvenčiai rodė 0 — tikėtina, nes tuo metu variklis buvo
išjungtas; `0x12` slėgio laukas rodė ~989 mbar — atitinka atmosferos slėgį). **BET:** kadangi
variklis buvo išjungtas visą užfiksuotą laiką, šis patikrinimas patvirtina TIK, kad dekoderis
NESUGENDA ir duoda tikėtinus rezultatus statinei/nulinei būsenai — **jis NEPATVIRTINA
konkrečios grupė→parametras lentelės teisingumo dinaminėms reikšmėms** (RPM>0, kintantis
slėgis ir t.t.). Tam reikalingas Žingsnio 1 realaus OBD testo pakartojimas SU veikiančiu
varikliu ir `tagState()` mygtukais (jau yra UI, [index.html:614-621](../android/www/index.html#L614)).

**Orientacinė grupių lentelė** — TIK etikečių pasiūlymas, NE patvirtintas faktas (surinkta iš
3+ nepriklausomų VAG PD TDI bendruomenės šaltinių skirtingiems, bet giminingiems varikliams —
mūsų tikslus EDC16CP34/Crafter derinys atskirai netikrintas):

| Grupė | Galimas turinys |
|---|---|
| 0 / 1 | RPM + įpurškimo kiekis + trukmė/kampas + aušinimo skysčio temp. |
| 2 | Tuščios eigos reguliavimas |
| 3 | EGR |
| 4 | Purkštuko laikas |
| 10 / 11 | Turbo slėgis |
| 12 | Kaitinimo žvakės |
| 13 / 14 | Cilindrų balansas (įpurškimo korekcija/cilindrui) |

**NENAUDOTI šios lentelės UI etiketėms be patvirtinimo** — žr. Žingsnis 5, kur UI rodo grupės
NUMERĮ, ne spėjamą pavadinimą, kol nepatvirtinta realiais duomenimis.

**Priėmimo kriterijus:** `node --check android/www/obd.js` praeina.

**Kontekstas (nekeičia užduoties, tik paaiškina apimtį):** 2006-2016 VW Crafter buvo statomas
kartu su Mercedes Sprinter (ta pati platforma/gamykla). Automobilis NETURI VAG Gateway
modulio; tik variklis, imobilaizeris ir centrinis užraktas yra VW/VCDS-native, likę moduliai
(ABS, oro pagalvės ir t.t.) — Mercedes aparatūra, VAG metodais nepasiekiama. Todėl šis
dokumentas apsiriboja TIK variklio Service 21 duomenimis — tai vienintelis realiai pasiekiamas
šaltinis papildomai telemetrijai.

---

## Žingsnis 5: Service 21 duomenų atvaizdavimas programėlėje

**Failas:** `android/www/index.html`, OBD kortelė ([index.html:549-628](../android/www/index.html#L549)).

**SVARBU — apsauga nuo pažeidimų:** ši dalis PRIDEDA naują sekciją, **NELIEČIA** esamų
elementų. Draudžiama:
- Trinti, pervadinti ar keisti esamas `id="obd-rpm"`, `obd-speed`, `obd-coolant`, `obd-load`,
  `obd-map`, `obd-maf`, `obd-rail-p`, `obd-oil-t`, `obd-egr-cmd`, `obd-egr-err`, `obd-dpf-p`,
  `obd-dpf-t` korteles ([index.html:563-572](../android/www/index.html#L563)) — jos liks
  rodyti `0`/`--` po Žingsnio 1 pakeitimų (Mode 01 polling išjungtas), TAI YRA NUMATYTA, ne klaida.
- Liesti bet kurį kitą tab'ą (Renogy, GPS, Energija ir t.t.).
- Keisti esamus mygtukus/funkcijas — tik pridėti naujų.

**Ką pridėti** — nauja sekcija PO „🔬 Gilus skenavimas / testavimas" (po
[index.html:621](../android/www/index.html#L621)), PRIEŠ „📋 OBD diagnostikos žurnalas":

```html
<div class="section-title" style="margin-top:24px;">📊 Service 21 dekoduoti duomenys</div>
<div class="card" style="grid-column: 1 / -1;">
    <div class="card-subtext">Kiekviena grupė = neverifikuota reikšmė. Grupių numeriai
        rodomi tokie, kokie yra — NEBANDYTA spėti pavadinimų be patvirtinimo.</div>
    <div id="obd-block-results" style="margin-top:10px; font-family:monospace; font-size:12px;"></div>
</div>
```

`obd.js` viduje pridėti funkciją, kuri perpiešia `#obd-block-results` iš `STATE.blockResults`
(atnaujinama po kiekvieno `runBlockSweepInternal()` gauto atsakymo — kviesti tą pačią
`window.updateUI()` konvenciją, kurią jau naudoja kiti moduliai, arba atskirą
`renderBlockResults()`, iškviečiamą iš `runBlockSweepInternal()` po kiekvieno dekodavimo):

```js
function renderBlockResults() {
    const el = document.getElementById('obd-block-results');
    if (!el || !STATE.blockResults) return;
    const rows = Array.from(STATE.blockResults.entries()).sort((a,b) => a[0]-b[0]);
    el.innerHTML = rows.map(([block, data]) => {
        const fieldsStr = data.fields.map(f =>
            `${f.v !== null ? (typeof f.v === 'number' ? f.v.toFixed(2) : f.v) : '?'} ${f.u}`
        ).join(' | ');
        return `<div>Grupė ${block}: ${fieldsStr}</div>`;
    }).join('');
}
```

**Priėmimo kriterijus:** po `runBlockSweep()` paleidimo, nauja sekcija realiu laiku parodo
dekoduotas grupes; VISOS esamos kortelės/mygtukai/kitos kortelės nepakitę (patikrinti diff'u
prieš/po — turi keistis TIK pridėti nauji blokai, joks esamas HTML/JS neturi būti ištrintas
ar pervadintas).

---

## Žingsnis 6: Papildomi PATVIRTINTI duomenys — baterijos įtampa, VIN/ID, klaidų kodai

**Kontekstas (paskutinė paieška, 3 komercinės programėlės — ScanMaster, OBD Auto Doctor,
Car Scanner ELM OBD2 — sutampa):**

### 6a. Baterijos/borto įtampa — `ATRV`

`ATRV` yra ELM327 AT komanda, kuri matuoja įtampą PAČIO adapterio aparatūra (OBD-II 16
kontaktas) — **NEPRIKLAUSO nuo automobilio protokolo**, veikia visada, net be sėkmingo
ryšio su ECU. Patvirtinta 3 nepriklausomose programėlėse.

**Failas:** `android/www/obd.js`. Pridėti periodinį (pvz. kas 10s, panašiai kaip Renogy
`pollCycle`) `sendCmd('ATRV', 1000)` iškvietimą, atsakymą (formatas paprastai `"12.6V"`)
išsaugoti į `window.sensorCache['obd_battery_v']` arba `STATE.batteryVoltage`.

### 6b. Klaidų kodai — KWP Service `0x18` (`readDiagnosticTroubleCodesByStatus`)

**NETESTUOTA mūsų automobiliui, bet konkretus komandos formatas rastas realiame atvirame
kode** ([baconwaifu/PyVCDS](https://github.com/baconwaifu/PyVCDS/blob/master/vw.py),
`readDTC()` funkcija): komanda `18 02 FF 00` (servisas 0x18, statuso maskas 0x02, grupė
0xFF00 = „visos grupės"). Atsakymo formatas: `byte[1]` = klaidų skaičius, tada poromis po
2 baitus nuo offset'o 2 — kiekviena pora yra vienas klaidos kodas.

**Rizikos įvertinimas:** Mode 01/09 (generic OBD/EOBD servisai) šiam ECU jau patvirtintai
NEVEIKIA — Service 0x18 yra KITOKS, KWP2000-specifinis servisas (ne generic OBD), tad TIKĖTINA,
kad veiks (nes veikia Service 21/22 KWP kanalas), bet **nėra garantijos**. **Testuoti PIRMĄ
KARTĄ TIK stovint**, po `1003` (extended session, jau naudojamas pattern'as kituose
skenavimuose).

**Failas:** `android/www/obd.js`. Pridėti funkciją:
```js
async function readDtcCodes() {
    await sendCmd('1003', 1000); // extended session, jau naudojamas pattern'as
    const resp = await sendCmd('1802FF00', 2000);
    // TODO: patikrinti realų atsakymo formatą prieš rašant pilną parserį —
    // jei NO DATA/klaida, žurnale aiškiai parašyti "DTC servisas neatsakė", NE tylėti.
    return resp;
}
```
Kviesti per naują mygtuką OBD kortelėje (NE automatiškai — klaidų kodų skaitymas turi būti
sąmoningas vartotojo veiksmas, panašiai kaip esami skenavimo mygtukai).

### 6c. UI — konsoliduotas PATVIRTINTŲ duomenų rodinys + debug log su jungikliu

**Failas:** `android/www/index.html`, ta pati OBD kortelė. PRIDĖTI (NE keisti esamų
elementų — ta pati apsauga kaip Žingsnyje 5) naują sekciją PRIEŠ Žingsnio 5 „Service 21
dekoduoti duomenys" bloką:

```html
<div class="section-title" style="margin-top:24px;">✅ Patvirtinti duomenys</div>
<div class="grid">
    <div class="card"><h3>Baterijos įtampa</h3><div class="card-value"><span id="obd-battery-v">--</span><span class="card-unit">V</span></div></div>
    <div class="card"><h3>VIN</h3><div class="card-value" style="font-size:14px;"><span id="obd-vin">--</span></div></div>
    <div class="card"><h3>SW-ID</h3><div class="card-value" style="font-size:12px;"><span id="obd-sw-id">--</span></div></div>
    <div class="card" style="grid-column: 1 / -1;">
        <h3>Klaidų kodai (DTC)</h3>
        <button class="btn btn-primary obd-scan-btn" onclick="ObdBLE.readDtcCodes()">🔍 Tikrinti klaidų kodus</button>
        <div id="obd-dtc-list" style="margin-top:10px; font-family:monospace; font-size:12px;">Nepatikrinta.</div>
    </div>
</div>
```

**Debug žurnalo jungiklis** — prie jau esamos „📋 OBD diagnostikos žurnalas" antraštės
([index.html:627](../android/www/index.html#L627)) pridėti jungiklį (tokį patį `<label
class="switch">` elementą, kokį jau naudoja `chk_obd`/`chk_obd_safe_mode`), kuris
rodo/slepia žurnalo `<div>` per CSS `display:none`, saugo būseną `localStorage` (panašiai
kaip `renogy_enabled`/`obd_safe_mode`), numatyta reikšmė — **rodyti** (kad naujo diegimo
metu iškart matytųsi, kas vyksta; vartotojas patys išjungs, kai įsitikins, kad viskas veikia
be klaidų).

```js
// obd.js — pridėti prie kitų toggle funkcijų
function toggleDebugLog(visible) {
    localStorage.setItem('obd_debug_log_visible', visible);
    const el = document.getElementById('obd-log-output'); // arba tikras esamo žurnalo id
    if (el) el.closest('.card').style.display = visible ? '' : 'none';
}
```
(Patikrinti tikslų esamo žurnalo konteinerio `id`/struktūrą prieš rašant — naudoti TĄ PATĮ,
kurį jau naudoja `obdLog()` [obd.js:75-89](../android/www/obd.js#L75), NEKURTI antro.)

**Priėmimo kriterijus:** po pilno testo su nauja APK — baterijos įtampa atsinaujina periodiškai
be klaidų; VIN/SW-ID rodomi (jau žinome, kad šie duomenys pasiekiami); paspaudus „Tikrinti
klaidų kodus" — arba parodomi realūs kodai, arba aiškus „neatsakė" pranešimas (NE tyla/
klaida konsolėje); debug žurnalas gali būti paslėptas/rodomas jungikliu, numatytai — matomas.

---

## Bendros taisyklės

- Po KIEKVIENO kodo pakeitimo: `node --check android/www/obd.js` ir
  `node --check android/www/renogy.js`.
- **NEDARYTI** `git commit`/`push` be aiškaus vartotojo prašymo šioje sesijoje.
- **NEDARYTI** APK build'o be aiškaus vartotojo prašymo, IŠSKYRUS Žingsnį 0, kuris yra
  būtina sąlyga likusiems žingsniams — jei neaišku, ar leidimas duotas, KLAUSTI vartotojo
  prieš vykdant Žingsnį 0.
- Versijos numeris (`versionCode`/`CURRENT_VERSION`/`version.json`) turi būti VISADA
  identiškas visose trijose vietose.
- Kiekvieną žingsnį (0-6) baigti PRIEŠ pradedant kitą. Nedaryti prielaidų apie tai, ar
  ankstesnis žingsnis pavyko, be realaus žurnalo įrodymo.
- **Prieš baigiant bet kurį žingsnį, kuris keičia `obd.js`/`renogy.js`/`index.html`:**
  palyginti `git diff` su ankstesne versija ir įsitikinti, kad NIEKAS jau veikiantis
  (Renogy, GPS, kiti tab'ai, esamos OBD kortelės) NEBUVO ištrinta, pervadinta ar pakeista —
  leidžiami TIK PRIDĖJIMAI, nebent konkretus žingsnis aiškiai nurodo ką pakeisti (pvz.
  Žingsnis 3 pakeičia `queryDevice()` viduje esamą kodą, tai leidžiama, nes taip nurodyta).
