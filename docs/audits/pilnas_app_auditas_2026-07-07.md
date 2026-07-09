# Pilnas programėlės auditas — 2026-07-07 (v52 / v35.5)

**Apimtis:** visa Android programėlė (`android/www/index.html`, `renogy.js`, `obd.js`,
`ble-plugin.js`, `capacitor.js`) + kryžminis patikrinimas su firmware
(`firmware/kempervanas.yaml`) ir Apps Script. Klausimai: (1) ar kodas švarus ir funkcionalus,
(2) ar visos kortelės gauna teisingus duomenis ir laiku, (3) ar visi aliarmai logiški ir
tikrai suveiks, (4) kokiais scenarijais viskas veiks.

**Metodas:** statinis kodo auditas (tikri Windows failai), kryžminis SSE mapping patikrinimas,
palyginimas su oficialia ESPHome / Apps Script / renogy-bt dokumentacija. Fizinis įrenginys
netikrintas (PC ne Kemperio AP tinkle). Remiasi ir tęsia ankstesnius auditus
(`veikimo_auditas_2026-06-29_SSE_mapping.md`, `gilus_meta_auditas_2026-07-03_v42.md`,
`auditas_2026-07-02_vakaras_v38.md`).

---

## 0. Verdiktas (TL;DR)

Programėlė funkciškai **stabili ir gerai sutvarkyta**: kortelių→sensorių susiejimas beveik
100 % teisingas, aliarmų logika nuosekli, filtrai/watchdog'ai robustūs. Nuo birželio 29 d.
audito **pataisyti** anksčiau rasti trūkumai (BME temperatūros užteršimas, GPS fix statusas,
GSM dBm, modemo būsena — visi dabar turi SSE taisykles). Renogy R2/R3/R6 pataisos pritaikytos.

Bet yra **du strateginiai P0 rizikos taškai** ir keli funkciniai spragos, kuriuos verta žinoti:

| # | Prioritetas | Radinys |
|---|---|---|
| **A1** | 🔴 P0 (naujas, laiko juosta!) | ESPHome atnaujinimas **sugriaus visą SSE kortelių srautą** — žr. §2.1 |
| **A2** | 🔴 P0 (žinomas) | Firmware SMS komandos nevalidyja siuntėjo; OTA/API/web_server/Apps Script be auth |
| **B1** | 🟠 P1 | SOC / vandens / dujų kritiniai aliarmai firmware'e **nesiunčia SMS** — tik Google Sheets įrašas (§3.3) |
| **B2** | 🟡 P2 | Git darbinė kopija — nesukommit'inti pakeitimai (obd.js „v53", index.html/MainActivity M); reikia švaraus commit + `node --check` (§5) |
| **C**  | 🟢 kosmetika | Komentarų/dokų neatitikimai, monolitinis index.html, negyvas mapping kodas |

---

## 1. Kaip kortelės gauna duomenis (patvirtinta)

Trys šaltiniai (nepakito nuo `app_korteliu_duomenu_saltiniai.md`):

1. **ESP32 SSE** (`http://192.168.4.1/events`, `EventSource`, `index.html:2902`) — event-driven
   push; kortelės dažnis = sensoriaus `update_interval` firmware'e.
2. **Google Sheets** (offline fallback) — kai SSE tyli, kas 60 s traukiamos paskutinės reikšmės.
3. **App skaičiuoja** (energija Wh, maršruto statistika, žvejyba, vietinis CSV, domkratai).

**Renogy/OBD** — atskiras kelias: skaitoma tiesiogiai per planšetės BLE (`renogy.js`) ir
ELM327 (`obd.js`), ne per ESP SSE.

### 1.1 Kortelių→sensorių susiejimas — dabartinė būsena ✅

Kryžmiškai patikrinta SSE dispečerio grandinė (`index.html:2952–3031`). Palyginus su birželio 29 d.
auditu, **pataisyta reikšmingai daugiau nei tada**:

| Anksčiau UNMATCHED / klaida | Būsena dabar | Įrodymas |
|---|---|---|
| Akum. temp. užteršia BME temp. (§2.1 kritinė) | ✅ Ištaisyta — `else if` grandinė + `>45°C` filtras | `index.html:2965`, `junc_temp` pagaunama anksčiau (2961) |
| GPS Fix statusas nerodomas | ✅ Pridėta taisyklė | `index.html:2987` `endsWith('_fix_statusas')` |
| GSM Signalas (dBm) nerodomas | ✅ Pridėta taisyklė | `index.html:2990` |
| Modemo būsena / Paskutinė klaida | ✅ Pridėtos taisyklės | `index.html:2988–2989` |
| Akum Total Ah | ✅ Susieta | `index.html:2960` `junc_ah_total` |

**Išvada:** visos pagrindinės kortelės gauna teisingą sensorių ir teisingu dažniu
(akum 5 s, vanduo/dujos 5 s, GPS 5–10 s, BME/miegamasis 30 s, sistema 60 s). Nerastas nė
vienas realus „bloga reikšmė ne iš to sensoriaus" atvejis dabartiniame kode.

Likę **nekenksmingi** (nenaudojami UI, „⚪"): `binary_sensor` entitetai (220V dubliuojasi per
`power_source_220` text_sensor), Junctek Raw HEX, negyvas `number`-entitetų mapping sensorių
grandinėje (kalibravimas realiai ateina per `num_*` REST — veikia).

### ⚠️ 2.1 P0 (A1): ESPHome atnaujinimas sugriaus VISĄ kortelių srautą

Tai **svarbiausias techninis radinys** ir jis laikinai aktualus (dabar 2026-07).

App SSE dispečeris remiasi **senuoju** `id` lauko formatu: `domenas-object_id`, kur
`object_id` = slugifikuotas lietuviškas `name:` (pvz. `"GPS | Greitis"` → `sensor-gps___greitis`).
Visi ~60 `rawId.includes('...')` tikrinimų suderinti su šiuo slug'u.

Pagal oficialią ESPHome dokumentaciją (web_server v3 pakeitimai, `esphome#12627`):

- **2026.7.0** — senieji `object_id` URL'ai **pašalinami**; įvedami entiteto-vardu grįsti URL'ai.
- **2026.8.0** — SSE `id` laukas **pakeičiamas** į naują formatą `name_id` = `domenas/entiteto_vardas`
  (tikrasis `name:` su tarpais/UTF-8), o pereinamasis `name_id` laukas pašalinamas.
- Pereinamuoju laikotarpiu SSE payload turi **abu** laukus: `id` (senas) ir `name_id` (naujas).

**Pasekmė:** kai tik firmware bus perkompiliuotas su ESPHome ≥ 2026.8.0, `id` taps
`sensor/GPS | Greitis` (su tarpais, „|", didžiosiomis) — ir dauguma `includes()` taisyklių
**neberas atitikmens** → kortelės masiškai nustos gauti duomenis ([SSE UNMATCHED] terminale).

**Rekomendacijos (užduotis asistentui, ne dabar keisti):**
1. **Trumpam:** užrakinti ESPHome versiją (`< 2026.7.0`) firmware build aplinkoje, kol app atnaujintas.
2. **Ilgam:** SSE dispečeryje pereiti prie `name_id` lauko (jei yra) su fallback į `id`; arba
   normalizuoti abu (`toLowerCase()`, `|`/`/`/tarpą → `_`) prieš `includes()`. Tada veiks
   nepriklausomai nuo ESPHome versijos.
3. Įsidėti į gyvo testo checklist: po firmware atnaujinimo — patikrinti `[SSE UNMATCHED]` logą.

> Nesusiję su dabartiniu veikimu: **su esamu firmware viskas veikia**. Tai spąstai ateičiai,
> kurie suveiks tyliai (be klaidos, tik tuščios kortelės) būtent atnaujinus ESPHome.

---

## 3. Aliarmai — logika, ar suveiks, scenarijai

Aliarmai yra **dviejuose sluoksniuose**: firmware (autonominis, siunčia SMS + Sheets) ir app
(balsinis TTS + UI, kai planšetė įjungta). App aliarmai remiasi firmware paskaičiuotomis
reikšmėmis per SSE.

### 3.1 App aliarmų variklis (`index.html:3197–3246`, `checkAlarms` 3664, `checkTheftVoice` 3708)

Vykdomas kas 5 s (`setInterval` 2847) + prie kiekvieno SSE įvykio (2279). Histerezė ir
pakartojimai apgalvoti:

| Aliarmas | Slenkstis | Reset (histerezė) | Kartojimas | Logiška? |
|---|---|---|---|---|
| 💧 Vanduo kritinis | <5 % | ≥8 % | 30 min | ✅ |
| 💧 Vanduo žemas | <15 % (ir ≥5) | ≥20 % | 30 min | ✅ (sibling — nesidubliuoja su kritiniu) |
| 🔋 SOC kritinis | <15 % | ≥18 % | 30 min | ✅ |
| 🔋 SOC žemas | <25 % (ir ≥15) | ≥28 % | 30 min | ✅ |
| 🔥 Dujos žemas | <10 % | ≥15 % | 30 min | ✅ |
| 🛏️ CO₂ pavojingas | >2500 ppm | <2000 | 1 min | ✅ |
| 🛏️ CO₂ padidėjęs | >1800 (ir ≤2500) | <1500 | 1 min | ✅ |
| 💧 Kondensatas | temp<8 °C IR drėgmė>65 % | temp≥10 ARBA drėgmė≤60 | — | ✅ |
| 🚨 Vagystė | `alert_movement_sent==true` | firmware nuima vėliavą | 30 s stovint / 2 min važiuojant | ✅ |

Papildomos apsaugos: `snooze` (nutildymas su auto-clear kai sąlyga atsistato), `alarmFlags`
išsaugoja būseną `localStorage` (išlieka po app minimize), `_focusAlarmCard` perjungia į kortelę.

**Debesų aliarmai** (`checkCloudAlarms` 2636): tik whitelist `water/soc/gas/condensation`
(**be vagystės** — teisinga, nes debesų duomenys pasenę iki 20 min), tikrina duomenų amžių
(`_cloudDataAgeMin` atmeta >20 min / neigiamą). ✅ Logiška.

### 3.2 Firmware aliarmai (`kempervanas.yaml:2130–2226`) — autonominiai

- **Vagystė (ADXL):** reikia `sw_security_armed` + `park_stable_count ≥ 6` (stabilus park_ref)
  + roll/pitch nuokrypis **>3°** → `alert_movement_sent=true`, sekimas, GPS pažadinamas
  (`AT+CGNSSPWR=1`), **SMS** (`sms_send_security_alert`), Sheets įrašas su `speed=-1` žymekliu.
  Sekimas baigiamas kai vėl stabilu (`park_stable_count ≥ 12`). ✅ Logiška ir suveiks.
- **Vagystė (GSM CLBS trianguliacija):** kas 30 min kai apsauga įjungta; poslinkis >800 m
  (ir tikslumas <800 m) → SMS. ✅
- **CO₂:** smailių filtras — aliarmas tik kai **2 iš 3** matavimų >2500 ppm (~60 s) →
  `sms_send_co2_alert` + Sheets. Reset kai <1800. ✅ Gerai apsaugota nuo vienkartinių smailių.
- **Kritinis SOC ≤25 %, vanduo ≤10 %, dujos ≤10 %:** įrašomi į Google Sheets su histereze
  (reset 30 %/15 %/15 %). Stovint imamos „stabilios" reikšmės (po 60 s nusistovėjimo). ✅ reikšmės

### 3.3 🟠 P1 (B1): SOC/vandens/dujų kritinis — NĖRA SMS

**Radinys:** firmware bloke (2193–2225) SMS siunčiamas **tik** vagystei (`crit_movement`) ir
CO₂ (`crit_co2`, 2187). `crit_soc` / `crit_water` / `crit_gas` **tik** įrašomi į Google Sheets
ir nustato `alert_low_*_sent` vėliavas — **SMS nesiunčiamas**.

**Pasekmė / scenarijus:** jei planšetė išjungta (app neveikia) ir baterija nusėda iki 20 %,
vanduo iki 8 % ar dujos iki 8 % — vartotojas **negaus jokio įspėjimo nuotoliniu būdu**.
Įspėjimas ateis tik: (a) atsidarius app (balsu/UI iš gyvų arba debesų duomenų), arba
(b) peržiūrėjus Google Sheets rankiniu būdu. CO₂ ir vagystė — vieninteliai, kurie „pasiekia"
telefoną be app.

CLAUDE.md §10 formuluotė („SOC ≤25 %, vanduo ≤10 %, dujos ≤10 %, CO₂ >2500 ppm — su SMS")
**dviprasmiška** — kodas „su SMS" taiko tik CO₂. Verta apsispręsti:
- Jei taip ir norėta (SMS tik CO₂/vagystei — saugumo/energijos taupymui) → patikslinti CLAUDE.md.
- Jei norima SMS ir SOC/vandens/dujų kritiniams → tai firmware užduotis (pridėti
  `sms_send_*` iškvietimus po `crit_soc/water/gas`, su rate-limit).

### 3.4 🟡 Smulkūs aliarmų neatitikimai (kosmetika)

- `checkTheftVoice` komentaras (3732) sako „stovint kas 30 s", bet kodas naudoja `60000` ms
  (60 s) stovint (`isDriving ? 120000 : 60000`). CLAUDE.md §9 irgi mini 30 s (tai firmware
  Sheets įrašo periodas, ne balso). Balsas stovint kartojamas kas **60 s** — patikslinti komentarą.
- Vagystės app-aliarmas priklauso nuo `gps_speed_sensor > 3` „važiuoja" nustatymui; jei GPS
  miega/be fix (greitis NaN→0), skaičiuojama „stovi" → balsas kas 60 s. Praktiškai OK.

---

## 4. Saugumas (A2, P0 — žinoma skola, patvirtinta)

| Vektorius | Būsena | Įrodymas |
|---|---|---|
| SMS siuntėjo autorizacija (firmware) | ❌ Nevalidijama | `sms_sender` naudojamas tik istorijai; komandos vykdomos iš bet kokio numerio |
| OTA slaptažodis | ❌ Nėra | `ota:` be `password:` |
| API encryption key | ❌ Nėra | `grep encryption:` = 0 |
| web_server auth | ❌ Nėra | — |
| Apps Script token | ❌ Nėra | `GoogleSheetsScript.js` |

**Naujiena (teigiama):** app-pusėje `sendOfflineSms` (2554–2573) **jau** validyja `+arm`/`+disarm`
prieš admin numerį ir savo telefono numerį. Bet tai gina tik nuo klaidingo siuntimo iš pačios
app — **firmware vis tiek įvykdys `+arm`/`+disarm`/`+ikelk` iš BET KOKIO siuntėjo**, kuris žino
kemperio numerį ir sintaksę. SMS eina viešu GSM tinklu → realiausias vektorius.

**Prioritetas:** Etapas A (kaip numatyta CLAUDE.md) — firmware SMS whitelist prieš vykdant
komandą. Mažiausiai pastangų, didžiausia rizikos redukcija.

---

## 5. Kodo švarumas ir higiena

**Gerai:**
- Modularizacija progresuoja: `renogy.js` (502 eil.), `obd.js` (779 eil.), `ble-plugin.js`,
  `capacitor.js` atskirti nuo monolito. Sveikintina.
- `node --check obd.js` / `renogy.js` praeina (tikrinta prieš tikrus failus).
- Robustūs filtrai: median+EMA, spike atmetimas, BLE watchdog, modem semaforas, SSE watchdog
  (60 s tyla → atkūrimas, 2 min → offline).
- 0 TODO/FIXME žymų kode (skola gyvena `docs/audits/`).
- Renogy R2 (`ren_temp = s16(data[36],data[37])/10.0`, renogy.js:315) ir R6 (celės `/10.0`,
  305–308) — **pritaikyti**. R3 stadijų žemėlapis (374–377) atitinka renogy-bt.

**Tobulintina:**
- `index.html` — **3839 eilutės viename faile** (monolitas). Kitas modularizacijos žingsnis:
  SSE dispečeris, CSV, aliarmų variklis į atskirus `.js`.
- **Nėra automatinių testų.** Renogy parseris ir 22-laukų CSV kontraktas — puikūs kandidatai
  unit-testams (jau įrodyta izoliuojami/paleidžiami Node'e).
- **Negyvas mapping kodas** (3001–3018): `water_empty_cm`, `gas_full_weight`, `hx711_scale`
  ir kt. sensorių grandinėje niekada nesuveikia (tai `number` entitetai). Nekenkia, bet triukšmas.
- Latentinės `renogy.js`: B1 (multi-frame — apdoroja tik 1 kadrą/notifikaciją; realiai maža
  rizika, nes buferis resetinamas kiekvienai užklausai), B2 (`u32()` be `>>> 0` — neigiamas
  ties ≥2³¹; `dcc_ah_total`/`wh_total` gyvavimo skaitikliai teoriškai gali persipildyti).

**🟡 B2 — git darbinė kopija (patikrinti asistentui):**
Nekommit'inti pakeitimai: `obd.js` (mtime šiandien, komentaras „v53" nors `versionCode`=52
visur kitur), `index.html`, `MainActivity.java`, `renogy.js` — visi ` M`. Prieš laikant
darbą baigtu: (1) `node --check` abiem JS; (2) suvienodinti versijos numerį (52 arba pakelti
į 53 visose 3 vietose); (3) `git status` + `git diff --stat` peržiūra prieš commit (pagal
liepos 2 d. „worktree truncation" pamoką — jei diff'e vien ištrynimai failo gale, failas
nukirstas). *Pastaba: šios sesijos Linux sandbox mount rodė kai kuriuos didelius failus
nukirstus — tai buvo sinchronizacijos artefaktas; tikri Windows failai pilni ir tvarkingi.*

---

## 6. Online palyginimas (patvirtinta prieš šaltinius)

- **ESPHome web_server SSE `id` formatas** — patvirtinta oficialioje dokumentacijoje, kad
  keičiasi 2026.7.0/2026.8.0 (žr. §2.1). Tai naujas, laiku aktualus radinys.
- **Apps Script 302** — patvirtinta: Apps Script Web App **visada** grąžina 302 redirect POST'ui
  (į `script.googleusercontent.com`). Firmware `success = HTTP 200 ARBA 302` (`kempervanas.yaml`
  logeris) — **teisinga**; buferis valomas tik po patvirtintos sėkmės. ✅
- **renogy-bt protokolas** — R2/R3/R6 masteliai atitinka `cyrils/renogy-bt` šaltinį (patikrinta
  ankstesniame audite, pritaikymas patvirtintas šiame). ✅
- **Junctek BLE** — savas BCD parseris; SOC = Ah likutis / `num_bat_capacity`. Skalės teisingos
  (imitacija ankstesniame audite). ✅

---

## 7. Scenarijų sąrašas — kada VISKAS veiks, o kada NE

### ✅ Scenarijai, kuriais viskas veikia kaip numatyta

1. **Planšetė prijungta prie „Kemperis-Valdymas" AP, ESPHome esamos versijos:** visos kortelės
   gauna gyvus SSE duomenis teisingu dažniu; aliarmai (balsas+UI) suveikia; Renogy/OBD per
   planšetės BLE atskirai. Pilnas funkcionalumas.
2. **Vagystė kai apsauga įjungta ir įrenginys stabiliai pastatytas (park_ref nustatytas):**
   posvyris >3° → firmware siunčia SMS + pradeda GPS sekimą + Sheets įrašus; app (jei atidaryta)
   kartoja balsu. Suveikia autonomiškai net be planšetės.
3. **CO₂ pakyla >2500 ppm miegant (2 iš 3 matavimų):** firmware SMS + Sheets; app balsu kartoja
   kas 1 min. Suveikia be planšetės.
4. **Offline (SSE tyli >60 s):** app pereina į Google Sheets fallback (kas 60 s), rodo pasenimo
   juostą, debesų aliarmai (vanduo/SOC/dujos/kondensatas) su amžiaus patikra.
5. **GSM trianguliacija pavogus kai nėra GPS fix:** CLBS poslinkis >800 m → SMS.

### ⚠️ Scenarijai, kuriais NEVEIKS arba veiks ribotai

1. **Planšetė išjungta + kritinis SOC/vanduo/dujos:** ❌ jokio SMS (tik Sheets įrašas) — §3.3 (B1).
   Vartotojas nesužinos, kol neatsidarys app ar Sheets.
2. **Firmware perkompiliuotas su ESPHome ≥ 2026.8.0:** ❌ SSE `id` formatas pasikeičia → dauguma
   kortelių tuščios (`[SSE UNMATCHED]`), be jokio klaidos pranešimo — §2.1 (A1).
3. **Piktavalis žino kemperio SIM numerį:** ⚠️ gali `+arm`/`+disarm`/`+ikelk` iš bet kokio
   telefono — firmware nevalidyja siuntėjo — §4 (A2).
4. **Vagystė kai park_ref dar nenustatytas** (`park_stable_count < 6`, ką tik pastatyta/dar
   vibruoja): ADXL aliarmas neaktyvuojamas, kol nusistovi (~saugiklis nuo klaidingų). Trumpas
   „aklas" langas iškart po pastatymo — tikėtina, kad taip ir norėta.
5. **Renogy DCC50S stovint ilgai:** galimas BLE ryšio time-out (Renogy PRO aparatūros ypatumas,
   ne kodo klaida; log lygis jau nuleistas iki WARN). Duomenys atsistato patys.
6. **SMS su lietuviškomis raidėmis:** firmware siunčia tik ASCII (GSM 7-bit) — teksto koduotė
   ribota, bet tai sąmoninga.

---

## 8. Prioritetų rekomendacijos (užduotims asistentui)

1. **A1 (P0, laiko juosta):** apsaugoti nuo ESPHome SSE `id` pakeitimo — užrakinti ESPHome
   versiją IR/ARBA app SSE dispečerį padaryti atsparų (normalizuoti `id`/`name_id`). *Firmware
   + app užduotis.*
2. **A2 (P0):** Etapas A — firmware SMS siuntėjo whitelist; `ota.password`; `api.encryption.key`;
   web_server auth; Apps Script token.
3. **B1 (P1):** apsispręsti dėl SMS SOC/vandens/dujų kritiniams (arba pridėti firmware, arba
   patikslinti CLAUDE.md, kad SMS = tik CO₂/vagystė).
4. **B2 (P2):** švarus git commit + `node --check` + versijos suvienodinimas (obd.js „v53" vs 52).
5. **C (kosmetika):** komentarų taisymas (30 s→60 s), negyvo mapping valymas, unit-testai
   Renogy/CSV, tolesnė index.html modularizacija, `renogy.js` B1/B2 pataisos (`while` ciklas,
   `u32 >>> 0`).

---

*Auditą atliko: Claude (Cowork, auditoriaus vaidmuo — kodas nekeistas). 2026-07-07.
Bazė: v52 (v35.5). Statinis kodo + dokumentacijos auditas; fizinis įrenginys netikrintas.*
