# Užduotis: naujas „Renogy" tab'as programėlėje (BLE tiesiai iš planšetės)

**Data:** 2026-07-02 · **Bazinė versija:** app v35 (versionName 32.3), firmware v32.3 (8d19a1d)
**Tipas:** tik APP (frontend + native). **Firmware, Apps Script ir Sheets CSV formato NELIESTI.**

---

## Tikslas

Programėlėje sukurti atskirą tab'ą **„Renogy"**, kuriame planšetė **tiesiogiai per savo Bluetooth**
(ne per ESP) prisijungia prie dviejų Renogy įrenginių ir rodo gyvus duomenis — taip, kaip daro
originali Renogy programėlė:

1. **Renogy Pro 12V 100Ah LiFePO4** (SKU `RBT12100LFP-BT`, integruotas BT, self-heating)
2. **Renogy DCC50S 12V 50A** (SKU `RBC50D1S-BT`) — jungiasi per komplekte esantį
   **BT-2 modulį** (skenuojant matysis kaip `BT-TH-xxxx`)

Tai Etapo D (CLAUDE.md) pirmas žingsnis: be firmware rizikos, protokolo kodas vėliau
tarnaus perkeliant į ESP.

## Apimtis

**Daroma:** naujas tab UI, BLE skenavimas/prisijungimas/reconnect, Modbus parseris JS,
kortelės, diagnostikos logas, nustatymai, lokalus CSV papildymas, versijos bump, APK build.

**NEdaroma:** firmware pakeitimai; SSE grandinės pakeitimai; Google Sheets CSV formato
pakeitimai; veikimas fone kai app uždarytas (v1 — tik kai app atidarytas); Junctek BLE
logikos lietimas (Junctek jungiasi prie ESP, ne planšetės — nesusiję).

---

## Techniniai reikalavimai

### 1. BLE sluoksnis (native)

- Naudoti `@capacitor-community/bluetooth-le` **^8.2.0** — suderinamumas PATIKRINTAS
  2026-07-02: naujausia 8.2.0, `peerDependencies: @capacitor/core >= 8.0.0`, projektas turi
  Capacitor 8.3.4 → tinka be triukų. **Capacitor core vis tiek NEatnaujinti** (regresijos
  rizika visam app). Diegimas: `npm install @capacitor-community/bluetooth-le` +
  `npx cap sync android` iš `android/`.
- Android permisijos — manifest'e BT permisijų **šiuo metu nėra jokių**, pridėti bloką pagal
  plugin'o README: `ACCESS_COARSE_LOCATION` (`maxSdkVersion=30`), `ACCESS_FINE_LOCATION`
  (`maxSdkVersion=30`), `BLUETOOTH` + `BLUETOOTH_ADMIN` (`maxSdkVersion=30`),
  `BLUETOOTH_SCAN` (su `usesPermissionFlags="neverForLocation"`), `BLUETOOTH_CONNECT`.
  ⚠️ Oficiali Android pastaba: su `neverForLocation` dalis BLE beacon'ų **filtruojama iš
  skenavimo rezultatų** — jei Renogy įrenginiai nesimato, išbandyti be šio flag'o
  (tada Android 12+ skenavimui reikės ir location permisijos). Runtime prašymas atidarius
  Renogy tab'ą pirmą kartą (ne app starte). `minSdk=24` — **prieš darbus užfiksuoti realią
  planšetės Android versiją**: jei 7–11, Location kelias (permisija + įjungta paslauga)
  privalomas.
- **BT adapterio būsenos tikrinimas:** atidarius Renogy tab'ą (ir prieš kiekvieną
  skenavimą/jungimąsi) tikrinti `BleClient.isEnabled()`. Jei BT išjungtas — kortelėse
  rodyti „Bluetooth išjungtas" su mygtuku **„Įjungti BT"** (`BleClient.requestEnable()` —
  Android parodo sisteminį dialogą; jei nepavyksta — `openBluetoothSettings()`).
  Registruoti `startEnabledNotifications()`: BT įjungus — automatiškai tęsti jungimąsi,
  BT išjungus veikimo metu — sustabdyti polling/reconnect ir rodyti būseną (ne klaidų
  laviną loge, vienas įrašas). App niekada nekrenta dėl išjungto BT.
- Skenavimas: **minkštas filtras** — Renogy prefiksus (`BT-TH-`, `RNGRBP`) iškelti sąrašo
  viršun, bet turėti jungiklį „Rodyti visus" (Pro baterijos advertising vardas nežinomas —
  kietas filtras galėtų paslėpti ją pačią). Junctek MAC `38:3B:26:79:DB:64` — **visada**
  išbrauktas (žr. pasalą 7). Vartotojas priskiria: „Baterija" / „DCC50S"; saugoti
  `localStorage` (MAC + tipas) — kitą kartą jungtis be skenavimo. Skenuoti tik priskyrimo
  režime (ne nuolat fone, ne esant aktyvioms jungtims).

### 2. Renogy protokolas (Modbus RTU per BLE GATT)

Šaltinis — **cyrils/renogy-bt** (https://github.com/cyrils/renogy-bt): registrų žemėlapius
imti iš `renogybt/BatteryClient.py` (baterija) ir `renogybt/DCChargerClient.py` (DCC50S),
**ne iš atminties**. Baziniai faktai:

- GATT: **write** — char `0000ffd1-...` (service `0xFFD0`); **notify** — char `0000fff1-...`
  (service `0xFFF0`). PATIKRINTA renogy-bt README loge 2026-07-02.
- Užklausa: `[device_id, 0x03, regHi, regLo, cntHi, cntLo, crcLo, crcHi]` (CRC16-Modbus, LE).
- `device_id` (iš renogy-bt README lentelės): **standalone baterija — `255`**;
  DCC50S (controller) — `255` arba `17`. Jei atsakymo nėra / duomenys „garbled" —
  bandyti hub/daisy-chain ID (baterija 48/49/50 arba 33/34/35, controller 96/97);
  atsakyme laukas `device_id` parodo tikrąjį — jį įsirašyti į `localStorage`.
- Atsakymas per notify: `[id, 0x03, byte_count, data..., crc]` — gali ateiti **keliomis
  notifikacijomis**, reikia defragmentacijos pagal `byte_count` (analogiška logika jau yra
  firmware Junctek parseryje — žr. `firmware/kempervanas.yaml` `on_notify`).
- DCC50S pagrindinis blokas: registrai nuo `0x0100`, 0x21 žodžių. Paruošta užklausa su CRC
  jau yra repo: `research/renogy-dcdc/renogy.yaml` (58–76 eil.):
  `[0xFF, 0x03, 0x01, 0x00, 0x00, 0x21, 0xD1, 0xCA]`.
- Baterijos blokas: ~5000 registrų sritis (celių įtampos, temperatūros, srovė, likutis Ah,
  talpa, SOC) — tikslius adresus ir mastelius imti iš `BatteryClient.py`.
- Ryšio modelis: **abi GATT jungtys laikomos atviros nuolat** (kol jungiklis ON);
  kaitaliojamos tik užklausos — polling kas **10 s**, įrenginiai apklausiami pakaitomis
  (ne lygiagrečiai). **NE** connect→read→disconnect kas ciklą — tai su „stale connection"
  pasala sukeltų nuolatinę jungimosi audrą. Auto-reconnect su backoff
  (2 s → 5 s → 15 s → 30 s max). CRC tikrinti; blogas kadras — atmesti ir loguoti,
  ne crash'inti.

### 3. UI — naujas tab „Renogy"

- Pridėti į tab select/swipe sąrašą (`SWIPE_TABS`, `index.html` ~1381) ir meniu.
- Kortelės (stilius kaip esamų kortelių; turinys — pagal originalios „DC Home" app pavyzdį,
  vartotojo ekrano nuotraukos 2026-07-02; laukų šaltiniai — renogy-bt klientų raktai):

  - **KROVIMO ŠALTINIAI (svarbiausia vartotojui):** du vizualiai lygiaverčiai blokai
    vienas šalia kito —
    „🚗 Variklis": `alternator_voltage` V / `alternator_current` A / `alternator_power` W;
    „☀️ Saulė": `pv_voltage` V / `pv_current` A / `pv_power` W.
    Aktyvų šaltinį paryškinti (galia >0), virš jų — krovimo būsena iš `charging_status`
    (NOT CHARGING / solar / alternator / boost / float …).
  - **DCC50S kortelė:** buitinė baterija `battery_voltage` V + `combined_charge_current` A;
    starterio pusė = alternator įtampa; `controller_temperature` (MPPT) ir
    `battery_temperature`; `power_generation_total` → kWh („Total kWh Generated");
    `battery_percentage`.
  - **Baterijos kortelė:** SOC (`remaining_charge/capacity`, gauge kaip originalioje),
    `capacity` Ah max / `remaining_charge` Ah dabar, `voltage` V, `current` A (+/− su
    spalva), 4 celių įtampos (`cell_voltage_0..3`), temperatūra min/max
    (`temperature_0..3`). Likusį laiką skaičiuoti patiems: `Ah / |A|` (kaip firmware daro
    Junctek atveju), kai A≈0 rodyti „--". Self-heating būsena — tik jei registras
    randamas renogy-bt (Pro serija; jei ne — praleisti, ne blokuoti).
  - **Ryšio būsena** kiekvienam įrenginiui: prijungta / jungiamasi / atjungta + duomenų
    šviežumo indikatorius (pattern kaip Freshness Bar).

  Pastaba: vartotojo DCC50S modelis **RBC50D1S-G4** (renogy-bt testuotas su G1 — protokolas
  tas pats, bet jei kuris laukas grąžins nesąmonę, žiūrėti raw logą, ne spėlioti).
- Skenavimo/priskyrimo UI: mygtukas „Ieškoti įrenginių", sąrašas, priskyrimas, „Pamiršti".
- **Pagrindinis ON/OFF jungiklis** tab'o viršuje („Renogy ryšys"): OFF — app iškart
  atsijungia nuo abiejų įrenginių ir **nedaro** auto-reconnect (kad vartotojas galėtų
  prisijungti originalia Renogy programėle iš telefono); ON — jungiasi vėl. Būsena
  saugoma `localStorage` ir išlieka po app restarto. **Pradinė būsena: ON** (bet jungiamasi
  tik kai yra priskirtų įrenginių — švarioje instaliacijoje jungiklis nieko nedaro).
  Kai OFF — kortelės rodo „Išjungta (galite jungtis Renogy app)", ne klaidą.
- Diagnostika: raw HEX paskutinio kadro + klaidų skaitliukai į esamą `sysLog`/terminalą
  (kaip `debugLog` pattern), įjungiama tik su debug jungikliu.

### 4. Integracija su esamu app

- **Lokalus CSV logas:** į `addLocalLogRow` gale pridėti Renogy stulpelius
  (`ren_soc, ren_v, ren_a, ren_temp, dcc_solar_w, dcc_alt_w, dcc_stage`) — **tik gale**,
  kad nesugriūtų esamas `downloadLocalCSV` stulpelių nuoseklumas. Kai Renogy neprijungta —
  tušti laukai. **Migracija:** localStorage jau saugomi senos struktūros įrašai — eksporto
  metu senas eilutes papildyti tuščiais laukais iki naujo stulpelių skaičiaus (arba
  versijuoti log failą), kad header ir eilutės visada sutaptų.
- **Kodo vieta:** visą Renogy logiką laikyti atskirame `www/renogy.js` (jungiamas per
  `<script src>`), ne pūsti `index.html` monolito — mažiau merge konfliktų ir lengvesnis
  būsimas perkėlimas į ESP etapą.
- **Nekeisti:** SSE grandinės, `_fetchSheetsNow`, cloud aliarmų, Sheets CSV (22 laukai —
  firmware kontraktas).
- Energijos taupymas: BLE polling vykdyti tik kai Renogy tab'as matomas ARBA įjungtas
  nustatymas „Renogy fone" (default OFF). Uždarius tab'ą su OFF — atsijungti.

### 5. Versijos ir build — taikinys **v36**

Dabartinė būsena (patikrinta 2026-07-02): `version.json "version": 35`,
`versionCode 35`, `versionName "32.3"`, APK `android/kemperis_v35.apk`.

Šio taisymo reikšmės — **visos keturios privalo sutapti**:

| Kas | Reikšmė |
|---|---|
| `version.json "version"` | **36** |
| `versionCode` (build.gradle) | **36** |
| `CURRENT_VERSION` (index.html, jei naudojamas) | **36** |
| APK failas | `android/kemperis_v36.apk` |
| `version.json "apk_url"` | `.../android/kemperis_v36.apk` |
| `versionName` | tekstinis, pvz. `"33"` (su notes apie Renogy BLE) |

- `version.json "notes"` — trumpas v36 changelog (Renogy tab, BLE, ON/OFF jungiklis).
- Build: `npx cap sync android` + Gradle release APK kaip įprasta.

---

## Žinomi apribojimai (dokumentuoti UI/README)

- BLE periferija priima **vieną** centralą: kol app prisijungęs, originali Renogy programėlė
  to įrenginio nematys (ir atvirkščiai — jei atidaryta Renogy app, mūsų app neprisijungs).
  Klaidos pranešime tai paminėti („Uždarykite Renogy app, jei prisijungti nepavyksta").
- Duomenys nepatenka į SMS statusą, Google Sheets ir ESP aliarmus — tai bus Etapo D
  antra dalis (per ESP), jei prireiks.
- Abu įrenginiai **oficialiai palaikomi renogy-bt** (patikrinta README Compatibility
  lentelėje 2026-07-02): „Renogy DC-DC Charger DCC50S — BT-2 ✅" ir
  „RBT12100LFP-BT (Pro Series, built-in BLE) ✅" — registrų žemėlapiai iš `BatteryClient.py` /
  `DCChargerClient.py` turėtų tikti be derinimo. Skenuojant vis tiek užfiksuoti tikslų
  baterijos advertising vardą (minkštas filtras tam ir yra) ir raw atsakymus diagnostikos
  loge, jei mastelis nesutaptų su Renogy app rodmenimis.

## Žinomos pasalos (privaloma atsižvelgti)

1. **Location paslauga:** Android 6–11 BLE skenavimas grąžina tuščią sąrašą, jei išjungtas
   sisteminis Location — tikrinti ir rodyti paaiškinimą (kaip su BT įjungimu).
2. **Scan throttling:** Android leidžia ~5 skenavimus / 30 s — skenavimo mygtukui dėti
   cooldown, nekartoti automatiškai cikle.
3. **Fonas / ekranas:** užgesus ekranui WebView taimeriai pristabdomi — polling sustos.
   V1 tai priimtina; šviežumo indikatorius privalo tai parodyti (ne rodyti senų reikšmių
   kaip gyvų).
4. **WiFi+BLE koegzistencija:** planšetė laiko ESP AP WiFi (2.4 GHz) + 2 BLE jungtis tuo
   pačiu radiju — jei po įjungimo padažnės SSE trūkinėjimai, mažinti polling dažnį
   (pvz., 20–30 s) ir tai padaryti konfigūruojamu nustatymu.
5. **Stale connection:** po negražaus atsijungimo Renogy įrenginys ~30–60 s nesireklamuoja —
   reconnect backoff tai dengia; UI rodyti „laukiama įrenginio", ne klaidą.
6. **DCC50S gali miegoti** be krovimo šaltinio (naktis, variklis išjungtas) — „nerandamas"
   tokiu metu yra norma; kortelėje rodyti „Neaktyvus (nėra krovimo?)" po N nesėkmių,
   ne raudoną klaidą.
7. **⚠️ Junctek šuntas skenavimo sąraše:** jis irgi reklamuoja FFF0 servisą. DRAUSTI jį
   priskirti (blacklist pagal MAC `38:3B:26:79:DB:64` ir vardo filtrai) — planšetei prisijungus
   prie Junctek, ESP netektų energijos duomenų, o BLE watchdog imtų perkrovinėti ESP.
8. **MTU:** plugin'as `requestMtu()` metodo NETURI (patikrinta API 2026-07-02) — MTU
   deramas automatiškai prisijungiant; pasitikrinti `BleClient.getMtu(deviceId)` ir
   **privalomai** turėti notify defragmentaciją pagal Modbus `byte_count` (nepriklausyti
   nuo MTU dydžio).

## Priėmimo kriterijai

1. `Renogy` tab'as atsidaro, swipe veikia, kiti tab'ai — be regresijų (ypač SSE ir Sheets).
2. Skenavimas randa abu įrenginius; priskyrimas išlieka po app restarto.
3. Gyvi duomenys iš baterijos IR DCC50S atsinaujina ≤15 s intervalu. Palyginimas su
   originalia Renogy programėle — **nuoseklus** (vienu metu prisijungęs gali būti tik
   vienas): užsifiksuoti mūsų app reikšmes → jungiklis OFF → prisijungti Renogy app →
   palyginti (±1 vnt. / mastelio paklaida leidžiama, kryptis ir dydžio eilė privalo sutapti).
3a. **Variklio ir saulės krovimo galios rodomos ATSKIRAI** (W kiekvienam šaltiniui) ir
    teisingai: važiuojant su varikliu rodo alternator W > 0, saulėtą dieną stovint —
    pv W > 0; abu blokai matomi vienu metu.
4. Išjungus Renogy įrenginį — kortelė rodo „atjungta", app nekrenta, reconnect veikia
   vėl įjungus.
5. Lokalus CSV turi naujus stulpelius gale; senų stulpelių tvarka nepakitusi.
6. Be Bluetooth permisijos app nekrenta — rodo paaiškinimą ir mygtuką suteikti.
6a. ON/OFF jungiklis: perjungus OFF, per ≤5 s originali Renogy programėlė telefone
    randa ir prisijungia prie įrenginio; perjungus ON — app vėl prisijungia be restarto.
    OFF būsena išlieka po app restarto (jokio auto-reconnect paleidus app iš naujo).
6b. Išjungtas BT planšetėje: tab'as rodo „Bluetooth išjungtas" + veikiantį „Įjungti BT"
    mygtuką; įjungus BT (per mygtuką ar sistemos nustatymus) ryšys atsistato be app
    restarto; išjungus BT veikimo metu — app nekrenta, loge vienas įrašas, ne ciklas.
7. Versijų identifikatoriai sinchronizuoti; APK vardas == `apk_url`.
8. Įgyvendinus — trumpas audito įrašas `docs/audits/` (kas padaryta, kas liko) ir git commit.

## Šaltiniai

- Protokolas: https://github.com/cyrils/renogy-bt (`renogybt/BatteryClient.py`,
  `renogybt/DCChargerClient.py`; Modbus dokumentacija —
  https://github.com/cyrils/renogy-bt/discussions/94)
- ESPHome portas (pravers Etapo D 2 žingsniui): https://github.com/mavenius/renogy-bt-esphome
- Vietinis tyrimas: `research/renogy-dcdc/renogy.yaml` (paruošta DCC50S užklausa su CRC)
- Plugin: https://github.com/capacitor-community/bluetooth-le (^8.2.0, Cap 8 suderinamas)
- Defragmentacijos pavyzdys: `firmware/kempervanas.yaml` Junctek `on_notify` parseris
