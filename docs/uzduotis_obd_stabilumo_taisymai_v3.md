# Užduotis: OBD testavimo apimties apribojimas + Renogy baterijos taisymai

**Data:** 2026-07-07 · **Bazinė versija:** v50 (versionCode 50, versionName "35.3")
**Kontekstas:** Analizavau `docs/logs/obd_log_2026-07-07T*.txt` (5 failai, 2 pilnos „Surinkti
VISKĄ" sesijos) + `docs/logs/terminal_log_2026-07-07T08-31-01.txt`, palyginęs su
2026-07-06 žurnalais ir esamu `android/www/obd.js`/`android/www/renogy.js` kodu (v50 — po
Taisymų 1-8 ir Bug#1/#2 pataisų). **Neimplementuoti jokių papildomų funkcijų be aiškaus
prašymo** — tik žemiau išvardinti pakeitimai.

---

## ⚠️ PATIKRINIMO STATUSAS (atnaujinta po `git show a1efe0a`)

Agentas jau atliko darbą ir **sukūrė + PUSH'INO** commit'ą `a1efe0a` („🚀 v35.4 - OBD Scope
Optimization & Renogy Hardware Hardening (v51)") į `origin/main` — nepaisant šio dokumento
tiesioginio nurodymo apačioje „**NEDARYTI** `git commit`/`push`/APK build be aiškaus
vartotojo prašymo". **Tai reikia paminėti vartotojui** — jei tai nebuvo aiškiai prašyta
atskirai, tai nukrypimas nuo instrukcijų.

### Kas patikrinta ir PATVIRTINTA teisingai (peržiūrėjus `git show a1efe0a` diff'ą):

- **Dalis A1** (Mode 01 polling išjungtas) — ✅ `startPolling()` iškvietimai užkomentuoti
  ir po `initSequence()`, ir `runFullCollection()` pabaigoje.
- **Dalis A2** (Mode 09 identitetas pašalintas iš `runFullCollection()`) — ✅ padaryta.
- **Dalis A4** (modulių skenavimo ATCS bug) — ✅ **TEISINGAI pataisyta** TIKSLIAI pagal
  nurodymą: `ATCS` dabar siunčiamas tik PRIEŠ ir PO `for` ciklo, NE kas 25 adresus per vidurį.
  Agentas PAPILDOMAI pridėjo `&& r1.trim() !== 'OK'` patikrą prie `ok1`/`ok2`
  ([obd.js:552-553](../android/www/obd.js#L552)) — protingas papildomas apsaugos sluoksnis
  virš to, ką prašiau (filtruoja atvejį, kai vien „OK" būtų klaidingai palaikytas „atsakymu").
  `runModuleScanInternal()` taip pat pašalintas iš `runFullCollection()` automatinės sekos.
- **Dalis B** (Renogy temp. jutikliai 3/4) — ✅ **PATVIRTINTA, NE spėjimas**: agentas rado, kad
  `data[34..35]` IŠ TIKRŲJŲ yra jutiklių skaičiaus registras (reg 5017) ir realiuose
  duomenyse rodo tik 2 aktyvius jutiklius — patvirtina mano hipotezę iš pokalbio. Kodas dabar
  sąlygiškai užpildo `ren_temp`/`ren_temp_1` tik jei `tempCount>=N`, IR **`delete`** naudoja
  `ren_temp_2`/`ren_temp_3` iš `sensorCache` (ne tik palieka 0) — tai geriau nei mano pasiūlytas
  UI-only sprendimas, nes UI net negaus klaidinančios „0.0°C" reikšmės.
- **Dalis C** (Renogy „Writing characteristic failed") — ✅ implementuota TIKSLIAI kaip
  pasiūlyta: `type === 'battery'` → `writeWithoutResponse()`, DCC50S nepaliestas. Pakartotinio
  siuntimo blokas pašalintas visiškai (ne tik komentuotas).

### Kas DAR NEATLIKTA / reikia dėmesio:

1. **Versijos bump NEPADARYTAS.** Commit pavadinimas sako „v51", bet
   `build.gradle` versionCode vis dar **50**, `MainActivity.CURRENT_VERSION` **50**,
   `version.json` **50**. Tai pažeidžia projekto taisyklę
   (`CURRENT_VERSION == versionCode == version.json version`) — reikia arba realiai pakelti
   iki 51 visur trijuose vietose + changelog, arba pataisyti commit'o pavadinimą, kad
   neklaidintų.
2. **NĖRA jokio naujo test'o po pataisymų.** Visi `docs/logs/*.txt` failai yra IKI commit'o
   (paskutinis — 11:31, commit — 13:21). Nei Dalies B (temp. jutikliai realiu BLE), nei
   Dalies C (writeWithoutResponse realiu BLE) pakeitimai **NĖRA patvirtinti realiu testu** —
   abu šio dokumento priėmimo kriterijai („realiame teste... 10 min be klaidos") tebėra
   neįvykdyti. **Kitas žingsnis — realus testas su automobiliu IR Renogy baterija.**
3. **Dalis A3** (UDS `22F190` komentaras) — nepaliesta, bet tai buvo pažymėta „NEBŪTINA".
4. **Dalis D** (Service 21 KWP tipo-baito dekoderis) — tuo metu, kai agentas dirbo, ši dalis
   dokumente DAR NEEGZISTAVO (pridėta vėliau tolimesnėje pokalbio sesijoje) — **dar
   neimplementuota**. Tai dabar didžiausias likęs darbas — žr. Dalį D žemiau.

---

## Dalis A — OBD: testuoti tik tai, kas GALI duoti atsakymą

### Radinys A1: Standartiniai Mode 01 PID — 100% patvirtinta NEVEIKIA šiam ECU

`startPolling()` ([obd.js:219-251](../android/www/obd.js#L219)) ciklu kas 600ms siunčia
`ALL_PIDS` = `PID0` + `PID_A` ([obd.js:28-45](../android/www/obd.js#L28)): `010C, 010D, 0105,
0104, 010B, 0110, 0123, 015C, 012C, 012D, 017A, 017C`.

**Įrodymas:** 2026-07-07 žurnaluose šie PID siųsti nenutrūkstamai nuo 11:22:54 iki bent
11:30:46 (>8 min, per kelis failus) — **kiekvienas** atsakymas buvo `NO DATA`, be išimties.
Tai vyko **net po** rankinio žymo `=== BŪSENA: Laisva eiga ===` (11:23:10, `obd_log_...
08-22-54.txt:648`) — t.y. variklis buvo pažymėtas kaip veikiantis, o standartiniai PID vis
tiek negrąžino nieko. Tas pats matyta ir 2026-07-06 žurnaluose. Trys nepriklausomos sesijos,
skirtingos dienos, skirtingos variklio būsenos — **0 sėkmingų atsakymų iš dešimčių tūkstančių
bandymų**. Šis EDC16 ECU per šį ELM327+CAN kelią standartinių SAE Mode 01 PID tiesiog
neatsako — realūs duomenys ateina tik per nuosavybinį Service `21` (KWP measuring blocks,
adresas `0x7E0`).

**Taisymas:** Išjungti automatinį `startPolling()` paleidimą po `initSequence()`
([obd.js:212-216](../android/www/obd.js#L212), eilutė `startPolling();`). Palikti funkciją
kode (galimam regresijos patikrinimui kitoje transporto priemonėje), bet **NEKVIESTI** jos
automatiškai. Vietoj to, po inicijavimo tiesiog laukti vartotojo veiksmo (skenavimo mygtukų).
Jei norima, pridėti atskirą mygtuką „Bandyti standartinius PID (gali neveikti šiam ECU)" —
**NEBŪTINA**, tik jei nesunku.

### Radinys A2: Mode 09 (ECU identitetas) — 100% patvirtinta NEVEIKIA

`runIdentityInternal()` ([obd.js:437-445](../android/www/obd.js#L437)) siunčia `0902` (VIN),
`0904` (Calibration ID), `090A` (ECU Name) su 3000ms timeout kiekvienai (9s iš viso).

**Įrodymas:** `docs/logs/obd_log_2026-07-06T16-55-47.txt:307-330` — visos trys komandos
grąžino `NO DATA` (šis konkretus fragmentas be konkurentiškumo bug'o įtakos, žr. Taisymo v2
radinį #3 — čia sekos aiškios). VIN vis tiek jau žinomas per kitą, PATVIRTINTAI veikiantį
kanalą: Service 21 grupė `0x51` (`docs/uzduotis_obd_stabilumo_taisymai.md` Taisymas 5).

**Taisymas:** Pašalinti `await runIdentityInternal();` iš `runFullCollection()`
([obd.js:578](../android/www/obd.js#L578)). Pačią `runIdentity()`/`runIdentityInternal()`
funkciją ir mygtuką „1️⃣ ECU identitetas (Mode 09)" ([index.html:609](../android/www/index.html#L609))
**palikti** (nekenkia kaip atskiras rankinis bandymas, bet nebeturi būti dalis automatinio
pilno rinkimo, kad netrukdytų 9s be reikalo kiekvieną kartą).

### Radinys A3: Service 22 UDS (`22F190`) — patvirtinta NEVEIKIA

`runProtocolProbeInternal()` ([obd.js:460-469](../android/www/obd.js#L460)) siunčia
`2101` (KWP) IR `22F190` (UDS VIN). **Įrodymas:** `22F190` grąžino `NO DATA`
mažiausiai 3 kartus skirtinguose žurnalo fragmentuose (`obd_log_2026-07-06T16-55-47.txt:388,
454`). O `2101` (Service 21, grupė 1) **kartais grąžindavo realius baitus**
(`obd_log_2026-07-06T16-52-42.txt:188-192`) — tai patvirtina, kad KWP kelias veikia, UDS ne.

**Taisymas:** `runProtocolProbeInternal()` viduje palikti `2101` zondą (jis pigus ir
patvirtina KWP kelio veikimą), bet `22F190` eilutę ([obd.js:463](../android/www/obd.js#L463))
pažymėti komentaru `// UDS 22F190 patvirtinta NEVEIKIA (2026-07-06/07 žurnalai) — laikoma
tik dokumentacijai/regresijai, NEBŪTINA siųsti kiekvieną kartą`. Jei norima sutrumpinti —
galima šią eilutę ištrinti ir vietoj `uds`/`udsOk` naudoti fiksuotą `false` su komentaru,
**bet tik jei tai nesukomplikuoja funkcijos grąžinamos reikšmės naudojimo kitose vietose**
(patikrinti, ar `runProtocolProbeInternal()` grąžinama `{kwpOk, udsOk}` reikšmė kur nors
naudojama pagal `udsOk`).

### Radinys A4 (svarbiausias): Modulių skenavimas (0x700-0x7FF) — sisteminga klaida, 0 naudingų radinių per 3 sesijas

`runModuleScanInternal()` ([obd.js:542-563](../android/www/obd.js#L542)) kas 25 adresus
siunčia papildomą `ATCS` zondą ([obd.js:554-557](../android/www/obd.js#L554)). Išanalizavau
šiandienos duomenis tiksliai: „atsakantys" adresai `0x797, 0x7B0, 0x7C9, 0x7E2, 0x7FB` yra
**lygiai +1** nuo adresų, kuriuose suveikia periodinis `ATCS` (`0x796, 0x7AF, 0x7C8, 0x7E1,
0x7FA`) — atstumas tarp jų **visada lygiai 25**, tiksliai sutampa su `(addr - 0x700) % 25`
sąlyga. Tai **deterministinis** įrodymas, kad šie „atsakymai" yra `ATCS` komandos sukeltas
atsakymų eilės poslinkis per vieną žingsnį (sekančio adreso `1001` gauna PRAĖJUSIOS
komandos pavėluotą `OK`/`STOPPED`), NE realūs ECU. Be to, `isNoData()`
([obd.js:195-197](../android/www/obd.js#L195)) neatpažįsta grynos `OK` reikšmės kaip
artefakto (tik `STOPPED`/`?`/`NO DATA` ir pan.), todėl `ok1 = !!r1 && !isNoData(r1)`
([obd.js:549](../android/www/obd.js#L549)) klaidingai patvirtina „atsakymą" gavus vien „OK".
3 nepriklausomos pilnos 0x700-0x7FF sesijos (2× 2026-07-07 + anksčiau 2026-07-06) **niekada
nerado jokio kito realaus modulio** be jau žinomo `0x7E0`.

**Taisymas (du žingsniai):**
1. Pašalinti periodinį `ATCS` zondą iš skenavimo ciklo ([obd.js:554-557](../android/www/obd.js#L554))
   — jis nesuteikia jokios patikimos diagnostinės vertės (bendra CAN būsena nesikeičia per
   skenavimą), o vienintelis matomas efektas — laiko juostos ardymas. Jei norima išlaikyti
   CAN būsenos patikrinimą, atlikti jį **PRIEŠ** pradedant `for` ciklą ir **PO** jo pabaigos
   (2 kartus iš viso), NE kas 25 adresus per vidurį.
2. Pašalinti `await runModuleScanInternal();` iš `runFullCollection()`
   ([obd.js:579](../android/www/obd.js#L579)) — kadangi 3 sesijos nerado nieko naujo,
   šis ~90s trunkantis skenavimas neturi būti dalis kiekvieno „Surinkti VISKĄ". Palikti kaip
   atskirą rankinį mygtuką „4️⃣ Modulių skenavimas" (jau yra, [index.html:613](../android/www/index.html#L613))
   retkarčiais pakartotinam patikrinimui (pvz. jei bus keičiama transporto priemonė ar
   įtariama nauja magistralė), **BET TIK PO** 1 punkto pataisymo (kad rezultatai būtų
   patikimi, ne artefaktai).

### Ką PALIKTI kaip pagrindinį testavimo įrankį

`runBlockSweepInternal()` (Service 21, `0x01-0xFF` per `0x7E0`,
[obd.js:517-526](../android/www/obd.js#L517)) yra **vienintelis** kanalas, patvirtintai
grąžinęs realius duomenis (grupės `0x50/0x51/0x52`, žr. `docs/uzduotis_obd_stabilumo_
taisymai.md` Taisymas 5). `runDidScanInternal()` ([obd.js:485-501](../android/www/obd.js#L485))
irgi palikti — kandidatų sąrašas dar nepilnai išbandytas, ir jis automatiškai krenta į
block sweep, jei nieko neranda. **Naujame `runFullCollection()`** liktų:
`runProtocolProbeInternal()` (trumpas sanity check) → `runDidScanInternal()` (su auto-fallback
į block sweep) → grąžinimas į default sesiją (`1001`). Tai sutrumpina „Surinkti VISKĄ" laiką
maždaug **~100s** (9s Mode09 + ~90s modulių skenavimas), nekeičiant naudingų duomenų kiekio.

**Priėmimo kriterijus:** paspaudus „Surinkti VISKĄ", žurnale nebelieka `0902/0904/090A`
komandų, nebelieka `0x700-0x7FF` modulių skenavimo, o pagrindinis rezultatas — Service 21
DID/block sweep duomenys. Standartinis Mode 01 polling automatiškai nebesipaleidžia po
inicijavimo.

---

## Dalis B — Renogy baterijos kortelė: temperatūros jutikliai 3 ir 4 rodo 0

**Failas:** `android/www/renogy.js`, `parseFrame()` ([renogy.js:307-311](../android/www/renogy.js#L307))

```js
window.sensorCache['ren_temp']   = s16(data[36], data[37]) / 10.0;
window.sensorCache['ren_temp_1'] = s16(data[38], data[39]) / 10.0;
window.sensorCache['ren_temp_2'] = s16(data[40], data[41]) / 10.0;
window.sensorCache['ren_temp_3'] = s16(data[42], data[43]) / 10.0;
```

**Pastebėjimas iš šiandienos žurnalo** (`terminal_log_2026-07-07T08-31-01.txt:337`, hex
`ff 03 44 00 04 00 24 00 24 00 24 00 24 00 00...00 02 00 a0 00 a1 00 00...00 4d 08`):
matomos tik DVI prasmingos ~16°C reikšmės viena po kitos, o likusi temperatūrų srities dalis
— vien nuliai. **Prieš darant išvadą, kad tai dekodavimo bug'as, PRIVALOMA patikrinti**
(neatspėti) šią hipotezę pagal 3 šaltinius:

1. `docs/logs/*.txt` — surinkti KELETĄ (bent 5) skirtingų realių `battery:` HEX frame'ų
   (ne tik vieną), tiksliai suskaičiuoti baitų offset'us nuo `data[0]` (po `ff 03 44`,
   prieš CRC) — patikrinti, ar registras **prieš** temperatūrų bloką (t.y. `data[34..35]`,
   registras 5017 pagal esamą komentarą „reg 5018..5021") reiškia **jutiklių skaičių**.
2. `https://github.com/cyrils/renogy-bt` (`PROTOCOL.md` ir `battery.py`/panašus parseris) —
   patikrinti, ar ten aprašytas atskiras „cell temperature count" registras prieš pačias
   temperatūras (analogiškai jau žinomam „cell count" registrui `data[0..1]`).
3. Bent vienas trečias šaltinis (Home Assistant Renogy integracijos issue'ai, arba kitas
   GitHub projektas su Renogy Modbus registrų žemėlapiu) — palyginti registrų išdėstymą.

**Jei pasitvirtina, kad jutiklių skaičiaus registras rodo `2`** (t.y. ši konkreti 100Ah
LiFePO4 baterija turi tik 2 fizinius NTC jutiklius, o registrai 5020-5021 tiesiog neužpildyti
gamykloje) — **TAI NĖRA BUG'AS**. Sprendimas: UI pusėje (rasti Renogy kortelės HTML bloką,
kur rodomi `ren_temp_2`/`ren_temp_3`) rodyti „—" arba paslėpti šiuos laukus, kai jutiklių
skaičiaus registras < 4, o NE klaidinantį „0.0°C" (kuris atrodo kaip realus šaltas
matavimas). **Jei pasitvirtina, kad registras rodo `4`** (t.y. baterija TEIGIA turinti 4
jutiklius, bet 2 iš jų vis tiek 0) — tuomet tai arba (a) realus techninis gedimas
(neprijungtas/sugedęs jutiklis — informuoti mane, NE taisyti kaip programinę klaidą), arba
(b) offset'ų klaida kode — taisyti tik jei radote patvirtinimą iš 2+ šaltinių, kokie TIKRI
offset'ai turėtų būti.

**Priėmimo kriterijus:** aiškiai nustatyta ir dokumentuota (trumpu komentaru kode virš
[renogy.js:307](../android/www/renogy.js#L307)), kodėl jutikliai 3/4 rodo 0 — su nuoroda į
patikrintą registrų šaltinį, NE spėjimu. UI nebeklaidina vartotojo, jei tai hardware
apribojimas.

---

## Dalis C — „Renogy battery query failed: Writing characteristic failed"

**Failas:** `android/www/renogy.js`, `queryDevice()` ([renogy.js:245-265](../android/www/renogy.js#L245))

**Radinys:** ši klaida **KIEKVIENĄ KARTĄ** ištinka BATERIJOS užklausas (abu bandymus — ir
pirminį, ir pakartotinį, žr. `terminal_log_2026-07-07T08-31-01.txt:249-252`), bet **NIEKADA**
DCC50S užklausas (patikrinau visus `docs/logs/*.txt` — nė vieno `dcc query failed` įrašo).
Svarbiausia: **iš karto po** kiekvienos „nepavykusios" baterijos rašymo klaidos žurnale
matomas **realus, teisingos CRC HEX atsakymas** (pvz. `terminal_log_...:249-252` — po abiejų
„failed" pranešimų du kartus iškart pasirodo TAS PATS `ff 03 0c 00 00 00 90 00 01 ad 10...`
kadras). Tai reiškia:

1. `BleClient.write()` (WRITE_TYPE_DEFAULT, laukiama patvirtinimo) komanda **fiziškai
   pasiekia** įrenginį ir gauna atsakymą IŠ TIKRŲJŲ abu kartus — pati Android/Capacitor BLE
   sluoksnio „rašymo" patvirtinimo callback'as tiesiog neateina laiku arba klaidingai
   praneša klaidą šiam konkrečiam charakteristikos tipui (jau užfiksuota kodo komentare
   „R5" — [renogy.js:255-257](../android/www/renogy.js#L255)).
2. Kadangi klaida spėjama „retkarčiais", esamas vienkartinis pakartojimas
   ([renogy.js:259-263](../android/www/renogy.js#L259)) IŠ TIKRŲJŲ **kas kartą** iššaukia
   antrą, PERTEKLINĮ rašymą — baterija gauna TĄ PAČIĄ komandą du kartus per vieną poll
   ciklą, todėl gaunami du (paprastai identiški) atsakymo kadrai. Tai dvigubina BLE srautą
   baterijai be jokios naudos ir nereikalingai teršia žurnalą klaidos pranešimais.

**Hipotezė (tikrinti, NE atspėti):** DCC50S charakteristika `0000ffd1-...` priima
`WRITE_TYPE_DEFAULT` normaliai, o BATERIJOS BLE modulio (galimai kitokio gamintojo UART
tiltas nei DCC50S viduje) ta pati charakteristika realiai palaiko tik `Write Without
Response`, todėl Android’o `writeCharacteristic()` su patvirtinimo laukimu visada
grąžina klaidą, nors baitai vseg tiek išsiunčiami.

**Ką padaryti:**
1. Prieš keičiant kodą „aklai" — patikrinti realias charakteristikos savybes: po
   `BleClient.connect()` baterijos įrenginiui iškviesti `BleClient.getServices(dev.id)`
   ir žurnale (`obdLog`/`debug`) išvesti `properties` masyvą CHAR_WRITE
   (`0000ffd1-...`) charakteristikai. Palyginti su tuo pačiu DCC50S įrenginiui.
2. Jei baterijos charakteristika NETURI `write` (tik `writeWithoutResponse`) — tai
   patvirtina hipotezę. Sprendimas: `queryDevice()` viduje pridėti sąlygą — jei
   `type === 'battery'` (arba geriau: dinamiškai pagal aptiktas savybes iš 1 punkto),
   naudoti `BleClient.writeWithoutResponse(dev.id, SERVICE_WRITE, CHAR_WRITE, fullCmd)`
   vietoj `BleClient.write(...)` ([renogy.js:253](../android/www/renogy.js#L253)).
   **NEKEISTI** DCC50S kelio, nes jis jau veikia be klaidų su `write()`.
3. Po pakeitimo pašalinti (arba palikti kaip papildomą apsaugą, bet NEBESITIKĖTI, kad jis
   suveiks) vienkartinį pakartojimo blokelį ([renogy.js:259-263](../android/www/renogy.js#L259)),
   jei `writeWithoutResponse` išsprendžia klaidą visiškai — kad nebeliktų dvigubo siuntimo.

**Priėmimo kriterijus:** po pakeitimo, realiame teste (BT prijungtas prie tikros baterijos)
`sysLog`/OBD-style žurnale nebelieka nė vieno `Renogy battery query failed` įrašo per
mažiausiai 10 min. nepertraukiamo veikimo, o `ren_*` sensoriai tebeatsinaujina reguliariai
(kas ~10s) be dubliuotų identiškų HEX kadrų iš eilės.

---

## Dalis D — Service 21 duomenų dekodavimas: rastas TIKSLUS VAG KWP2000 tipo-baito formulių žemėlapis

**Kontekstas:** Peržiūrėjau konkurentų APK (OBD Auto Doctor, ScanMaster-ELM, DashCommand —
žr. pokalbio istoriją) IR paieškojau GitHub/Ross-Tech/Rusijos automobilių diagnostikos
portaluose (vwts.ru, drive2.ru, audi-bel.com), kad suprastume, kaip komercinės programėlės
dekoduoja VAG Service 21 duomenis. Radau **konkretų, patikrintą** atsakymą — tai NĖRA
spėjimas, o realus atviro kodo (GPLv3) projekto sprendimas.

### Radinys D1: Service 21 atsakymo FORMATAS su savaime aprašančiu tipo baitu

Šaltinis: [jazdw/vag-blocks](https://github.com/jazdw/vag-blocks) (GitHub, GPLv3, autorius —
tas pats Jared Wiltshire, kurio TP2.0 dokumentaciją jau cituoja `docs/uzduotis_obd2_elm327_tab.md`).
Projekto `kwp2000.cpp` atskleidžia tikrąjį Service 21 (`21 XX`) teigiamo atsakymo formatą:

```
61 <blokas> [tipas1 A1 B1] [tipas2 A2 B2] [tipas3 A3 B3] [tipas4 A4 B4]
```

T.y. **12 duomenų baitų = 4 laukai po 3 baitus**, ir kiekvieno lauko PIRMASIS baitas yra
**tipo kodas**, nurodantis, kokia formule/vienetais interpretuoti likusius du baitus (A, B).
Tai reiškia: **NEREIKIA žinoti konkretaus grupės numerio prasmės iš anksto** — pakanka
dekoduoti kiekvieną 3-baičių lauką pagal jo tipo kodą, ir dažnu atveju iškart matysis, ar tai
RPM, temperatūra, °C, % ir pan., nepriklausomai nuo to, kurią grupę paklausėte.

**Pilna žinoma tipo kodų lentelė** (verbatim iš `kwp2000.cpp` `decodeBlockData()`):

| Tipas (hex) | Vienetai | Formulė (A, B — baitai po tipo baito) |
|---|---|---|
| `0x01` | rpm | `A*B/5.0` |
| `0x04` | ° ATDC/BTDC (jei B>127 → ATDC, kitaip BTDC) | `abs(B-127)*0.01*A` |
| `0x07` | km/h | `0.01*A*B` |
| `0x08` | Binary | `(A<<8)\|B` |
| `0x10` | Binary | `(A<<8)\|B` |
| `0x11` | ASCII | 2 simboliai: `chr(A)+chr(B)` |
| `0x12` | mbar | `A*B/25.0` |
| `0x14` | % | `A*B/128.0 - 1` |
| `0x15` | V | `A*B/1000.0` |
| `0x16` | ms | `0.001*A*B` |
| `0x17` | % | `B*A/256.0` |
| **`0x1A`** | **°C** | **`B - A`** ⚠️ tikėtina temperatūrų formulė (aušinimo skystis, alyva, oro) |
| `0x21` | % | jei A=0: `100*B`, kitaip `100*B/A` |
| `0x22` | kW | `(B-128)*0.01*A` |
| `0x23` | l/h | `A*B/100.0` |
| `0x25` | Binary | `(A<<8)\|B` |
| `0x27` | mg/stk (įpurškimo kiekis) | `A*B/256.0` |
| `0x31` | mg/stk | `A*B/40.0` |
| `0x33` | mg/stk Δ (korekcija) | `((B-128)/255.0)*A` |
| `0x36` | Count | `A*256+B` |
| `0x37` | s | `A*B/200.0` |
| `0x51` | ° CF | `(A*112000 + B*436)/1000` |
| `0x5E` | Nm | `A*(B/50.0 - 1)` |
| kita | Raw | `(A<<8)\|B` (nežinomas tipas) |

**Svarbu:** grupės 0x50/0x51/0x52 (jau patvirtintos VIN/SW-ID) matomai naudoja tipą `0x11`
(ASCII, po 2 simbolius per lauką) arba panašią teksto koduotę — patikrinti, ar tai sutampa
su jau užfiksuotais realiais baitais.

### Taisymas D1: Implementuoti šį dekoderį

**Failas:** `android/www/obd.js` — pridėti naują funkciją (pvz. šalia `isNoData()`):

```js
function decodeKwpField(type, a, b) {
    // Šaltinis: jazdw/vag-blocks (GPLv3) kwp2000.cpp decodeBlockData() — patikrinta lentelė,
    // žr. docs/uzduotis_obd_stabilumo_taisymai_v3.md Dalis D.
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
    // hexResp — švarus hex string be tarpų, pvz. "610144B900..." (61, blokas, 4x[tipas,A,B])
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

Panaudoti `decodeKwpBlockResponse()` `runBlockSweepInternal()` viduje ([obd.js:517-526](../android/www/obd.js#L517)) — vietoj vien žalio hex loginimo, papildomai iškviesti dekoderį ir žurnale rodyti IR žalią hex, IR dekoduotą (tipas+reikšmė+vienetai) kiekvienam laukui, kad testo metu iškart matytųsi prasmingos reikšmės (pvz. „24.3 rpm, 82°C, 1013 mbar...").

### Taisymas D2 (NEMOKAMAS, atlikti PRIEŠ kitą realų testą): dekoduoti JAU turimus duomenis

Prieš važiuojant vėl su automobiliu — pritaikyti aukščiau esantį dekoderį (gali būti laikinas
Node.js/Python skriptas, nebūtina iš karto keisti `obd.js`) VISIEMS jau užfiksuotiems
`21 XX` atsakymams iš `docs/logs/obd_log_2026-07-06T16-5*.txt` (ten yra Service 21 block
sweep su realiais duomenimis, pvz. Grupė 1 = `61 01 01 69 00 27 46 00 64 45 05 05 09 9B 31
C8 00 12 FF 61 12 FF 61 21 FF 00 00` — **PASTABA:** šis konkretus pavyzdys IŠ TIKRŲJŲ turi
daugiau nei 14 baitų dėl `[SSE UNMATCHED]`-tipo žurnalo formatavimo klaidų (eilutėse yra
įterpti „0:"/„1:"/„2:"/„3:" žymenys, kurie NĖRA duomenų dalis — juos pirma reikia išvalyti
prieš dekoduojant). Tai gali **iš karto**, be jokio naujo važiavimo, atskleisti, kurios
grupės yra RPM/temperatūra/slėgis vs. tekstinės ID eilutės — **nemokamas** žingsnis prieš
tolimesnį darbą.

### Papildoma gili paieška — konkretus ECU identifikuotas + teisinga variklio šeima

Antra paieškos banga (GitHub + Rusijos portalai) davė ŽYMIAI tikslesnį rezultatą:

**Konkretus ECU:** realiame Crafter 2.5 TDI **BJK** remonto atvejyje (rusiškas forumas
[carmasters.org](https://carmasters.org/topic/15131-crafter-25tdi-bjk-%D0%BD%D0%B5%D1%82-%D0%B8%D0%BC%D0%BF%D1%83%D0%BB%D1%8C%D1%81%D0%BE%D0%B2-%D0%BD%D0%B0-%D1%84%D0%BE%D1%80%D1%81%D1%83%D0%BD%D0%BA%D0%B8/))
ECU blokas identifikuotas kaip **EDC16CP34**. Verta ateityje ieškoti specifiškai
„EDC16CP34 measuring blocks"/„EDC16CP34 группы измеряемых величин" — tikslesni rezultatai
nei bendras „EDC16".

**Svarbi PATAISA ankstesniam radiniui:** VW Crafter BJJ/BJK/BJL/BJM yra **5 cilindrų R5 TDI
su siurbliu-purkštuku (Pumpe-Düse/PD unit injector)**, NE Common Rail — tai reiškia
ankstesnė nuoroda į `vwts.ru` CBAB straipsnį (Common Rail EDC17) yra **NETINKAMA variklio
šeima** šiam automobiliui. Teisingas analogas — PD/unit-injector VAG TDI grupių struktūra,
kurią dabar patvirtina **trys nepriklausomi šaltiniai** (drive2.ru Audi A4 B8, TDIClub forumas
1.9 PD varikliui, bendra VAG TDI bendruomenės žinyno struktūra) — jos SUTAMPA tarpusavyje:

| Grupė | Turinys (PD/unit-injector VAG TDI, patvirtinta 3 šaltiniuose) |
|---|---|
| 0 / 1 | RPM + įpurškimo kiekis (mg/H arba mg/gūž.) + įpurškimo trukmė/kampas + aušinimo skysčio temp. |
| 2 | Tuščios eigos reguliavimas: RPM + pedalo/droselio padėtis + režimo būsena + temp. |
| 3 | EGR: RPM + norimas oro srautas (MAF) + realus MAF + EGR vožtuvo DC % |
| 4 | Purkštuko laikas: RPM + įpurškimo pradžios kampas + trukmė + sinchronizacijos kampas |
| 10 / 11 | Turbo/pripūtimo slėgis: RPM + norimas/realus slėgis + wastegate DC % |
| 12 | Kaitinimo žvakės: būsenos bitai (pre-glow/start/interim/post-glow/ready fazės) |
| 13 | Cilindrų balansas — įpurškimo kiekio korekcija kiekvienam cilindrui atskirai (±2.8 mg/H tipinis diapazonas) — **klasikinis VAG „smooth running" diagnostikos blokas**, naudojamas rasti blogą purkštuką |
| 14 | Papildomos purkštuko korekcijos (susijusi su 13, tiksliau neaprašyta) |

Šaltiniai: [TDIClub „VAG-COM: Measuring blocks in-depth"](https://forums.tdiclub.com/showthread.php?p=1502390)
(1.9 PD, dalies nr. 038-906-019), [drive2.ru Audi A4 B8](https://www.drive2.ru/l/487603721178448015/),
[T5 2.5 TDI info threads](https://www.vwwatercooled.com.au/forums/f136/2-5-tdi-transporter-bpc-2009-info-thread-133253-2.html)
(patvirtina: „Measuring blocks 013 and 014 give info on how the injectors are fueling" būtent
2.5 TDI unit-injector varikliams, EDC16U1/EDC16U31 — tos pačios R5 TDI variklio šeimos kaip
Crafter, tik ankstesnė karoserijos versija).

**Papildomas oficialus šaltinis (mechaninei/jutiklių sąrangai, ne grupių numeriams):**
VW savišvietos programa Nr. 305 „The 2.5 l R5 TDI engine — Design and function" (SSP 305) —
[PDF](https://vwcampersite.wordpress.com/wp-content/uploads/2015/01/ssp_305-the-2-5-r5-tdi-engine.pdf) —
oficialus VW gamyklinis šio TIKSLIAI variklio (2.5 R5 TDI, ta pati šeima kaip Crafter) aprašymas
su visais jutikliais/vožtuvais — naudinga suprasti, KO IŠ VISO tikėtis matuojamuosiuose blokuose.

### Trečia paieškos banga — dar tikslesnis ECU dalies numeris + „pjezo" mįslė išspręsta

**Tikslus VW/Bosch dalies numeris rastas** (B-Parts/WorldECU atsarginių dalių katalogai):
Crafter 30-50 (2E_) 2.5 TDI originalus ECU — **VW Nr. 074906032AF**, **Bosch Nr. 0281013699**
(EDC16CP34 šeima, atitinka carmasters.org radinį). Tai naudingas raktas ieškant `.lbl`/`.clb`
žymėjimo failo (žr. žemiau).

**„Pjezo purkštukų" mįslė IŠSPRĘSTA:** tas pats paieškos šaltinis atskleidė, kad VĖLESNĖS
Crafter (2E) kartos su **EDC17CP20** ECU (VW Nr. 076906022C, Bosch 0281015747, variklio kodas
**CEBB**) TIKRAI naudoja Common Rail su pjezo purkštukais. Kadangi Bosch **piezo purkštukai
yra IŠIMTINAI Common Rail technologija** (patvirtinta tiesiogiai Bosch svetainėje) — o mūsų
automobilis yra **2008 m.** (pagal CLAUDE.md), taigi PRIEŠ EDC17/CEBB pereinamąjį laikotarpį —
šis automobilis **PATIKIMAI yra BJJ/BJK/BJL/BJM + EDC16CP34, PD/unit-injector (solenoidiniai
purkštukai, NE pjezo)**. Ankstesnis carmasters.org „pjezo" paminėjimas tikriausiai buvo apie
KITĄ (vėlesnės kartos EDC17/CEBB) automobilį tame pačiame forume, arba forumo dalyvio
netiksli terminija. **Šis klausimas dabar laikytinas išspręstu**, PD/EDC16 prielaida stipriai
patvirtinta — nebereikia to tikrinti realiu testu kaip neišspręstos abejonės.

**Praktiškas patarimas ateičiai (VCDS Labels):** MHH AUTO forumo gijoje patvirtinta, kad
tikroji Ross-Tech VCDS programinė įranga turi lokalų `Labels/` aplanką su **paprasto teksto
`.LBL` failais** (senesnio formato, atidaromi Notepad'u), pavadinimo šablonu
`<VW dalies nr.>-906-<priesaga>-<variantas>.LBL` (pvz. matytas pavyzdys
`022-906-032-BDB.LBL`). Jei kada nors atsirastų prieiga prie VCDS (net trial/demo versijos —
tik peržiūrai, NE kodavimui), failas pavadinimu artimu **`074-906-032-XX.LBL`** turėtų
turėti TIKSLŲ, gamintojo patvirtintą Crafter EDC16 grupių→parametrų žemėlapį — tai būtų
GALUTINIS, autoritetingas šaltinis, pranokstantis visą šio dokumento spėjimą iš bendruomenės
šaltinių. **Nebūtina** dabar to ieškoti, bet paminėti kaip žinomą, greičiausią kelią, jei
progresas per Service21 sweep empiriką sustotų.

### Ketvirta paieškos banga — ribos pasiektos

Patikrinau GitHub `.lbl` failų talpyklas (`gnits/vag-labels`, `dittohead/vcds_labels`,
`risty/VCDS_Labels`) — visos per mažos (1-4 failai), nė viena NETURI Crafter/EDC16/074-906
atitikmens. Taip pat per `gh search code` patvirtinau, kad **realus firmware dump'as tikslaus
ECU variantui `074906032BB`** egzistuoja bendruomenės chip-tuning archyve
(`darkenSVK/chiptuning.pw_ecu-backup` katalogo sąrašas — pats archyvas talpina TIK
pavadinimus/numerius, ne pačius dump'us; tikri failai būtų `chiptuning.pw` svetainėje, kuri
reikalauja registracijos/mokėjimo). Iš tokio dump'o teoriškai būtų galima ištraukti vidines
eilutes (panašiai kaip darėme su APK), bet tai jau **firmware reverse engineering**, ne
paprasta žiniatinklio paieška — atskira, žymiai didesnės apimties užduotis, kurią rekomenduoju
daryti TIK jei D1-D3 (tipo-baito dekoderis + offline turimų duomenų dekodavimas) neduoda
pakankamai rezultatų. **Šiuo momentu paieška interneto šaltiniuose pasiekė praktinę ribą** —
tolimesnis progresas priklauso nuo realių duomenų dekodavimo (D2) arba prieigos prie
tikros VCDS programos.

**Grupių 13/14 papildomas patvirtinimas** (dieselirk.ru + avtoadapter.ru rusiški forumai,
nepriklausomi nuo drive2.ru/TDIClub): korekcijos reikšmės tipiškai **±1...±1.5** (šiek tiek
siauresnis diapazonas nei anksčiau rastas ±2.8 — verta traktuoti kaip apytikslę orientaciją,
ne tikslią ribą, kol nepatvirtinta realiais duomenimis). Neigiama reikšmė → adatos dėvėjimasis/
nuotėkis; teigiama → užsikimšęs purkštukas. **Pastaba iš bendruomenės:** ~50% atvejų, kai
korekcijos reikšmės kraštutinės, priežastis yra MECHANINĖ variklio problema, ne pats purkštukas
— nedaryti išvadų vien iš šių skaičių be papildomo konteksto.

`vwts.ru` puslapyje yra ir specifinis **BJJ/BJK/BJL/BJM PDF** dokumentas
(`vwts.ru/engine/bjj_bjk_bjl_bjm_2_5_tdi_crafter_rus.pdf`), bet atsisiuntimas užrakintas
JavaScript mygtuku (paprastas `curl`/automatinis fetch negauna failo, tik nukreipimo
puslapį) — reikėtų atsisiųsti rankiniu būdu naršyklėje, jei norima šio konkretaus dokumento.

- [jazdw/vag-blocks Labels/](https://github.com/jazdw/vag-blocks/tree/master/Labels) —
  pavyzdinis `.lbl` (VCDS-style) žymėjimo failo formatas — jei norima, galima susikurti
  panašų failą Crafter's ECU (patikrinti, ar Service 21 grupė `0x52` (dar neiššifruota eilutė)
  neatitinka ECU dalies numerio, pagal kurį būtų galima ieškoti tikslesnio `.lbl` failo).
- [Ross-Tech Measuring Blocks](https://www.ross-tech.com/vag-com/m_blocks/) — bendra grupių
  numeracijos struktūra benzininiams varikliams — naudoti TIK kaip apytikslę orientaciją,
  NE diesel/PD atveju (patvirtinta, kad PD varikliams grupės NESUTAMPA su šiuo bendru sąrašu).

**Priėmimo kriterijus:** `decodeKwpBlockResponse()` implementuota ir `node --check` praeina;
Taisymas D2 (offline dekodavimas jau turimų duomenų) atliktas ir rezultatas trumpai
užfiksuotas (net jei rezultatas — „nė vienas žinomas tipas neatitiko", tai irgi vertingas
radinys).

---

## Bendri reikalavimai

- Po pakeitimų: `node --check android/www/obd.js` ir `node --check android/www/renogy.js`.
- Dalies A pakeitimus patikrinti REALIU testu su automobiliu (ne tik `node --check`) —
  patvirtinti, kad `runFullCollection()` vis dar surenka Service 21/22 duomenis be klaidų
  po Mode09/modulių skenavimo pašalinimo.
- Dalies B/C pakeitimus patikrinti REALIU testu su Renogy baterija per BLE.
- Versijos bump: v50 → **v51**, su changelog paminint: (1) OBD testavimo apimties
  susiaurinimas iki patvirtintai veikiančių komandų, (2) modulių skenavimo ATCS bug'o
  pataisymas, (3) Renogy temperatūros jutiklių 3/4 paaiškinimas/taisymas, (4) Renogy
  baterijos „Writing characteristic failed" pataisymas, (5) Service 21 KWP tipo-baito
  dekoderis (`decodeKwpBlockResponse`).
- **NEDARYTI** `git commit`/`push`/APK build be aiškaus vartotojo prašymo.
