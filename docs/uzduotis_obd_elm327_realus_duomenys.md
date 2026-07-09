# Užduotis: OBD — rodyti tik tai, ką realiai gali ELM327 (generic Mode 01/03/09)

**Kam:** Android asistentui (faktinis kodo taisymas). Cowork Claude = tik auditas, kodo nekeičia.
**Data:** 2026-07-09. **Failai:** `android/android/app/src/main/assets/public/obd.js` (+ `index.html` UI).
**Kontekstas:** `docs/audits/auditas_2026-07-09_obd_ir_gyvo_testo_logai.md` (regresija O6),
`[[kemperis-obd-regresija-v51]]`.

---

## 0. Esmė (kodėl ši užduotis)

Vartotojas klausė: „paimkime duomenis kaip OBD Car Scanner ir parodykime". Atsakymas:
- **Kopijuoti uždarų programėlių (OBD Car Scanner) kodo negalima** (svetima IP). **Ir nereikia.**
- Tos programėlės rodo **tik generic SAE J1979 duomenis** (Mode 01 PID + Mode 09 VIN + Mode 03 DTC)
  per standartinį ISO 15765-4 CAN. **Tavo `obd.js` tą kodą JAU turi** (29–33 eil. PID lentelė,
  `runIdentityInternal` VIN) — v51 jį tik užkomentavo.
- Gilūs VAG duomenys (DPF suodžiai, purkštukų korekcijos) reikalauja **TP2.0** protokolo, kurio
  paprastas ELM327 v1.5 **negali**. Tų programėlės Crafteriui irgi NErodo. → **Šioje užduotyje
  jų NEDAROM.**

**Tikslas:** grąžinti ir stabilizuoti tik tą OBD dalį, kuri realiai veikia su ELM327, kad app
vėl rodytų apsukas, greitį, temperatūras, VIN ir klaidų kodus.

Atviro kodo etalonai (legalu studijuoti, GPL): **AndrOBD** (`fr3ts0n/AndrOBD`), **python-OBD**.
Jų init/PID logika sutampa su žemiau aprašyta.

---

## 1. ✅ KĄ DARYTI — realu su ELM327 (ISO 15765-4 CAN)

### 1.1 Init seka (grąžinti `ATSP0` auto vietoje `ATSP6`)

| Komanda | Paskirtis | Pastaba |
|---|---|---|
| `ATZ` | reset | laukti ≥1 s / iki `>` |
| `ATE0` | echo off | |
| `ATL0` | linefeed off | |
| `ATS0` | tarpai off | greitesnis parse (nebūtina) |
| `ATSP0` | **auto protokolas** | 2008 Crafter = CAN; `ATSP0` pats suras 11/29-bit. **Nebefiksuoti `ATSP6`** — jei ECU 29-bit ar kitokia sparta, hardcode nutraukia ryšį |
| `ATAT1` | adaptyvus laikas | palikti |
| (nebūtina) `ATH1` | rodyti antraštes | pravartu multi-ECU atskyrimui; app parse turi tai palaikyti |

Po init paleisti `ATDP` — užlogint, kokį protokolą ELM327 realiai sudarė (diagnostikai).

### 1.2 Ryšio patikra — PID bitmap (BŪTINA pirmiausia)

Prieš bet ką siųsti **`0100`**. Atsakymas `41 00 xx xx xx xx` = ryšys su varikliniu ECU (7E8) YRA.
Toliau `0120`, `0140`, `0160` — surinkti pilną palaikomų PID bitmap'ą. **Poll'inti TIK tuos PID,
kurie bitmap'e pažymėti kaip palaikomi** (dyzelis palaiko ne visus — negaišti laiko su
nepalaikomais).

Jei `0100` grąžina `NO DATA` su **veikiančiu varikliu** — problema fizinė (uždegimas/gateway/
laidai/protokolas), NE PID žemėlapyje; tik tada gilintis toliau.

### 1.3 Mode 01 — gyvi duomenys (formulės patikrintos su SAE J1979 / Wikipedia)

> `A`, `B` = pirmi du duomenų baitai po `41 <PID>`. App JAU turi 0C/0D/05/04/0B (29–33 eil.);
> jų formulės teisingos. Pridėti likusius, jei yra bitmap'e.

| PID | Rodmuo | Formulė | Vnt. | Ar app turi |
|---|---|---|---|---|
| `0C` | Apsukos (RPM) | (256·A+B)/4 | rpm | ✅ turi |
| `0D` | Greitis | A | km/h | ✅ turi |
| `05` | Aušinimo temp | A−40 | °C | ✅ turi |
| `04` | Variklio apkrova | 100/255·A | % | ✅ turi |
| `0B` | Įsiurbimo slėgis (MAP) | A | kPa | ✅ turi |
| `0F` | Įsiurbimo oro temp | A−40 | °C | pridėti |
| `10` | MAF (oro srautas) | (256·A+B)/100 | g/s | pridėti |
| `1F` | Variklio veikimo laikas | 256·A+B | s | pridėti |
| `33` | Barometrinis slėgis | A | kPa | pridėti |
| `42` | ECU modulio įtampa | (256·A+B)/1000 | V | pridėti |
| `46` | Aplinkos oro temp | A−40 | °C | pridėti (jei palaiko) |
| `5C` | Alyvos temp | A−40 | °C | pridėti (dažnas dyzelyje) |
| `5E` | Kuro sąnaudos | (256·A+B)/20 | L/h | pridėti (jei palaiko) |
| `62` | Faktinis variklio momentas | A−125 | % | pridėti (jei palaiko) |
| `63` | Nominalus momentas | 256·A+B | N·m | pridėti (jei palaiko) |

> ⚠️ Turbo slėgis, DPF suodžiai, purkštukų korekcijos generic Mode 01 **nėra** — žr. §2.

### 1.4 Mode 09 — VIN ir ECU tapatybė (grąžinti `runIdentityInternal`)

- `0902` → VIN (multi-frame ISO-TP; ELM327 pats surenka; atsakymas `49 02 …` → ASCII).
- `0904` → Calibration ID. `090A` → ECU pavadinimas.
- **Grąžinti `runIdentityInternal()` iškvietimą** „Surinkti VISKĄ" flow'e (dabar užkomentuota
  `obd.js` ~751 eil.).

### 1.5 Mode 03 / 07 — klaidų kodai (DTC)

- `03` = išsaugotos DTC, `07` = laukiančios, `0A` = permanentinės. Formatas: 2 baitai/kodą →
  raidė (P/C/B/U) + 4 skaitmenys. (`04` = išvalyti — daryti tik su patvirtinimu.)
- Dabartinis DTC kelias naudoja KWP `18 02 FF 00` — **pakeisti į generic `03`** (veikia per CAN;
  `18` yra senas KWP servisas, EDC16-CAN dažnai neatsako).

### 1.6 Pacing / stabilumas (klono adapteris)

- Native tiltas (`MainActivity.java` ~1055) JAU dalija atsakymus per `>` promptą — kiekvienas
  `onObdData` = pilnas atsakymas. **Tai gerai, palikti.**
- „STOPPED" lavina normaliame režime = **pigus ELM327 v1.5 klonas nespėja** po prompto
  (0 ms pauzė). Logai įrodo: „saugus režimas" (300 ms) → švarūs atsakymai be „STOPPED".
- **Padaryti `safeMode` numatytą ĮJUNGTĄ** (arba priverstinį per skenavimą), nepriklausomai nuo
  `localStorage`. `obd.js` 23 eil. (`safeMode` init) ir 183 eil. (`delay`).

---

## 2. ⛔ KO NEDARYTI — reikia kito adapterio (ne ELM327)

| Funkcija | Kodėl neveikia su ELM327 | Ką reiktų |
|---|---|---|
| VAG measuring blocks — **Service 21** (`21 01…21 FF`) | Crafter EDC16CP34 juos atiduoda per **KWP2000-over-TP2.0**; ELM327 TP2.0 neatidaro | VCDS/Ross-Tech HEX-V2, OBDeleven, arba TP2.0-gebantis adapteris |
| UDS DID `22 xxxx` (DPF suodžiai, purkštukų korekcijos, rail slėgis) | Patvirtinta `NO DATA` visuose testuose; reikia gamintojo UDS sesijos, kurios šis ECU per generic ELM327 neduoda | tas pats — pro įrankis |
| Modulių adresų sweep `0x700–0x7FF` (`ATSH` + `10 01/03`) | Triukšmas, jokio rezultato; ne generic | — |

**Veiksmas kode:** `runProtocolProbeInternal` (Service 21 zondas), `runDidScanInternal`,
`runBlockSweepInternal`, `runModuleScanInternal` — **nebekviesti** iš „Surinkti VISKĄ". Palikti
kodą (jei kada bus TP2.0 adapteris), bet paslėpti/atjungti iš pagrindinio srauto, kad
nekurtų 3–4 min „NO DATA/STOPPED" triukšmo ir neklaidintų „nieko neveikia".

---

## 3. Konkretūs `obd.js` pakeitimai (santrauka)

1. `initSequence`: `ATSP6` → `ATSP0`; po init pridėti `ATDP` log.
2. `runAll` (~750 eil.): **įjungti** `startPolling()` (arba vienkartinį Mode 01 nuskaitymą) ir
   `runIdentityInternal()` (VIN); **išjungti** `runProtocolProbeInternal` / `runDidScanInternal` /
   block/module sweep iš pagrindinio srauto.
3. Prieš poll'inimą — `0100/0120/0140` bitmap; poll'inti tik palaikomus PID.
4. Praplėsti `PID0/PID_A` lentelę §1.3 PID'ais (0F,10,1F,33,42,46,5C,5E,62,63) su `key`+`fmt`.
5. DTC: `18 02 FF 00` → `03` (+`07`).
6. `safeMode` default = `true`.
7. UI (`index.html`): naujiems PID pridėti korteles (arba rodyti tik gautus); VIN kortelė jau yra.

---

## 4. Testo protokolas (privaloma — su VEIKIANČIU varikliu)

1. Užvesti variklį (RPM > 0). *(Ankstesni testai vyko su išjungtu varikliu — todėl `NO DATA`.)*
2. Prijungti ELM327, palaukti init + `ATDP` (užfiksuoti protokolą).
3. `0100` → turi grįžti `41 00 …`. Jei ne — stop, tikrinti fizinį ryšį/uždegimą.
4. Paspausti „Surinkti VISKĄ" / poll — patikrinti, kad rodo RPM (kinta su gazu), greitį,
   aušinimo/alyvos temp, VIN.
5. Užlogint sesiją; patvirtinti, kad **nėra** „STOPPED" lavinos (safeMode ON).

**Sėkmės kriterijus:** RPM realiu laiku + VIN + bent aušinimo temp rodomi app'e, be „STOPPED".

---

## 5. Verifikacija (online-patikrinta rašant užduotį)

- **ELM327 init (`ATZ/ATE0/ATL0/ATS0/ATSP0`, laukti `>`):** SparkFun ELM327 AT Commands;
  ScanDoc ELM327 guide; AndrOBD praktika.
- **2008 Crafter = CAN (ISO 15765-4), ne K-line:** OBDPORT / CSS Electronics protokolų aprašai.
  Todėl `ATSP0` (auto) saugu; `05/06` servisai (K-line specifiniai) nenaudojami.
- **PID formulės (0C,0D,05,04,0B,0F,10,1F,33,42,46,5C,5E,62,63):** patikrinta su
  Wikipedia „OBD-II PIDs" (SAE J1979) lentele — sutampa su app esamomis formulėmis.
- **Bitmap `0100/0120/0140`:** Wikipedia OBD-II PIDs (supported-PID bitmask).
- **VIN `0902` multi-frame per ELM327:** Wikipedia Mode 09 (`49 02`, ISO-TP reassembly).
- **Service 21 / VAG blokams reikia TP2.0:** Ross-Tech Wiki „VW Crafter (2E)" (KWP2000-over-TP2.0
  / UDS-over-CAN); patvirtina, kad generic ELM327 jų neduoda.
- **`>` prompt gating jau yra tilte:** `MainActivity.java` ~1055 (`buf.indexOf(">")`).

---

## 6. Papildomos UI užduotys (ne OBD — Renogy / Junctek / SSE)

> Iš `docs/audits/auditas_2026-07-09_obd_ir_gyvo_testo_logai.md` (U1–U3). Tik `index.html`
> (ir kur nurodyta `renogy.js`) — **firmware NELIESTI**.

### 6.1 (U1) Baterijos temp jutikliai 3 ir 4 — visada tušti → šalinti

**Faktas:** `index.html` (494–502 eil.) turi **4 fiksuotas** temp korteles:
`ren_bat_temp`(1), `ren_bat_temp_1`(2), `ren_bat_temp_2`(3), `ren_bat_temp_3`(4). Bet `renogy.js`
(319–321 eil.) **visada ištrina** `ren_temp_2` ir `ren_temp_3` („Reg 5020/5021 dažniausiai 0"),
o `ren_temp_1` — tik jei jutiklių ≥2. RBT12100LFP-BT realiai grąžina **2 jutiklius** → 3 ir 4
kortelės niekada neužsipildo (rodo `0`).

**Veiksmas:**
- Pašalinti korteles 3 ir 4 (`index.html` 500–501 eil., `id="ren_bat_temp_2"` ir
  `id="ren_bat_temp_3"`) ir jų bindus (`setSensorValue('ren_temp_2'…)`,
  `setSensorValue('ren_temp_3'…)` — 2216–2217 eil.).
- 2-ą kortelę (`ren_bat_temp_1`) padaryti **sąlyginę**: rodyti tik kai `ren_temp_1` yra cache'e
  (t. y. `tempCount ≥ 2`); kitaip slėpti. Gridą sumažinti iki 1–2 stulpelių pagal turimų kiekį.

### 6.2 (U2) „Likęs veikimas 0 min" kraunant → rodyti „—" / „kraunasi"

**Faktas:** `junc_time_remaining` (Junctek D6) kraunant = 0 (rodo tik iškrovimo laiką). App tuo
pat metu žino būseną: `bat-status` = „⚡ KRAUNASI" kai `junc_a > 0.1` (2083 eil.). Bet „Likęs
veikimas" kortelė (**1960 eil.**) rodo žalią `0 min` — klaidina.

**Veiksmas:** `junc_time_val` formatteryje patikrinti srovę ir kraunant nerodyti „0 min":

```js
setSensorValue('junc_time_remaining', 'junc_time_val', v => {
    const a = parseFloat(window.sensorCache['junc_a']);
    if (!isNaN(a) && a > 0.1) return '—';   // kraunasi → likusio veikimo laiko nėra prasmės
    const m = Math.round(parseFloat(v));
    return m >= 60 ? Math.floor(m/60)+'h '+(m%60)+'min' : m+' min';
});
```

(Kosmetika; galima vietoj „—" rodyti „kraunasi".)

### 6.3 (U3) ESP temp yra sraute, bet apatinėje juostoje nerodo → slug mapping

**Faktas:** SSE srautas turi `sensor_sistema_esp_temperat_ra = 43` — **duomuo YRA**, firmware
jutiklis (`sys_esp_temp`, `kempervanas.yaml` 1101 eil.) veikia. Map lentelė (`index.html`
**924 eil.**) turi 3 **trigubo pabraukimo** variantus (`sistema___esp_temperat_ra`,
`…temperatura`, `…temperatra`) → `sys_esp_temp`, o `diag-esp-temp` juosta atnaujinama 1965 eil.
Vis tiek nerodo → **įtartinas slug'o forma** (žr. `[[kemperis-sse-objectid-faktas]]`).

**Įtarimas:** SSE debug sraute raktas matomas su **vienguba** pabraukimo forma
(`sistema_esp_temperat_ra`), o map turi tik **trigubą** — jei SSE handleris ieško vienguboje
formoje, nei vienas variantas nesutampa → nerodo.

**Veiksmas (2 min patikra + 1 eilutės pataisa):**
1. SSE handleryje, **prieš** map paiešką, `console.log` išvesti **tikslų** raktą, kurį handleris
   naudoja lookup'ui ESP temperatūrai (ne app „gražintą" debug variantą).
2. Palyginti su 924 eil. variantais. Jei skiriasi (labiausiai tikėtina — vienguba forma
   `sistema_esp_temperat_ra` / `sistema_esp_temperatura`), **pridėti trūkstamą variantą** į map.
3. **Firmware pavadinimo NEKEISTI** („Sistema | ESP temperatūra") — kitaip pasikeis slug'as ir
   nukris kiti mapping'ai (žr. SSE object_id faktą).

---

### Šaltiniai
- ELM327 AT komandos: https://cdn.sparkfun.com/assets/4/e/5/0/2/ELM327_AT_Commands.pdf
- ELM327 v2.3 guide: https://scandoc.org/en/develop/elm327.html
- OBD-II PIDs (formulės, bitmap, Mode 09): https://en.wikipedia.org/wiki/OBD-II_PIDs
- OBD-II modes & protokolai (CAN 2008+): https://obdport.com/knowledge/protocols/obd-ii-modes-and-pids
- OBD2 PID lentelė / J1979: https://www.csselectronics.com/pages/obd2-pid-table-on-board-diagnostics-j1979
- AndrOBD (atviras kodas, etalonas): https://github.com/fr3ts0n/AndrOBD
- VW Crafter (2E) protokolai: https://wiki.ross-tech.com/wiki/index.php/VW_Crafter_(2E)
