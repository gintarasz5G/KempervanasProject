# Užduotis: OBD/ELM327 stabilumo taisymai po realaus testo su automobiliu

**Data:** 2026-07-06 · **Bazinė versija:** v47 (versionCode 47, versionName "35.0")
**Kontekstas:** OBD/ELM327 tab'as (`docs/uzduotis_obd2_elm327_tab.md`) buvo implementuotas ir
testuotas realiu automobiliu (VW Crafter 2008, 2.5 TDI). 4 realūs žurnalų failai (terminalo +
OBD) atskleidė **4 konkrečius, patvirtintus bug'us**. Ši užduotis juos taiso. **Neimplementuoti
jokių papildomų „gilesnių" funkcijų be aiškaus prašymo** — tik žemiau išvardinti taisymai.

---

## Radiniai (patvirtinti realiais žurnalais, ne spėjimai)

1. **ECU „užsirakina" po sesijos komandų.** Po Mode 22 DID skenavimo (`10 03`) ir modulių
   skenavimo (`10 01`/`10 03` į daugybę adresų), grįžus prie normalaus Mode 01 polling'o,
   **visi** atsakymai tampa `7F 01 22` (neigiamas UDS atsakymas, NRC 0x22
   „conditionsNotCorrect") ir tęsiasi **~1+ minutę be perstojo**. Įrodymas:
   `obd_log_2026-07-06T16-49-47.txt`, eilutės 578-800. Priežastis: niekur nesiunčiama `10 01`
   (grįžti į DEFAULT diagnostikos sesiją) prieš atnaujinant normalų polling'ą.
2. **Pasenusio atsakymo (stale response) klaidingas priskyrimas.** Modulių skenavime matomi
   neįmanomai greiti atsakymai (Δ8-17ms — fiziškai neįmanoma per Bluetooth+CAN) ir
   „OK"/„STOPPED" atsakymai vietoj tikrų duomenų į `1001`/`1003` komandas ~24 skirtinguose
   adresuose iš eilės (0x780-0x7FF diapazone) — tai statistiškai neįtikėtina tiek daug realių
   ECU modulių. Priežastis: kai `sendCmd()` timeout'as suveikia, `pumpQueue()` **iškart**
   siunčia kitą komandą, bet ELM327 vis tiek gali atsiųsti PAVĖLUOTĄ atsakymą SENAI komandai —
   jis klaidingai priskiriamas naujai laukiančiai komandai. Įrodymas:
   `obd_log_2026-07-06T16-48-41.txt` (ir dublikatas `16-49-47.txt`), pvz. eilutės su
   `RX: ? (Δ14ms)` iškart po `ATSH<addr>` komandos.
3. **Konkurentiškumas — kelios skenavimo funkcijos vienu metu.** Faile
   `obd_log_2026-07-06T16-55-47.txt` sekcijų antraštės (`=== DID SKENAVIMAS ===` ir pan.)
   atsiranda VIDURYJE jau vykstančio kito skenavimo (pvz. eilutė 435 „DID SKENAVIMAS" prasideda,
   kai jau nuo eilutės 5 siunčiamos DID kandidatų komandos). Jokios apsaugos nuo pakartotinio
   paleidimo (pvz. dvigubo mygtuko paspaudimo) nėra.
4. **SSE UNMATCHED sensoriai vis dar neveikia.** Ankstesnis fix'as (SENSOR_ID_MAP papildymas)
   pridėjo įrašus tik pagal `data.id`, bet realiame teste `text_sensor_gps_fix_statusas`,
   `sensor_gsm_signalas_dbm_`, `text_sensor_sistema_paskutine_klaida`,
   `text_sensor_sistema_modemo_busena` **vis dar** rodomi kaip `[SSE UNMATCHED]`. Priežastis:
   `data.id` reikšmė realybėje neatitinka to, kas buvo tikėtasi — reikia PAPILDOMAI pridėti
   atsarginius (fallback) įrašus pagal pavadinimo atpažinimą (`rawId.includes(...)`), kaip veikia
   VISI kiti sėkmingai atpažįstami sensoriai tame pačiame kode.

**⚠️ Sąmoningai ATMESTA (NEDARYTI, nepaisant kito šaltinio pasiūlymo):**
- „VW TP 2.0 handshake" į adresą `0x200` — **paneigta realiais duomenimis**: gavome tikrus
  Service 21 atsakymus iš `0x7E0` be jokio TP 2.0 setup. TP 2.0 ir ISO 15765 yra du atskiri,
  nepriklausomi transportai (patvirtinta šaltiniu jazdw.net/tp20) — vienas nėra kito
  prerekvizitas. Nepridėti jokio „Inicijuoti TP 2.0" mygtuko ar logikos.
- Automatinis Renogy BLE pristabdymas OBD skenavimo metu — neįrodyta šiais žurnalais (Renogy
  write klaidos matėsi dar prieš OBD tab'ą atsirandant), pridėtų sudėtingumo be patvirtintos
  naudos. Nedaryti.

---

## Taisymas 1: Sesijos grąžinimas po diagnostikos darbo

**Failas:** `android/www/obd.js`

Po **bet kurio** iš šių: `runDidScan()`, `runBlockSweep()`, `runModuleScan()` — prieš grąžinant
valdymą (ir prieš `startPolling()` iškvietimą `runFullCollection()` pabaigoje) **būtina** siųsti:
```
10 01
```
(grąžinti ECU į DEFAULT diagnostikos sesiją). Tai turi būti paskutinis siunčiamas dalykas
prieš bet kokį normalų Mode 01 polling'ą.

Konkrečiai:
- `runModuleScan()` jau siunčia `ATSH7E0` + `ATSP6` pabaigoje (eilutės apie
  „Modulių skenavimas baigtas") — **prieš** tai (ar iškart po `ATSH7E0`) papildomai
  pridėti `await sendCmd('1001', 1000);` kad grąžintų default sesiją.
- `runFullCollection()` pabaigoje (prieš `startPolling()`) — papildomai užtikrinti, kad
  paskutinis siųstas dalykas prieš grąžinant į polling'ą būtų `1001` (default session),
  net jei `runModuleScan()` jau tai padarė — saugus dubliavimas nekenkia.
- `runDidScan()` po savo `10 03` (extended session) darbo, jei toliau EINA prie
  `runModuleScan()` (kaip `runFullCollection()` sekoje) — nereikia atskirai taisyti čia,
  nes `runModuleScan()` pabaigoje bus grąžinama; bet jei `runDidScan()` kviečiamas
  ATSKIRAI (pvz. paspaudus tik „3️⃣ Skenuoti DID" mygtuką be viso `runFullCollection`),
  jis TAIP PAT turi pats grąžinti `10 01` savo pabaigoje (po galimo `runBlockSweep()`
  iškvietimo), kad neliktų ECU „pakibęs" sesijoje net atskiro mygtuko atveju.

**Priėmimo kriterijus:** po bet kurio individualaus skenavimo mygtuko (ne tik „Surinkti VISKĄ")
paleidimo, normalus PID polling'as (Pakopa 0/A) turi vėl grąžinti tikras reikšmes, ne
`7F 01 22` seriją.

---

## Taisymas 2: Apsauga nuo pasenusio atsakymo klaidingo priskyrimo

**Failas:** `android/www/obd.js`, funkcijos `pumpQueue()` ir `onDataLine()`

Dabartinė problema: timeout'ui suveikus, `STATE.pending` iškart nustatomas į `null` ir
`pumpQueue()` iškart siunčia kitą komandą — bet SENOS komandos atsakymas gali atkeliauti
VĖLIAU ir būti klaidingai priskirtas NAUJAI komandai.

**Sprendimas** — įvesti trumpą „nutildymo" (flush/settle) langą po timeout'o:

```js
function pumpQueue() {
    if (STATE.pending || STATE.queue.length === 0 || STATE.flushing) return;
    // ... esama logika nepakitusi iki timeout callback'o
}

// Timeout callback'e (dabartinis kodas):
STATE.pending.timer = setTimeout(() => {
    if (STATE.pending && STATE.pending.cmd === item.cmd) {
        obdLog('RX: (timeout, be atsakymo)', 'warn');
        const p = STATE.pending; STATE.pending = null;
        STATE.flushing = true;
        setTimeout(() => {
            STATE.flushing = false;
            pumpQueue();
        }, 250); // trumpas langas, kad pasenęs atsakymas atkeliautų ir būtų atmestas
        p.resolve(null);
    }
}, item.timeoutMs);
```

Ir `onDataLine()` funkcijoje — jei `STATE.pending` yra `null` IR `STATE.flushing === true`,
tiesiog tyliai atmesti (loguoti kaip „ignoruotas pasenęs atsakymas", NE „RX (nelaukta)"):

```js
function onDataLine(line) {
    if (STATE.flushing) {
        obdLog('(ignoruotas pasenęs atsakymas po timeout): ' + line, 'warn');
        return;
    }
    if (!STATE.pending) { obdLog('RX (nelaukta): ' + line, 'warn'); return; }
    // ... likusi esama logika nepakitusi
}
```

Papildomai: padidinti minimalius timeout'us modulių skenavime nuo 400ms iki **bent 600ms**
(realiuose duomenyse matyti atsakymai iki 587-625ms), kad sumažintume timeout'ų dažnį iš
principo, ne tik geriau tvarkytume jų pasekmes.

**Priėmimo kriterijus:** modulių skenavimo metu nebeturėtų matytis Δ<20ms „atsakymų" ar
„OK"/„STOPPED" reikšmių kaip 10 01/10 03 duomenų (nebent tai realiai teisingas ELM327
atsakymas tai konkrečiai komandai).

---

## Taisymas 3: Apsauga nuo pakartotinio skenavimo paleidimo

**Failas:** `android/www/obd.js`

Pridėti `STATE.scanBusy` (boolean) lauką. Kiekvienos iš šių funkcijų pradžioje:
`runIdentity`, `runProtocolProbe`, `runDidScan`, `runBlockSweep`, `runModuleScan`,
`runFullCollection` — patikrinti:

```js
async function runFullCollection() {
    if (STATE.scanBusy) {
        obdLog('Skenavimas jau vyksta — palaukite, kol baigsis, prieš paleidžiant iš naujo.', 'warn');
        return;
    }
    STATE.scanBusy = true;
    try {
        // ... esama runFullCollection logika
    } finally {
        STATE.scanBusy = false;
    }
}
```

Tą patį principą pritaikyti VISOMS penkioms atskiroms funkcijoms (kad ir individualaus
mygtuko paspaudimas negalėtų susikirsti su jau vykstančiu „Surinkti VISKĄ" ar kitu skenavimu).
UI pusėje (index.html) — kol `STATE.scanBusy === true`, visus 6 skenavimo mygtukus laikinai
`disabled`, kad vartotojas fiziškai negalėtų paspausti dar karto (papildoma apsauga virš JS
patikros).

**Priėmimo kriterijus:** paspaudus bet kurį skenavimo mygtuką, kiti skenavimo mygtukai tampa
neaktyvūs, kol vykstantis skenavimas nesibaigia arba nenutrūksta ryšys.

---

## Taisymas 4: SSE UNMATCHED fallback įrašai

**Failas:** `android/www/index.html`, didelė `else if (rawId.includes(...))` grandinė
(apie eilutę 2988, prieš galutinį `else { UNMATCHED }`)

Pridėti (jei dar nepridėta — vienas įrašas `junc_ah_total` jau buvo pradėtas ankstesnėje
sesijoje, patikrinti ar išliko po pertraukimo):

```js
else if (rawId.includes('fix_status') || rawId.includes('fix_statusas')) sensorCache['gps_fix_status'] = value;
else if (rawId.includes('paskutine_klaida') || rawId.includes('last_error')) sensorCache['last_error'] = value;
else if (rawId.includes('modemo_busena') || rawId.includes('modem_state')) sensorCache['modem_state'] = value;
```

Ir GSM dBm atveju — **prieš** esamą eilutę (nes dabartinė eilutė EXPLICITAI atmeta `dbm`):
```js
else if (rawId.includes('signalas_dbm') || rawId.includes('gsm_signal_dbm')) sensorCache['gsm_signal_dbm'] = value;
else if ((rawId.includes('gsm_signal') || rawId.includes('signalas_')) && !rawId.includes('dbm')) sensorCache['gsm_signal_pct'] = value; // ESAMA eilutė, nekeisti
```
(naują `dbm` eilutę įdėti PRIEŠ esamą `gsm_signal_pct` eilutę, kad else-if grandinė pirma
patikrintų dbm atvejį.)

**Priėmimo kriterijus:** paleidus app'ą su ESP32 ryšiu, jokie iš šių keturių sensorių
nebeturi rodytis kaip `[SSE UNMATCHED]` debug žurnale.

---

## Taisymas 5 (dokumentacija, ne kodas): Service 21 patvirtintos grupės

**Failas:** `docs/uzduotis_obd2_elm327_tab.md`

Pridėti naują poskyrį (pvz. po Pakopa B/C aprašymo) su patvirtintais realiais Service 21
(KWP2000 „measuring blocks") radiniais iš 2026-07-06 realaus testo:

| Grupė (hex) | Turinys | Patvirtinta |
|---|---|---|
| 0x50 (`21 50`) | Programinės įrangos ID: `BPG-810 02.12.07 --H07-- 1348 0104` | ✅ realus atsakymas |
| 0x51 (`21 51`) | VIN (dalinis/pilnas): `WV1ZZZ2EZ8602877...` — sutampa su žinomu VIN | ✅ realus atsakymas |
| 0x52 (`21 52`) | Kitas identifikacinis string'as (serijos nr.?) | ✅ realus atsakymas, reikšmė neiššifruota |

Pastaba: šios grupės — identifikaciniai duomenys, ne gyvi variklio parametrai (RPM, temp ir
pan.). Reikės TOLESNIO testo (su būsenos žymėjimu §3 punkte 8 aprašytu būdu — variklis
išjungtas/laisva eiga/apsukos), kad nustatytume, KURIOS grupės (galbūt 0x01-0x49 diapazone,
kur anksčiau matėme kintančius baitus, pvz. Grupė 30, 42 ir kt.) atitinka realius kintančius
parametrus, ne pastovias ID eilutes. Tai patvirtina 2026-07-06 pasiūlytą metodą (Service 21
veikia šiam EDC16), bet konkretaus grupė→parametras žemėlapio DAR NĖRA — kitas žingsnis
lieka empirinis testavimas su būsenos žymėmis, ne spėjimas.

---

## Taisymas 6: `ATST` padidinimas + auto-`ATZ` atsigavimas po pasikartojančių timeout'ų

**Failas:** `android/www/obd.js`, `initSequence()` ir `pumpQueue()`/timeout logika.

**6a. `ATST` (ELM327 vidinis laukimo laikas automobilio atsakymui) padidinimas.** Šis
parametras ATSKIRAS nuo mūsų JS `sendCmd(timeoutMs)` — `ATST` valdo, kiek PATS ELM327 laukia
atsakymo iš automobilio prieš praneisdamas „NO DATA", o mūsų JS timeout valdo, kiek MES laukiame
BET KO per Bluetooth. Trumpas numatytas `ATST` (default `32` = 200ms) gali generuoti klaidingus
„NO DATA" lėtai atsakantiems servisams (21/22, ypač skenavimo metu). Į `initSequence()` po
`ATSP6` pridėti:
```
ATST FF
```
(maksimalus laikas, ~1020ms = 0xFF×4ms). **Pastaba:** tai NEBŪTINAI ištaisys log'e matytą
atsijungimą (prieš jį matėme MŪSŲ PAČIŲ JS timeout'us, ne ELM327 „NO DATA" — tai rodo
fizinio ryšio problemą, ne per trumpą `ATST`), bet pagerins duomenų kokybę lėtai
atsakantiems Service 21/22 užklausimams.

**6b. Automatinis `ATZ` atsigavimas.** Sekti paeiliui einančių pilnų JS timeout'ų
(„(timeout, be atsakymo)", NE ELM327 „NO DATA") skaičių skenavimo metu (`STATE.consecutiveTimeouts`).
Jei pasiekia **3 iš eilės** — automatiškai:
1. Nusiųsti `ATZ` (adapterio perkrovimas).
2. Palaukti ~1500ms (ATZ pats užtrunka).
3. Iš naujo nusiųsti init seką (`ATE0`, `ATL0`, `ATH0`, `ATSP6`, `ATST FF`).
4. Atstatyti `STATE.consecutiveTimeouts = 0`.
5. Tęsti skenavimą nuo ten, kur sustojo (ne nuo pradžios).

**Riboti bandymų skaičių** — max **2 automatiniai `ATZ` atsigavimai per vieną skenavimo
sesiją** (`STATE.autoResetCount`). Jei ir po 2 atsigavimų vėl 3 timeout'ai iš eilės — nutraukti
skenavimą su aiškiu pranešimu vartotojui („Adapteris nebeatsiliepia po 2 bandymų atsigauti —
patikrinkite Bluetooth ryšį"), NE begalinė perkrovimo kilpa.

**Priėmimo kriterijus:** jei adapteris laikinai „pakimba" skenavimo metu, sesija turėtų
automatiškai atsigauti bent kartą, o ne iškart baigtis atsijungimu.

---

## Taisymas 7: „Saugus režimas" (pasirenkamas lėtesnis tempas)

**Failai:** `android/www/obd.js`, `android/www/index.html`

Naujas UI jungiklis OBD tab'e — „🐢 Saugus režimas (lėtesnis, stabilesnis)", numatyta **OFF**.
`STATE.safeMode` (boolean, saugomas `localStorage`). Kai įjungtas — po kiekvieno `sendCmd()`
išsprendimo (prieš siunčiant kitą komandą per `pumpQueue()`) papildoma **300ms** pauzė (virš
įprasto laukimo laiko), t.y. bendra pauzė tarp komandų tampa ~500ms vietoj įprasto ~200ms.
Skirta pigiems ELM327 klonams su mažu UART buferiu — lėčiau, bet stabiliau.

**Priėmimo kriterijus:** jungiklis matomas UI, veikia nepriklausomai nuo kitų nustatymų,
išlieka po app restarto (localStorage).

---

## Taisymas 8: Automatinis variklio būsenos žymėjimas (pakeičia rankinį mygtuką validacijai)

**Failas:** `android/www/obd.js`, `startPolling()`

Esami rankiniai būsenos mygtukai (§3 punktas 8, `tagState()`) **paliekami** kaip papildoma
galimybė, bet PAPILDOMAI — automatinis žymėjimas pagal RPM (`01 0C`) reikšmę Pakopa 0 polling
cikle. Sekti PASKUTINĘ žinomą RPM būseną (`STATE.lastEngineRunning`, boolean); kai RPM
perkerta 0 ↔ >0 ribą (**tik pasikeitus būsenai, NE kiekvieną poll'ą** — kitaip logas
užsipildys):
```js
const running = (rpmValue > 0);
if (running !== STATE.lastEngineRunning) {
    STATE.lastEngineRunning = running;
    obdLog('=== BŪSENA (auto): ' + (running ? ('Variklis veikia, RPM=' + Math.round(rpmValue)) : 'Variklis sustabdytas') + ' ===', 'warn');
}
```
Eliminuoja žmogiškąją klaidą (pamirštą mygtuką) ir leidžia tiksliai koreliuoti Service 21/22
duomenis su realia variklio būsena analizei.

**Priėmimo kriterijus:** paleidus/sustabdžius variklį testo metu, žurnale automatiškai
atsiranda atitinkama žyma be rankinio veiksmo.

---

## Taisymo 4 patikslinimas: substring kolizijos rizika

Pakeisti Taisymo 4 pasiūlytus `rawId.includes(...)` patikrinimus tikslesniais, kad išvengtume
atsitiktinių substring kolizijų su būsimais sensoriais (atitinka jau esamą kodo stilių, žr.
temperatūros filtro `!rawId.includes('esp_')` pavyzdį):
```js
else if (rawId.endsWith('_fix_statusas') || rawId.endsWith('_fix_status')) sensorCache['gps_fix_status'] = value;
else if (rawId.endsWith('_paskutine_klaida') || rawId.endsWith('_last_error')) sensorCache['last_error'] = value;
else if (rawId.endsWith('_modemo_busena') || rawId.endsWith('_modem_state')) sensorCache['modem_state'] = value;
else if (rawId.endsWith('_signalas_dbm_') || rawId.endsWith('_signal_dbm')) sensorCache['gsm_signal_dbm'] = value;
```
(Naudoti `endsWith`, NE papildomą tuščios reikšmės tikrinimą — joks kitas sensorius šiame
kode taip nesitikrina, nenuoseklu ir apsauga nuo realiai nepasitaikančio scenarijaus.)

---

## Bendri reikalavimai visiems taisymams

- **Nekeisti** nieko, kas nesusiję su šiais 4 taisymais (Renogy kodas, SSE grandinė,
  native Java bridge, versijos numeriai — jei nereikia versijos bump'o, nedaryti).
- Po pakeitimų: `node --check android/www/obd.js` (sintaksės patikra), patikrinti
  `index.html` inline script vis dar parsinasi be klaidų.
- Versijos bump: v47 → **v48** (versionCode, versionName, version.json, CURRENT_VERSION) —
  standartinis procesas kaip aprašyta `docs/uzduotis_obd2_elm327_tab.md` §7, su
  changelog paminint šiuos 4 stabilumo taisymus.
- **NEDARYTI** `git commit`/`push`/APK build be aiškaus vartotojo prašymo — palikti tai
  atskiram žingsniui po peržiūros.
