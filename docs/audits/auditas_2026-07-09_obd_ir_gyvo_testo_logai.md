# Auditas 2026-07-09 — OBD logai + gyvo testo telemetrija

**Auditorius:** Cowork Claude (tik analizė — kodo nekeičiau).
**Įvestis:** 4 OBD logai, 2 terminal/SSE logai, `kemperis_diena_2026-07-09.csv`, 2 app ekrano
nuotraukos (Energija / Renogy tab).
**Kontekstas:** app v52 (`version.json`), `obd.js` 779 eil., firmware v32.3.
**Susiję dokumentai:** `docs/uzduotis_obd_stabilumo_taisymai_v3.md` (v52 realaus testo užduotis),
`docs/uzduotis_obd2_elm327_tab.md`.

---

## Santrauka (TL;DR)

1. **OBD REGRESIJA (patikslinta 2026-07-09, žr. §4b/O6):** variklis VEIKĖ, anksčiau rodė VIN +
   apsukas, dabar — nieko. Priežastis kode: **v51 užkomentavo generic Mode 09 (VIN `0902`) ir
   Mode 01 polling (RPM `010C`)** „Surinkti VISKĄ" flow'e, palikdamas tik KWP Service 21 (reikia
   TP2.0 — ELM327 negali) ir UDS 22 (mirusi). Išjungti veikiantys, palikti neveikiantys → nerodo
   nieko. **Grąžinti VIN + Mode 01 polling.** (Ankstesnė „variklis išjungtas" hipotezė — atmesta.)
2. **OBD: „STOPPED" lavina** — patvirtinta kodo priežastis: `obd.js` normaliame režime tarp
   komandų daro **0 ms pauzę** (nelaukiama ELM327 `>` prompto). „Saugus režimas" (300 ms) tai
   išsprendžia, bet nėra įjungtas per sweep'ą.
3. **OBD: Service 21 (KWP measuring blocks) su ELM327 greičiausiai NIEKADA neveiks** —
   Crafter EDC16CP34 varikliškas ECU kalba **KWP2000-over-TP2.0** (VAG transporto sluoksnis),
   o `ATSP6` = paprastas ISO 15765-4 CAN. Stock ELM327 v1.5 TP2.0 nemoka. Realus kelias su šiuo
   adapteriu — generic **Mode 01/09** (ISO-TP), kuris dabar išjungtas („neveikia EDC16").
4. **SSE/Junctek: „D9 nerasta" ×57** — nekenksmingas, bet triukšmingas INFO logas (temp
   žymeklio nėra trumpuose fragmentuose; C0 įtampa parsinama OK, app rodo temp 19 °C).
5. **Google buferis: „HTTP POST klaida" „prilipęs"** — 6 SSE kadruose tas pats senas klaidos
   tekstas, nors buferis = 0 B ir signalas geras (−63 dBm, namų tinklas). Klaidos sensorius be
   laiko žymos / auto-valymo → klaidinantis „amžinas" aliarmas.
6. **DOKUMENTACIJOS DRIFTAS: CSV formatas nesutampa su CLAUDE.md** — realus Sheets failas turi
   **24 stulpelius** su Renogy/DCC laukais; CLAUDE.md §10 aprašo 22 laukus (`bed_temp, bed_hum,
   W, Ah, uptime_min`). Reikia suvienodinti dokumentaciją ir patikrinti firmware↔Apps
   Script↔app grandinę.

---

## 1. OBD2/CAN analizė

### 1.1 Faktai iš logų (visos 4 sesijos, ~13:51–14:11)

| Rodiklis | Rezultatas |
|---|---|
| ELM327 versija | `OBDII v1.5` (klonas), MAC `00:1D:A5:01:83:F2` |
| AT init | Visada OK: `ATZ, ATE0, ATL0, ATH0, ATSP6, ATSTFF, ATAT1, ATCFC1` |
| `ATRV` (borto įtampa) | **12.8–13.2 V** stabiliai (≈50 matavimų) |
| Teigiami OBD atsakymai (`41…`/`61…`/`62…`/`7Ex`) | **0 (nulis) per visus logus** |
| Bandyti servisai | `10 03`, `18 02 FF 00`, `21 01`, `22 F190…`, `21 01–21 FF`, `10 01` |
| Bandytas generic Mode 01/09 (`0100`, `010C`, `0902`) | **NE** (šiuose runuose nepaleista) |

### 1.2 Radinys O1 (P0, ROOT CAUSE) — variklis išjungtas testo metu

`ATRV` = 12.8–13.2 V = pastovi/įkraunama akumuliatoriaus įtampa **be generatoriaus**. Veikiantis
dyzelis rodytų ~14 V. Su išjungtu varikliu Crafter varikliškas ECU (0x7E0) **miega** ir
neatsako į jokį diagnostinį servisą — tai vienas paaiškina 100 % `NO DATA`.

Tai **nėra naujas defektas** — `uzduotis_obd_stabilumo_taisymai_v3.md` jau įspėja: *„vis dar
NETURIME nė vieno Service21 grupės atsakymo, užfiksuoto kol RPM > 0"*. 2026-07-09 logai kartoja tą
pačią klaidą.

**Veiksmas:** pakartoti testą **su UŽVESTU (dirbančiu) varikliu**. Tiksliai pagal v3 užduoties
Žingsnį 1: pirma `tagState('~1500 RPM')`, tada IŠKART „Surinkti VISKĄ", KOL variklis dirba.
Be šito visi kiti OBD radiniai lieka neįrodomi.

### 1.3 Radinys O2 (P0, METODAS) — nebandomas generic Mode 01/09

Šiuose runuose iškart pereinama prie manufacturer-specific KWP (21) / UDS (22). Bazinio ryšio
patikros (`0100` „kokie PID palaikomi", `010C` RPM, `0902` VIN) **nė karto nepaleista**. Be nė
vieno teigiamo kadro neįmanoma atskirti „miega ECU" nuo „blogas protokolas" nuo „PID neteisingas".

**Veiksmas:** su dirbančiu varikliu PIRMA paleisti `0100` → jei atsako `41 00 …`, ryšys yra ir
problema tik manufacturer PID'uose. Jei ir `0100` `NO DATA` — problema fizinė (uždegimas/gateway/
protokolas), ne PID žemėlapyje. (Kodas turi `runIdentity()` su `0902/0904/090A` — jį verta
paleisti pirmiau nei block sweep.)

### 1.4 Radinys O3 (P0, PROTOKOLAS) — Service 21 su ELM327 greičiausiai neįmanomas

Patikrinta: Crafter 2.5 TDI (2E) = **Bosch EDC16CP34**, diagnozuojamas VCDS/Ross-Tech. VAG
measuring blocks (Service 21) šiam ECU eina per **KWP2000-over-TP2.0** — VAG specifinį CAN
transporto sluoksnį su dinaminiais kanalais. `ATSP6` (ISO 15765-4 CAN 11-bit 500k) TP2.0
neatidaro; **stock ELM327 v1.5 TP2.0 nepalaiko**. Todėl `21 01–21 FF` sweep'as su šiuo adapteriu
gali niekada negrąžinti `61…`, net ir su dirbančiu varikliu.

`decodeKwpField()` lentelė kilusi iš `jazdw/vag-blocks` — teisingas *dekodavimas VAG blokams*, bet
niekada nepasieks duomenų, jei transporto sluoksnis (TP2.0) neatidaromas.

**Veiksmas / sprendimo kryptys (Android asistentui apsvarstyti):**
- Realų dyzelio duomenų srautą (RPM, temp, greitis) imti per **generic Mode 01** (ISO-TP) — tai
  vienintelis patikimas kelias su ELM327.
- Jei būtinai reikia VAG measuring blocks (DPF suodžiai, purkštukų korekcijos) — reikia
  **TP2.0-gebančio** sprendimo (pvz. dedikuotas VAG-CAN adapteris arba ESP32 su TP2.0 stack'u),
  ne stock ELM327.
- Prieš tolesnį block-sweep tobulinimą: **eksperimentu patvirtinti**, ar `0100`/`010C` iš viso
  atsako su dirbančiu varikliu. Tai atsako, ar verta toliau investuoti į 21-sweep.

### 1.5 Radinys O4 (P1, FLOW BUG) — „STOPPED" lavina = 0 ms pauzė

Kode (`obd.js`, `onDataLine`, ~183 eil.):

```js
const delay = STATE.safeMode ? 300 : 0;   // <-- normaliame režime 0 ms
setTimeout(pumpQueue, delay);
```

Gavus PIRMĄ eilutę iš ELM327 iškart siunčiama kita komanda — **nelaukiama `>` prompto**. Kai
atsakymas grįžta greitai (Δ 30–500 ms), nauja komanda pertraukia ELM327 vidinį OBD užklausos
ciklą → adapteris grąžina **`STOPPED`**. Kai laukiama iki pilno timeout (Δ~1100 ms), gaunamas
tvarkingas `NO DATA`. „Saugus režimas" (300 ms, `safeMode`) tai beveik pašalina — logas
`1b78893e` (🐢 saugus režimas ĮJUNGTAS) rodo švarius Δ~1100–1200 ms `NO DATA` be `STOPPED`.

Numatytoji `safeMode` reikšmė = `localStorage 'obd_safe_mode'==='true'` → **default false**.

**Veiksmas:**
- Sweep/skenavimo metu **priverstinai** įjungti pauzę-po-prompto (arba `safeMode`), nepriklausomai
  nuo localStorage.
- Geriau: laukti tikro `>` prompto simbolio (ne fiksuoto delay) prieš `pumpQueue()` — tai
  ELM327-native „komanda baigta" signalas. `STOPPED` rezultatai yra beverčiai (užklausa
  nutraukta), tad jie tik teršia logą ir DID/sweep išvadas.

### 1.6 Radinys O5 (P2, STRATEGIJA) — aklas 21 01–21 FF + didelis DID sąrašas per anksti

`runBlockSweepInternal()` cikliškai siunčia visus 255 blokus, `runDidScanInternal()` — ~24 DID.
Tai „šratinė" prieš patvirtinant nė vieną teigiamą kadrą → 3–4 min triukšmo ir galimas ECU
„užvertimas". Vertėtų gate'inti manufacturer sweep'ą už sėkmingo `0100`/`62…` atsakymo.

---

## 2. Terminal / SSE logo analizė

### 2.1 Radinys S1 (P2) — „D9 nerasta" ×57 (triukšmas, ne klaida)

`[I][junc_ble:1305]: D9 nerasta (buf=N b)` pasikartoja 57 k. Junctek paketas defragmentuojamas;
trumpuose (5–7 B) fragmentuose D9 (temp) žymeklio nėra — tai **normalu**. C0 (įtampa) parsinama
`14.42V OK`, app rodo Akum temp 19 °C, t. y. BLE veikia. Problema tik **INFO-lygio spam'as**.

**Veiksmas:** „D9 nerasta" nuleisti į DEBUG/VERBOSE arba logint tik jei D9 nerastas *po*
pilno paketo surinkimo (ne kiekvienam fragmentui).

### 2.2 Radinys S2 (P1) — „prilipusi" HTTP POST klaida

`text_sensor_sistema_paskutine_klaida = "HTTP POST klaida (rysys/serveris)"` matoma 6 SSE
kadruose, tuo pačiu `google_buferis = 0 B / 10000` ir signalas geras (−63 dBm, „Namu tinklas",
operatorius Bite). Buferis 0 B rodo, kad naujų duomenų nekaupiama / išsiųsta; klaidos tekstas
greičiausiai **senas retained state**, kartojamas kiekvienam SSE snapshot'ui (ne 6 naujos
klaidos).

**Veiksmas:** prie „paskutinė klaida" pridėti **laiko žymą** ir/arba **auto-valymą** po sėkmingo
POST (HTTP 200/302), kad nerodytų „amžino" aliarmo. Verta patikrinti, ar POST tikrai pavyksta
(logai turi 0 sėkmės eilučių, bet tai gali būti dėl DEBUG lygio — reikia matyti HTTP status kodą).

### 2.3 Pastaba S3 (INFO, SAUGA) — paslaptys SSE sraute

SSE debug sraute atviru tekstu matomi `txt_app_script_id=AKfycbw…` ir `txt_admin_number=+3706…`.
Tai atitinka jau žinomą **P0 saugumo skolą** (web_server be auth, žr.
`projekto_auditas_2026-07-02_rinkos_palyginimas.md`, Etapas A). Nieko naujo, bet užfiksuota.

---

## 3. CSV + Renogy ekranų kryžminė patikra

### 3.1 Radinys C1 (P1) — CSV formatas nesutampa su CLAUDE.md

Realus `kemperis_diena_2026-07-09.csv` antraštė (**24 stulpeliai**):

```
Laikas, (tuščias), Platuma, Ilguma, Aukštis, Greitis, Kryptis, SOC, Įtampa, Srovė,
Vanduo%, TDS, Dujos%, Temp, Drėgmė, Slėgis, CO2,
Ren SOC, Ren V, Ren A, Ren Temp, DCC Saulė W, DCC Gen W, DCC Stadija
```

CLAUDE.md §10 dokumentuoja **22 laukus**, kur 18–22 = `bed_temp, bed_hum, W, Ah, uptime_min`.
Realybėje 18–24 = Renogy/DCC laukai. **Formatas pasikeitė, dokumentacija — ne.**

Papildomas klausimas: pagal CLAUDE.md Renogy duomenys skaitomi **app'e per planšetės BLE**
(firmware Renogy neturi). Vis dėlto Sheet turi `Ren *` stulpelius — reikia patvirtinti **kas
juos rašo** (app tiesiai į Sheets? ar firmware gauna Renogy per tiltą?) ir kad
firmware CSV builder ↔ `GoogleSheetsScript.js` stulpelių žemėlapis ↔ app `_fetchSheetsNow` visi
sutampa su nauju 24-stulpelių formatu.

**Veiksmas:** atnaujinti CLAUDE.md §10 pagal faktinį 24-stulpelių formatą; patvirtinti trijų
grandžių (firmware / Apps Script / app) suderinamumą; dokumentuoti Renogy→Sheets duomenų kelią.

### 3.2 Radinys C2 (P2) — įtartinos „po-boot" eilutės CSV pradžioje

Pirmos 5 eilutės (04:18–04:23) identiškos: SOC 100, V 14.42, Temp 27.4, Drėgmė 41.3, Slėgis 995.6,
Vanduo 65. Nuo 04:24 **vienu metu** šoka daug nesusijusių dydžių: SOC 100→95, V 14.42→13.26,
Temp 27.4→**16.1**, Drėgmė 41.3→68.6, Slėgis 995.6→987.0, CO2 474→338, Vanduo 65→36. 11 °C temp
kritimas per minutę nėra realus — tai atrodo kaip **perkrovimas / pirmų kadrų stale reikšmės**
(BME680 self-heat po boot + SOC/„stabilių reikšmių" persiskaičiavimas). Šios eilutės teršia dienos
duomenis.

**Veiksmas:** apsvarstyti „warmup" langą (praleisti pirmus N s po boot prieš logint) arba
pažymėti tokias eilutes. Žemos svarbos.

### 3.3 Ekranų patikra (OK)

Energija tab: SOC 100 %, 14.32 V, 13.20 A, 189 W, Ah likutis 109.0, Akum temp 19.0 °C — sutampa su
SSE (`akum_itampa=14.42`, `akum_srove` teigiama įkrovimo metu) ir Junctek C0 parse. Renogy tab
(Pro BLE 100 %, celės 3.6 V ×4, DCC50S MPPT) atitinka CSV `Ren SOC 100 / Ren V 14.40 / Stadija
MPPT`. **Neatitikimų nerasta.**

---

## 4. Prioritetinis veiksmų sąrašas (Android asistentui)

> Cowork Claude kodo nekeičia. Žemiau — ką turi padaryti Android asistentas / vartotojas.

**P0 — pirmiausia (be jų OBD neįrodomas):**
- [ ] **O1:** Pakartoti OBD testą su UŽVESTU varikliu (RPM > 0), pagal v3 Žingsnį 1
      (`tagState` → iškart „Surinkti VISKĄ"). Vartotojo veiksmas.
- [ ] **O2:** Su dirbančiu varikliu PIRMA paleisti `0100` (ir `runIdentity` `0902`); užfiksuoti,
      ar grįžta `41 00…`/`49…`. Tai lemia visą tolesnę strategiją.
- [ ] **O3:** Pagal O2 rezultatą nuspręsti: jei generic Mode 01 atsako, o `21xx` — ne, tai
      patvirtina TP2.0 limitą → nenaudoti stock ELM327 measuring blokams; realius duomenis imti
      per Mode 01.

**P1:**
- [ ] **O4:** Skenavimo metu priverstinai laukti ELM327 `>` prompto (arba `safeMode`) — pašalinti
      `STOPPED` lavina; nepasitikėti localStorage default.
- [ ] **S2:** „paskutinė klaida" sensoriui pridėti laiko žymą + auto-valymą po HTTP 200/302.
- [ ] **C1:** Suvienodinti CLAUDE.md §10 su faktiniu 24-stulpelių CSV; patvirtinti firmware ↔
      Apps Script ↔ app suderinamumą; dokumentuoti Renogy→Sheets kelią.

**P2:**
- [ ] **O5:** Gate'inti 21-sweep/DID skenavimą už sėkmingo `0100`/`62…` atsakymo.
- [ ] **S1:** „D9 nerasta" → DEBUG lygis arba logint tik po pilno paketo.
- [ ] **C2:** „Warmup" langas CSV pradžioje (praleisti stale po-boot eilutes).

---

## 4b. PAPILDYMAS (2026-07-09, po vartotojo pastabų) — variklis VEIKĖ

Vartotojas patvirtino: **variklis testų metu veikė**, ir anksčiau bent VIN + apsukos rodydavo,
o dabar nieko. Tai keičia diagnozę iš „variklis išjungtas" (O1) į **regresiją**. (`ATRV` ~13 V su
veikiančiu varikliu vis tiek keista — vertėtų patikrinti, iš kur ELM327 ima įtampą, bet tai
antraeilis klausimas.)

### Radinys O6 (P0, REGRESIJA — TIKROJI PRIEŽASTIS) — v51 išjungė VIN + RPM servisus

`obd.js` „Surinkti VISKĄ" flow (745–762 eil.) v51 **užkomentavo būtent tuos servisus, kurie
realiai veikė**:

```js
// await runIdentityInternal(); // Isjungta v51 (neveikia EDC16)   <- VIN (Mode 09, 0902)
await runProtocolProbeInternal();   // 21 01  (KWP – reikia TP2.0, ELM327 negali)
await runDidScanInternal();         // 22 …   (UDS – patvirtinta NEVEIKIA)
// await runModuleScanInternal();   // Isjungta v51
...
// startPolling(); // Isjungta v51                                 <- RPM (Mode 01, 010C)
```

Taip pat `startPolling()` (Mode 01 polling, tarp jų **RPM 010C**) išjungtas 257, 761 eil. ir
viduje. `startPolling()` viduje (293 eil.) siunčia `01<pid>` ir 0x0C atveju netgi automatiškai
žymi „Variklis veikia, RPM=…" — t. y. **tai buvo tas kelias, kuris rodė apsukas.**

**Kas įvyko:** kažkas per v51 padarė išvadą „Mode 01/09 neveikia EDC16" (greičiausiai remdamasis
testu su išjungtu varikliu, kaip 07-09 logai) ir juos išjungė, pakeisdamas KWP Service 21 +
UDS 22. Bet Service 21 EDC16CP34 eina per **TP2.0**, kurio ELM327 neatidaro, o UDS 22 patvirtinta
mirusi → dabar **nerodo nieko**. Trumpai: **išjungti veikiantys (generic Mode 01/09), palikti
neveikiantys (KWP21/UDS22).**

**Veiksmas (Android asistentui, P0):**
- Grąžinti `runIdentityInternal()` (VIN `0902`, + `0904/090A`) į „Surinkti VISKĄ" flow.
- Grąžinti Mode 01 polling (`startPolling()`), bent RPM `010C`, temp `0105`, greitis `010D`.
- KWP21/UDS22 palikti kaip *papildomą* bandymą, NE vietoje generic Mode 01/09.
- Pirmiausia su veikiančiu varikliu paleisti `0100` — patvirtinti, kad generic PID atsako.

### Radinys U1 (P1, UI) — Renogy baterijos temp kortelės 3 ir 4 visada tuščios

`index.html` (496–501 eil.) turi **4 fiksuotas** temp korteles, bindinamas `ren_temp`,
`ren_temp_1`, `ren_temp_2`, `ren_temp_3` (2214–2217 eil.). Bet `renogy.js` **visada ištrina**
`ren_temp_2` ir `ren_temp_3` (320–321 eil.: „Reg 5020/5021 dažniausiai 0"), o `ren_temp_1` —
tik jei jutiklių ≥2. RBT12100LFP-BT realiai grąžina **2 temp jutiklius**, tad 3 ir 4 kortelės
niekada neužsipildo (rodo `0`/`---`).

**Veiksmas:** pašalinti 3 ir 4 temp korteles (ir 2-ą padaryti sąlyginę pagal `tempCount`).
Tik `index.html` — firmware neliesti.

### Radinys U2 (P2, UI) — „Likęs veikimas 0 min" kraunant

`junc_time_remaining` (Junctek D6) kraunant = 0 (Junctek rodo tik iškrovimo laiką). App tuo pat
metu žino būseną: `bat-status` = „⚡ KRAUNASI" kai `junc_a>0.1` (2083 eil.). Bet „Likęs veikimas"
kortelė (1960 eil.) rodo žalią `0 min` — klaidina.

**Veiksmas:** `junc_time_val` formatteryje jei kraunama (`junc_a>0`) rodyti „—" arba „kraunasi",
ne „0 min". Kosmetika, `index.html`.

### Radinys U3 (P1, SSE mapping) — ESP temp yra sraute, bet apatinėje juostoje nerodo

SSE srautas turi `sensor_sistema_esp_temperat_ra=43` — **duomuo YRA** (43 °C), firmware jutiklis
(`sys_esp_temp`, `kempervanas.yaml` 1101 eil.) veikia. App mapping'e (`index.html` 922–924 eil.)
yra 3 slug variantai (`sistema___esp_temperat_ra` ir kt.) → `sys_esp_temp`, o `diag-esp-temp`
juosta atnaujinama 1965 eil. Kadangi vis tiek nerodo — **įtartinas slug neatitikimas**: realus
ESPHome `object_id` gali nesutapti su nė vienu iš 3 variantų (žr. `[[kemperis-sse-objectid-faktas]]`:
SSE id = slugifikuotas LT pavadinimas). 

**Veiksmas:** Android asistentas turi užlogint **tikslų raw SSE raktą** ESP temperatūrai ir
palyginti su mapping lentele (922–924 eil.); jei nesutampa — pridėti variantą arba pataisyti
binding'ą. Firmware pavadinimo NEKEISTI (kitaip nukris kiti slug'ai).

---

## 5. Verifikacijos pastabos (auditoriaus)

- Visi OBD radiniai patikrinti prieš `android/…/assets/public/obd.js` (v52): `safeMode` default
  (23 eil.), 0 ms pump (183 eil.), `ATSP6` hardcoded (249 eil.), Mode 01 polling išjungtas
  („neveikia EDC16", 256 eil.), block sweep 1–255 (669+ eil.).
- „Nulis teigiamų atsakymų" patvirtinta `grep`'u per visus 4 OBD logus (tik `ATZ→OBDII v1.5` ir
  `ATRV→…V` atsako).
- Crafter EDC16CP34 / TP2.0 protokolas patikrintas web paieška (Ross-Tech Wiki, MHH Auto).
- CSV 24-stulpelių driftas patvirtintas tiesiogiai lyginant su CLAUDE.md §10.
- Ekranų reikšmės sutikrintos su SSE ir CSV — sutampa.
