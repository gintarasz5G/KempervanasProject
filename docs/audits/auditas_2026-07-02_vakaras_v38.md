# Vakaro auditas — 2026-07-02, v38 (v33.2) dienos darbų peržiūra

**Apimtis:** v33.0 → v33.2 (commit'ai a74c360, 1426158, 2d5f360 + cleanup), renogy.js (392 eil.,
pilna peržiūra), index.html Renogy/WiFi dalys, MainActivity bind logika, APK v38 (išpakuotas),
repo higiena. Tęsia: `projekto_auditas_2026-07-02_rinkos_palyginimas.md` (rytas) ir
`verifikacija_v36_renogy_2026-07-02.md` (2 iteracijos).

## Santrauka

Per dieną įgyvendintas **Etapo D 1 žingsnis** (Renogy BLE tab) ir **WiFi bind taisymas**.
Po dviejų verifikacijos iteracijų visos anksčiau rastos blokuojančios klaidos ištaisytos —
v38 grandinė (native plugin → capacitor.js → ble-plugin.js → renogy.js → UI) statiškai
vientisa. **Laukia gyvo testo.** Šis auditas rado 3 naujus vidutinius defektus renogy.js
parseryje (žr. R1–R3) — jie netrukdo diegti, bet paaiškės gyvame teste.

## ✅ Patvirtinta gerai (v38)

- WiFi: `checkAndBind` su SSID + privalomu `!VALIDATED` fallback, `onCapabilitiesChanged`,
  `<unknown ssid>` apdorojimas, Comms tab'e SSID/bind diagnostika (index.html ~1810).
- Renogy: BT enable UI (`requestEnable` + `startEnabledNotifications`), scan cooldown 10 s,
  Junctek MAC blacklist, dvi baterijos užklausos (0x1388 + 0x13B2), SOC = rem/cap,
  DCC alternator/PV offsetai pagal renogy-bt, CRC patikra, HEX diagnostikos logas,
  atviros GATT jungtys (ne connect/disconnect ciklas), visos UI kortelės susietos.
- Versijos: 38/38/„33.2"/kemperis_v38.apk/apk_url — sinchronizuota.

## 🟠 Nauji radiniai renogy.js (taisyti v33.3, netrukdo gyvam testui)

> **2026-07-02 vėlyvas vakaras — visi R1–R3 teiginiai PATIKRINTI prieš renogy-bt šaltinio
> kodą** (`BatteryClient.py`, `DCChargerClient.py` — raw iš GitHub). Patvirtinta:
> DCC50S laukų offsetai/masteliai renogy.js — **visi teisingi**; baterijos 5042 blokas —
> **teisingas**. Žemiau — patikslinti taisymai + naujas R6.

**R1. queryIndex lenktynės (parseFrame).** Atsakymo blokas nustatomas pagal `queryIndex`
būseną (apverstą rašymo metu), ne pagal patį atsakymą. Pametus/vėluojant vienam atsakymui
indeksas desinchronizuojasi → celės parsinamos kaip V/A ir atvirkščiai (tyliai, be klaidos).
**Taisymas (validuota):** bloką identifikuoti pagal `frame[2]` (byte_count): `0x44`=celės/temp
(0x1388, 34 žodžiai), `0x0C`=V/A/Ah (0x13B2, 6 žodžiai), `0x42`=DCC (0x0100, 33 žodžiai) —
reikšmės unikalios, būsenos nereikia.

**R2. `ren_temp` offset klaidingas — PATVIRTINTA.** Skaitoma `data[24]` (= registras 5012 —
**celės #12 įtampa**, 4 celių baterijoje 0). renogy-bt: `sensor_count` reg **5017**
(`data[34..35]`), temperatūros nuo **5018**, signed, **scale 0.1**. Užklausos keisti
NEREIKIA — 0x22 žodžių blokas (5000–5033) temperatūras jau dengia.
**Taisymas:** `ren_temp = s16(data[36], data[37]) / 10.0`.

**R3. DCC krovimo stadijų žemėlapis — PATVIRTINTA.** Tikrasis renogy-bt CHARGING_STATE:
`0=deactivated, 1=activated, 2=mppt, 3=equalizing, 4=boost, 5=floating, 6=current limiting,
8=alternator direct`. Kodo sąrašas `["Inactive","Normal","Boost",...]` — pasislinkęs
(mppt rodytų „Boost"). **Pastaba:** renogy-bt statusą skaito iš ATSKIRO bloko
(reg 288/0x0120, 3 žodžiai); renogy.js prielaida, kad status = 0x100 bloko paskutinis
žodis (`data[65]`), tikėtina teisinga (0x120 yra bloko riba), bet gyvame teste
sulyginti su Renogy app — jei nesutaps, daryti atskirą užklausą `[0xFF,0x03,0x01,0x20,0x00,0x03]`.

**R6 (NAUJAS — rastas šaltinio validacijos metu). Celių įtampų mastelis klaidingas.**
renogy-bt `parse_cell_volt_info`: **scale 0.1** (raw 36 = 3.6 V). renogy.js dalina iš 1000 —
celės rodys **0.036 V vietoj 3.6 V**. Offsetai (data[2],[4],[6],[8]) teisingi.
**Taisymas:** `/ 1000.0` → `/ 10.0` keturiose ren_cell_* eilutėse.

**R4. Smulkmenos:** `startReg` (254 eil.) — dead code; skenavimas nerikiuoja Renogy įrenginių
viršun; Android ≤11 Location paslaugos patikros prieš scan nėra (jei planšetė sena — tuščias
sąrašas be paaiškinimo).

**R5. CSV suderinamumas:** seni `localStorage` įrašai turi 17 laukų, naujas header — 24;
eksporte senas eilutes papildyti tuščiais laukais (užduoties reikalavimas liko neįgyvendintas).

## 🔴 Proceso radinys — worktree failų nukirtimas

Po v33.2 commit'o darbinėje kopijoje rasta **11 nukirstų failų** (MainActivity be WifiBridge
klasės, index.html be 233 eilučių, CLAUDE.md nutrūkęs viduryje žodžio, package.json su NULL
baitais) — git HEAD ir APK buvo geri, sugadinta tik disko kopija (panašu į nutrauktą
rašymą/sync). **Atstatyta iš HEAD** (`git show HEAD:file > file`). Liko vartotojui:
ištrinti `.git/index.lock` (0 B, nuo nulūžusio git proceso) ir paleisti `git reset`.
**Rekomendacija asistentui:** prieš KIEKVIENĄ commit — `git status` + `git diff --stat`
peržiūra; jei diff'e matosi vien ištrynimai failo gale — failas nukirstas, ne redaguotas.

## 🧹 Repo higiena

- **14 APK failų git istorijoje (44 MB)** — repo pūsis amžinai (git nepamiršta binarų net
  ištrynus). Siūlymas: naujus release'us kelti į **GitHub Releases** (apk_url jau vis tiek
  rodo į raw failą — Releases nuoroda veiktų taip pat), git'e laikyti tik naujausią;
  istorijos valymas (filter-repo) — atskiras sprendimas, nebūtinas dabar.
- Šaknies `package.json`/`node_modules` šiukšlės — išvalytos (c9bafb7, f3972ba) ✓.

## 📌 Statusas prieš rytinį auditą

| Tema | Būsena |
|---|---|
| **P0 sauga** (SMS siuntėjo auth, OTA/API/web_server, Apps Script token) | ❌ **Nepajudėjusi — tebėra pirmas prioritetas prieš naujas funkcijas** |
| Etapas D, 1 žingsnis (Renogy per planšetę) | ✅ Įgyvendinta (v38), laukia gyvo testo |
| WiFi bind klaida (v35 simptomas) | ✅ Taisymas v38, laukia gyvo testo |
| CLAUDE.md sinchronizacija | ✅ Atlikta ryte; Etapo D būsena atnaujinta šiame audite |
| CI (`esphome config` + SSE testas) | ❌ Neįgyvendinta |
| SSE gyva verifikacija (v32.2 fix) | ⏳ Vis dar laukia (žr. atmintį #1) |

## Gyvo testo checklist (v38)

1. WiFi: prisijungus prie AP (mobilieji ĮJUNGTI) — Toast „Pririšta prie Kemperio AP",
   Comms tab'e SSID + „Bound: taip", gyvi SSE duomenys.
2. Renogy: scan randa `BT-TH-xxxx` ir bateriją; priskyrus — duomenys ≤15 s.
3. Sulyginti su Renogy app (nuosekliai, per OFF jungiklį): V, A, SOC, celės —
   **ypač temp (R2) ir krovimo stadija (R3)** — tikėtini neatitikimai, fiksuoti HEX logą.
4. Variklis užvestas → alternator W > 0; saulė → pv W > 0.
5. OFF jungiklis → telefono Renogy app prisijungia per ≤5 s.
6. `[SSE UNMATCHED]` patikra terminale (sena užduotis iš v35 audito).

*Auditas: 2026-07-02 vakaras. Bazė: v38 / commit e59d86f (+ atstatytas worktree).*
