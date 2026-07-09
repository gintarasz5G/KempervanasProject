# Auditas 2026-07-09 — Pilnas app kodo ir veikimo auditas

**Auditorius:** Cowork Claude (tik analizė — kodo nekeičiau).
**Apimtis:** `android/www/obd.js`, `android/www/renogy.js`, `android/www/index.html`,
`android/android/app/src/main/java/lt/kemperis/app/{MainActivity,KemperisService,BootReceiver}.java`.
`ble-plugin.js` (vendored `@capacitor-community/bluetooth-le` UMD build) — nevertintas, ne projekto kodas.
**Metodas:** rankinis kodo skaitymas + 2 paraleliniai sub-agentai (index.html, native Java) su
nepriklausomu patikrinimu (top radiniai perskaityti ir patvirtinti tiesiogiai faile, ne tik
agento žodžiu).
**Susiję:** `docs/uzduotis_obd_elm327_realus_duomenys.md`, `docs/audits/auditas_2026-07-09_obd_ir_gyvo_testo_logai.md`.

---

## TL;DR — prioritetinis sąrašas

| # | Radinys | Sunkumas | Failas |
|---|---|---|---|
| 1 | PID bitmap ciklo bug — `supportedPids` niekad neužsipildo | **P0/HIGH** | `obd.js:277` |
| 2 | BT atsijungimas niekad neišsprendžia laukiančio `sendCmd` Promise → amžinas hang | **P0/HIGH** | `obd.js:566-574` + native |
| 3 | `connect()` be re-entry apsaugos → du threadai skaito tą patį socket | **HIGH** | `MainActivity.java:986-1014` |
| 4 | Socket/stream neuždaromi „iš išorės" atsijungus → leak | **HIGH** | `MainActivity.java:1067-1072` |
| 5 | Renogy/OBD sensorių reikšmės niekad neišvalomos atsijungus → rodo „gyvas" seną duomenį | **HIGH** | `renogy.js:238-243`, `obd.js:566-574`, `index.html:2211-2290` |
| 6 | `consecutiveTimeouts` neinicijuotas → `NaN` → auto-ATZ atsigavimas niekad nesuveikia | **MEDIUM/HIGH** | `obd.js:8-25,136,139` |
| 7 | SOC 0% gali būti tyliai perrašytas apskaičiuota reikšme → slepia realią „baterija tuščia" | **MEDIUM** | `index.html:1717-1725` |
| 8 | Aliarmų slenksčių tarpusavio priklausomybė netikrinama vartotojo nustatymuose | **MEDIUM** | `index.html:3216-3244,3767-3770` |

---

## 1. `obd.js` (ELM327 / Bluetooth Classic SPP)

### 1.1 P0 — PID bitmap ciklo bug (277 eil.)
```js
for (let bit = 0; b < 8; bit++) {   // TURI BŪTI: bit < 8
```
`b` yra baito reikšmė (0–255), ne skaitiklis — sąlyga nekinta ciklo eigoje:
- kai `b ≥ 8` (įprastas atvejis realiam ECU bitmap'ui, pvz. paties kodo komentaro pavyzdys
  `BE/1F/B8/11`) → vidinis ciklas **nė karto nesuveikia** → nė vienas PID nepažymimas kaip
  palaikomas.
- kai `b < 8` → **begalinis ciklas** (JS single-thread) → užšaldo WebView.

**Poveikis:** `STATE.supportedPids` praktiškai visada lieka tuščias → `startPolling()`
(315–316 eil.) mato `supported.length === 0` ir **niekad nieko nepoll'ina**. Visi kiti v53
pataisymai (PID lentelė, `ATSP0`, `runIdentityInternal` grąžinimas) tampa beprasmiai, nes
duomenys niekad nepasiekia UI net su veikiančiu varikliu. **Pataisymas: `bit < 8`.**

### 1.2 P0 — BT atsijungimas niekad neišsprendžia laukiančio `sendCmd` Promise (566–574 eil.)
```js
window.onObdDisconnected = function(reason) {
    ...
    STATE.pending = null;
    STATE.queue = [];
    ...
};
```
Bet koks tuo metu vykdomas `await sendCmd(...)` (viduje `initSequence()`, `runFullCollection()`,
`readDtcCodes()` ir kt.) turi savo `resolve` funkciją, saugomą **senos** `STATE.pending` objekto
kopijoje — kai ji čia tiesiog pakeičiama į `null`, `resolve()` niekad neiškviečiamas, laukiantis
`await` niekad nebaigia laukti. Vėliau suveikęs to komandos `setTimeout` taip pat nieko nedaro,
nes tikrina `STATE.pending && STATE.pending.cmd === item.cmd`, o `STATE.pending` jau `null`.

**Poveikis:** bet koks BT ryšio nutrūkimas vidury operacijos (o tai — dažnas atvejis su pigiu
ELM327 klonu, žr. audito 2026-07-09 O4) **amžinai** pakabina tą async grandinę, o kartu ir
`STATE.scanBusy = true`, kas blokuoja visus tolimesnius OBD veiksmus iki app perkrovimo.
**Pataisymas:** prieš `STATE.pending = null`, jei `STATE.pending` yra, iškviesti
`clearTimeout(STATE.pending.timer); STATE.pending.resolve(null);`.

### 1.3 MEDIUM/HIGH — `consecutiveTimeouts` niekad neinicijuotas (8–25, 136, 139 eil.)
`STATE` objekte nėra `consecutiveTimeouts` lauko. `STATE.consecutiveTimeouts++` (136 eil.) ant
`undefined` duoda `NaN`; `NaN >= 3` (139 eil.) visada `false`. **Auto-ATZ atsigavimas po 3
pasikartojančių timeout'ų (komentaras „Taisymas 6b") niekad nesuveikia** — funkcionalumas atrodo
kaip veikiantis kode, bet realybėje neaktyvus. **Pataisymas:** pridėti
`consecutiveTimeouts: 0` į STATE inicializaciją.

### 1.4 Žemesnio prioriteto
- `renderScanList()` (obd.js ~503 eil.) įterpia BT įrenginio `name` tiesiai į `innerHTML` be
  escaping (kitaip nei `obdLog`, kuris naudoja `escapeHtml`) — teorinis DOM injection per
  netikėtą BT device pavadinimą. Žema rizika (reikia fizinio BT artumo).
- `runProtocolProbeInternal()` (641 eil.) nuoroda į neapibrėžtą kintamąjį `uds` yra „mirusi",
  bet nepavojinga dabar, nes `udsOk` visada `false` (short-circuit); pavojinga tik jei kas nors
  vėl įjungtų UDS bandymą nepataisęs šios eilutės.

---

## 2. Native Android bridge (`MainActivity.java` — `KemperisObdBridge`)

### 2.1 HIGH — `connect()` be re-entry apsaugos (986–1014 eil.)
`connect()` niekada netikrina, ar `socket`/`in`/`out` jau užimti, ir neuždaro senos jungties
prieš perrašydamas laukus. `startReadLoop()` (1046–1074 eil.) kiekvieną iteraciją skaito
**instance lauką** `in` (ne lokalią kopiją) — jei `connect()` iškviečiamas pakartotinai kol sena
read-loop gija dar gyva (dvigubas paspaudimas, arba reconnect lenktynės su lėtai mirštančia sena
gija), sena gija savo kitoje iteracijoje pradės skaityti **naują** `in` srautą → du threadai
skaito tą patį `InputStream` vienu metu, duomenys gali susimaišyti prieš `>` prompt parser'į.

### 2.2 HIGH — Socket/stream neuždaromi „iš išorės" atsijungus (1067–1072 eil.)
Read-loop `finally` blokas tik nustato `connected=false` ir kviečia `evalJs(...)`, bet
**neuždaro** `in`/`out`/`socket` ir jų neišvalo (`= null`) — tai daro tik `disconnectInternal()`
(1036–1044 eil.). Kai adapteris atsijungia savaime (dažnas atvejis su pigiu klonu), lieka
neuždarytas `BluetoothSocket`/failo deskriptorius; kitas `connect()` tyliai perrašo lauką be
`close()` iškvietimo senajam objektui — nutekėjimas kiekvieną kartą.

### 2.3 MEDIUM — Neribotas buferis, jei `>` niekad neateina (1048 eil.)
`StringBuilder buf` auga be dydžio apribojimo. Triukšminga/sugedusi adapterio sesija be prompt'o
lėtai augins atmintį visą ryšio gyvavimo laiką.

### 2.4 MEDIUM — `KemperisService` pavadinimas/pranešimas žada daugiau nei daro
Servisas ir jo pranešimas („Stebimi sensoriai fone") sukuria įspūdį apie foninį OBD/dujų/CO2
stebėjimą, bet realus I/O vyksta tik WebView+JS lygyje — jei procesas nužudomas ir servisas
`START_STICKY` paleidžiamas iš naujo be Activity/WebView, joks realus stebėjimas/aliarmas
neveiks, nors vartotojui atrodys, kad sistema aktyvi. Saugumo funkcijai tai reikšminga spraga.

### 2.5 Žemesnio prioriteto
- `send()` (1017–1029 eil.) paleidžia naują `Thread` kiekvienam rašymui be `lock` ant `out`;
  normaliai serializuota per JS eilę, bet `handleAutoRecovery()` (`obd.js:160`) siunčia `ATZ`
  tiesiogiai apeidamas eilę — teorinės lenktynės su vėluojančiu ankstesnės komandos write thread.
- Permission handling (`hasConnectPerm()`/`hasScanPerm()`), `BootReceiver` manifest atitikimas,
  UI-thread `evalJs` naudojimas — patikrinti, problemų nerasta.

---

## 3. `index.html` + `renogy.js` (UI, SSE, alarms)

### 3.1 HIGH — Renogy/DCC/OBD sensorių reikšmės niekad neišvalomos atsijungus
`renogy.js` `onDisconnect()` (238–243 eil.) tik nustato `connected=false`, bet niekada neištrina
`sensorCache['ren_soc'|'ren_v'|'ren_a'|'dcc_*']`. `obd.js` `onObdDisconnected` (566–574 eil.)
elgiasi lygiai taip pat su `obd_*` reikšmėmis. `index.html` `setSensorValue` bindingai
(2211–2290 eil.) toliau rodo **paskutinę žinomą** reikšmę taip, lyg ji būtų gyva — nėra
per-lauko „sena/offline" indikatoriaus (tik bendras BT banner atspindi ryšio būseną, ne pačias
reikšmes). **Praktinis pavojus:** akumuliatoriaus įtampa/SOC gali valandų valandas rodyti
paskutinę žinomą „normalią" reikšmę po to, kai Renogy monitorius realiai nutilo.

### 3.2 MEDIUM — Aliarmų slenksčių tarpusavio priklausomybė netikrinama
`updateAlarmThreshold` (3767–3770 eil.) tikrina tik `!isNaN(val)` prieš išsaugant vartotojo
redaguotą slenkstį. Pvz. `water_low` sąlyga (3216–3221 eil.) yra
`v < 15 && v >= _siblingThreshold('water_critical', 5)`. Jei vartotojas per Nustatymus nustato
`water_critical` ≥ `water_low`, žemos rizikos aliarmas tampa **nepasiekiamas** (pvz.
`v<15 && v>=20` — niekad tiesa) ir nebeperspėja iš anksto. Tas pats šablonas kartojasi
`soc_low`/`soc_critical` (3234–3244) ir `co2_elevated`/`co2_danger` (3222–3233) porose.

### 3.3 MEDIUM/LOW — SOC 0% gali būti tyliai perrašytas apskaičiuota reikšme (1717–1725 eil.)
```js
if ((isNaN(soc) || soc <= 0) && !isNaN(ah) && ah > 0.5 && cap > 1) {
    sensorCache['junc_soc'] = Math.max(0, Math.min(100, Math.round((ah / cap) * 100)));
}
```
Skirta apsisaugoti nuo NaN/klaidingo 0, bet **neatskiria** „jutiklio klaida" nuo „baterija realiai
tuščia". Jei Junctek teisingai praneša 0%, o Ah likutis dėl kalibravimo dreifo vis dar >0.5,
realus 0% tyliai pakeičiamas „raminančia" apskaičiuota procentine reikšme **prieš** patenkant į
`soc_critical`/`soc_low` aliarmų logiką (3234–3244) — kritinis „baterija tuščia" signalas gali
niekad nepasiekti aliarmo.

### 3.4 MEDIUM (defensyvumo spraga) — formatter'iai be apsaugos nuo netikėtų reikšmių
`setSensorValue` (2335–2341 eil.) kviečia formatter'į be try/catch. Daug formatter'ių
(`ren_v`/`ren_a`/`dcc_*`/`obd_maf`/`obd_ecu_v`/`obd_fuel_rate` ir kt.) kviečia `.toFixed()`
tiesiai ant cache reikšmės be `typeof`/`parseFloat` apsaugos (kitaip nei `sys_uptime`
formatteris, 1970–1972 eil., kuris apsaugotas). Šiuo metu neveikia blogai, nes `renogy.js`/`obd.js`
visada įrašo skaičius — bet bet koks būsimas pakeitimas, įrašantis string/`undefined`, sukels
neperhvatytą `TypeError` ir **tyliai nutrauks** likusią to `updateUI()` ciklo dalį (visi po jo
eilėje esantys sensorių bindingai tą tiką „nedings" į UI).

### 3.5 Žemesnio prioriteto
- SSE parseris (2941–2950 eil.) `try/catch` apgaubia tik `JSON.parse`; jei `state` event neturi
  nei `name_id`, nei `id`, `rawId = (data.name_id || data.id).toLowerCase()` mes uncaught
  `TypeError`, tyliai praranda tą vieną SSE pranešimą.
- BME680 temperatūros fallback filtras (2978 eil.) `if (value > 45.0) return;` tyliai numeta bet
  kokią >45°C reikšmę (net jei tai reali sugedusio jutiklio būsena) tik su debug-lygio logu, be
  vartotojui matomo „klaida/sensorius neveikia" indikatoriaus.
- Kryžminė patikra: `SENSOR_ID_MAP` (880–930 eil.) ir visi `setSensorValue` bindingai
  (1765–2290 eil.) patikrinti — **nerasta** negyvų DOM ID nuorodų ar nesutampančių raktų.

---

## 4. Bendra rizikos vertinimo santrauka

Kritiškiausia grandinė: **#1 (bitmap bug) → #6 (auto-recovery neveikia) → #2 (disconnect hang)**
kartu paaiškina, kodėl OBD funkcionalumas gali atrodyti „ištaisytas kode", bet realiame teste
(net su veikiančiu varikliu) vis tiek nerodyti nieko ir/arba pakibti po pirmo BT nutrūkimo.
**#3/#4 (native connect race + leak)** yra tos pačios simptomatikos giluminė priežastis pigaus
ELM327 klono aplinkoje (dažni atsijungimai → dažni reconnect'ai → didesnė race/leak tikimybė).

**#5 (stale sensor values)** ir **#7 (SOC masking)** yra saugumo/patikimumo klausimai, nesusiję
su OBD užduotimi — vartotojas gali pasikliauti sena/klaidinančia baterijos informacija kempinge.

## 5. Rekomenduojama taisymo tvarka

1. `obd.js:277` — `bit < 8` (vieno simbolio pataisa, atrakina visą PID funkcionalumą).
2. `obd.js` STATE — pridėti `consecutiveTimeouts: 0`.
3. `obd.js` `onObdDisconnected` — `resolve()`+`clearTimeout()` esamam `STATE.pending` prieš
   nustatant `null`.
4. `MainActivity.java` `connect()` — prieš naują jungtį uždaryti/palaukti senos read-loop gijos
   pabaigos; `finally` bloke read-loop'e taip pat uždaryti socket/streams (ne tik `connected=false`).
5. `renogy.js`/`obd.js` disconnect handleriuose — išvalyti atitinkamus `sensorCache` raktus, kad
   UI aiškiai rodytų „—"/offline, ne paskutinę žinomą reikšmę.
6. `index.html` — SOC 0% fallback logiką apriboti (pvz. netaikyti, jei `soc === 0` tiksliai ir
   ryšys su Junctek šviežias), ir pridėti kryžminę slenksčių validaciją Nustatymuose
   (low ≥ critical → įspėti/atmesti).

**Failų priminimas** (žr. ankstesnę analizę, [[kempervanas-release-capsync.md]]): visos šios
pataisos turi būti daromos `android/www/*`, **NE** `android/android/app/src/main/assets/public/*`
(tas — build artefaktas, kurį perrašo `npx cap sync`). Po pataisų — `npx cap sync` + build.
