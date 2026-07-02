# Užduotis v33.3 (v39): Renogy parserio pataisos (R1, R2, R3, R6 + smulkmenos)

**Data:** 2026-07-02 · **Bazė:** v38 / commit e59d86f
**Kontekstas:** `docs/audits/auditas_2026-07-02_vakaras_v38.md` — visi taisymai žemiau
**validuoti prieš renogy-bt šaltinio kodą** (BatteryClient.py, DCChargerClient.py).
Keičiamas praktiškai tik `android/www/renogy.js` + versijos. Firmware, SSE, Sheets — NELIESTI.

## ⚠️ PRIEŠ PRADEDANT (privaloma, dėl vakar rasto worktree sugadinimo)

1. Įsitikinti, kad `.git/index.lock` pašalintas (vartotojas trina ranka) ir `git status`
   švarus (be netikėtų `D`/`M`).
2. Patikrinti, kad faili NENUKIRSTI: `MainActivity.java` baigiasi `WifiBridge` klase ir `}`,
   `index.html` — `</html>`, `renogy.js` — `RenogyBLE.init()` bloku. Jei nukirsta —
   `git checkout -- <failas>` ir tik tada dirbti.
3. Po darbų, PRIEŠ commit: `git diff --stat` — jei kurio failo diff'as vien ištrynimai
   gale, failas vėl nukirstas, necommitinti.

## Pataisos (renogy.js)

### R6 — celių įtampų mastelis (4 eilutės)
renogy-bt `parse_cell_volt_info`: scale **0.1** (raw 36 = 3.6 V). Dabar dalinama iš 1000.
```js
// buvo: u16(data[2], data[3]) / 1000.0;  → turi būti:
window.sensorCache['ren_cell_0'] = u16(data[2], data[3]) / 10.0;
window.sensorCache['ren_cell_1'] = u16(data[4], data[5]) / 10.0;
window.sensorCache['ren_cell_2'] = u16(data[6], data[7]) / 10.0;
window.sensorCache['ren_cell_3'] = u16(data[8], data[9]) / 10.0;
```

### R2 — baterijos temperatūra (1 eilutė)
Dabar skaitoma `data[24]` = reg 5012 (celės #12 įtampa — 4 celių baterijoje 0).
renogy-bt: sensor_count reg 5017 (`data[34..35]`), temperatūros nuo reg **5018**,
signed, scale 0.1. Užklausos keisti nereikia (0x22 žodžių blokas 5000–5033 jau dengia).
```js
window.sensorCache['ren_temp'] = s16(data[36], data[37]) / 10.0;
```

### R3 — DCC krovimo stadijų žemėlapis (1 masyvas)
Tikrasis renogy-bt `CHARGING_STATE` (su kodu 8!):
```js
const stages = { 0:"Neaktyvus", 1:"Aktyvuotas", 2:"MPPT", 3:"Islyginimas",
                 4:"Boost", 5:"Palaikymas", 6:"Sroves ribojimas", 8:"Tiesiai is generatoriaus" };
window.sensorCache['dcc_stage'] = stages[stageCode] || ("Kodas " + stageCode);
```
(Masyvą keisti į objektą — kodas 8 su masyvu paliktų skylę. Vertimus galima keisti,
svarbu KODŲ atitikimas.)

### R1 — atsakymo bloko identifikacija pagal turinį, ne būseną
Problema: `parseFrame` bloką renkasi pagal `queryIndex`, apverstą RAŠYMO metu — pametus
vieną atsakymą viskas pasislenka (celės parsinamos kaip V/A, tyliai).
Taisymas: identifikuoti pagal `frame[2]` (byte_count) — reikšmės unikalios:

| frame[2] | Blokas |
|---|---|
| `0x44` (68) | Baterija 0x1388: celės + temp |
| `0x0C` (12) | Baterija 0x13B2: A/V/Ah/talpa |
| `0x42` (66) | DCC 0x0100 |

```js
function parseFrame(type, frame) {
    const data = frame.slice(3, -2);
    const len = frame[2];
    if (type === 'battery' && len === 0x44) { /* celės + temp (R6+R2 offsetai) */ }
    else if (type === 'battery' && len === 0x0C) { /* A/V/Ah — esamas kodas, jis teisingas */ }
    else if (type === 'dcc' && len === 0x42) { /* esamas DCC kodas — jis teisingas */ }
    else { debug(`Nezinomas blokas: type=${type} len=${len}`, 'warn'); }
}
```
`queryIndex` palikti tik užklausų kaitaliojimui (rašymo pusėje) — parse'ui jo nebenaudoti.
Pašalinti dead code: `startReg` eilutę (~254).

### R4 (smulkios, jei telpa)
- `startScan`: kintamojo vardas `hasLocation` klaidina (tikrina BT, ne location) — pervadinti.
- Skenavimo sąraše Renogy įrenginius (`BT-TH`/`RNGRBP` prefiksai) rikiuoti viršun.

### R5 — lokalaus CSV senų eilučių suderinamumas (index.html)
`downloadLocalCSV`: prieš `join`, senas eilutes (mažiau kablelių nei header'yje) papildyti
tuščiais laukais, kad stulpeliai sutaptų:
```js
const nCols = (localLogHeader.match(/,/g)||[]).length;
const rows = localLogRows.map(r => { let m = nCols - (r.match(/,/g)||[]).length;
    return m > 0 ? r + ','.repeat(m) : r; });
const csvContent = localLogHeader + "\n" + rows.join("\n");
```

## NEDARYTI
- Nekeisti DCC laukų offsetų/mastelių ir baterijos 0x13B2 parse — jie **patikrinti, teisingi**.
- Nekeisti užklausų (0x1388/0x22, 0x13B2/0x06, 0x0100/0x21) — teisingos.
- Jei gyvame teste `dcc_stage` nesutaps su Renogy app — TIK TADA pridėti atskirą užklausą
  `[0xFF,0x03,0x01,0x20,0x00,0x03]` (reg 0x120, 3 žodžiai; atsakymo len=0x06 — pridėti
  į identifikacijos lentelę). Be gyvo įrodymo nedaryti.

## Versijos ir build
`version.json "version": 39` == `versionCode 39` == APK `android/kemperis_v39.apk` ==
`apk_url`; `versionName "33.3"`; notes — trumpas changelog (celių masteliai, temp,
stadijos, parserio stabilumas). `npx cap sync android` nereikia (native nesikeičia),
užtenka www kopijos + Gradle build.

## Priėmimo kriterijai
1. Celės rodo ~3.3–3.6 V (ne 0.03 V); temp — realią (~15–30 °C), sutampa su Renogy app ±1°.
2. Krovimo stadija atitinka Renogy app (mppt kraunant nuo saulės; jei nesutampa — HEX logą
   išsaugoti ir fiksuoti audite, žr. NEDARYTI).
3. Dirbtinis testas R1: debug loge nėra „Nezinomas blokas" normalioje eigoje; išjungus/
   įjungus bateriją duomenys nesusimaišo (celės nerodo įtampos lauke ir atvirkščiai).
4. Senas CSV eksportas atsidaro su lygiais stulpeliais.
5. Versijų grandinė sinchronizuota; `git diff --stat` be nukirstų failų; commit + push.
6. Trumpas įrašas `docs/audits/` (kas pataisyta, gyvo testo rezultatai).
