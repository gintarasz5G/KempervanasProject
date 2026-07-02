# v36 Renogy BLE verifikacija — 2026-07-02

**Tikrinta:** commit `a74c360` (+ `3289a1f` cleanup), `kemperis_v36.apk` (išpakuotas), prieš
`docs/uzduotis_renogy_ble_tab.md` reikalavimus.
**Verdiktas:** ❌ **Funkcija APK'e NEVEIKS.** UI ir failai sukurti, versijos sinchronizuotos,
bet yra 3 blokuojančios klaidos (iki BLE net neprieinama) ir 2 protokolo klaidos.
Reikalingas v37 taisymas.

## 🔴 Blokuojančios klaidos

### B1. Native plugin'o APK'e NĖRA
`npm install` atliktas **repo šaknyje** (sukurtas naujas `/package.json` su bluetooth-le),
o Capacitor projektas yra `android/` (`android/package.json` plugin'o neturi).
Įrodymai: `android/android/capacitor.settings.gradle` be bluetooth-le įrašo;
APK `assets/capacitor.plugins.json` turi tik text-to-speech; `BluetoothLe` klasių
DEX'e nėra. Bet koks BLE kvietimas runtime grąžins „plugin not implemented".
**Taisymas:** ištrinti šaknies `package.json`/`package-lock.json`/`node_modules`;
`cd android && npm install @capacitor-community/bluetooth-le && npx cap sync android`;
patikrinti, kad `capacitor.settings.gradle` ir `capacitor.plugins.json` turi BluetoothLe;
perbuild'inti.

### B2. `renogy.js` neįtrauktas į index.html
`www/renogy.js` failas sukurtas, bet `index.html` turi tik du `<script>` tagus
(leaflet + inline) — **`<script src="renogy.js"></script>` nėra**. `RenogyBLE` objektas
neegzistuos; „Ieškoti įrenginių" mygtukas mes `ReferenceError`.
**Taisymas:** pridėti script tagą prieš inline skriptą (arba po jo, bet prieš naudojimą).

### B3. Neteisinga plugin'o API prieiga
`renogy.js` visur: `const { BleClient } = Capacitor.Plugins.BluetoothLe;` —
**`BleClient` nėra native proxy savybė.** `BleClient` yra npm paketo JS wrapper'is
(ES modulis); be bundlerio jis nepasiekiamas, o `Capacitor.Plugins.BluetoothLe` yra
žemo lygio proxy su kitokia semantika (skenavimo rezultatai/notifikacijos ateina per
`addListener` eventus, ne per callback parametrus). Destructuring grąžins `undefined`.
**Taisymas (paprasčiausias kelias be bundlerio):** nukopijuoti
`node_modules/@capacitor-community/bluetooth-le/dist/plugin.js` į `www/` ir įtraukti
`<script src="ble-plugin.js">` PRIEŠ renogy.js — jis sukuria globalų
(patikrinti tikslų pavadinimą dist faile, tikėtina `capacitorCommunityBluetoothLe`,
su `.BleClient` eksportu); tada `const { BleClient } = window.<globalas>`.
Alternatyva — perrašyti į raw proxy API su addListener'iais, bet tai daugiau darbo.

## 🟠 Protokolo klaidos (duomenys būtų šiukšlės)

### P1. Baterijos registrų blokas neteisingas
Užklausa `0x1388 (5000), count 0x22` apima celių skaičių/įtampas/temperatūras
(5000–5033), bet **SOC/įtampos/srovės ten NĖRA** — jie ~`0x13B2 (5042+)`:
current 5042, voltage 5043, remaining Ah 5044–45, capacity 5046–47 (0.001 Ah).
Dabartinis parse (`ren_soc=data[0]`, `ren_v=data[2]`, celės nuo `data[14]`) —
išgalvotas: data[0–1] realiai yra cell_count, celės nuo data[2].
**Taisymas:** dvi užklausos pakaitomis (5000 blokas celėms/temp + 5042 blokas
V/A/Ah/talpai); SOC skaičiuoti `remaining/capacity×100`; offsetus imti iš
renogy-bt `BatteryClient.py`, ne iš atminties. `ren_temp` mastelis — 0.1 (dabar be /10).

### P2. DCC50S alternator offsetai prieštarauja renogy-bt
Kodas ima alternator iš `data[46..51]` (koment. 0x117), bet renogy-bt DCChargerClient
tvarka: 0x101 batt V, 0x102 combined A, 0x103 temps, **0x104–0x106 alternator V/A/W**,
0x107–0x109 PV V/A/W. T. y. alternator turi būti `data[8..13]`. PV offsetai (14–19)
teisingi. `dcc_charge_a` mastelis /100, ne /10 (README max_current 17.02).
Krovimo stadijos `data[65]` (0x120) ir kodų sąrašas — pasitikrinti prieš
`DCChargerClient.py` CHARGING_STATE žemėlapį.
**Taisymas:** perimti offsetus/mastelius tiesiai iš `DCChargerClient.py`; jei gyvi
duomenys nesutampa su Renogy app — raw HEX logas (jis numatytas užduotyje, bet
neįgyvendintas).

## 🟡 Trūkstami užduoties reikalavimai

1. **BT įjungimo UI nėra** (`requestEnable`/`openBluetoothSettings`/
   `startEnabledNotifications`) — tik tylus debug log. Kriterijus 6b neįvykdytas.
2. **Runtime permisijų srauto nėra** (initialize dalį dengia, bet Android ≤11 scan be
   Location tyliai tuščias — nėra paaiškinimo UI).
3. **Reconnect backoff nėra** (fiksuotas 10 s per poll — toleruotina, bet be
   „laukiama įrenginio"/„Neaktyvus (nėra krovimo?)" UI būsenų).
4. **Scan cooldown nėra** (Android throttling — 5/30 s).
5. Manifest: `ACCESS_FINE/COARSE_LOCATION` be `maxSdkVersion="30"`; pridėtas
   nereikalingas `BLUETOOTH_ADVERTISE`.
6. Diagnostikos raw HEX logo nėra (P1/P2 derinimui būtinas).

## ✅ Kas padaryta gerai

Tab UI + SWIPE_TABS, ON/OFF jungiklis su localStorage (default ON, jungiasi tik su
priskirtais įrenginiais), Junctek MAC blacklist skenavime, CSV stulpeliai pridėti
**gale** (sena tvarka nepažeista), CRC16 implementacija teisinga, Modbus defragmentacija
pagal byte_count yra, versijų grandinė (36/36/„33.0"/kemperis_v36.apk/apk_url) sutvarkyta,
manifest BT permisijų blokas iš esmės yra.

## Pastaba dėl Sheets

`uploadLocalDataToGoogle` dabar siųs 24 laukų eilutes — Sheets lape stulpeliai nuo #18
susikirs su firmware formato bed_temp/bed_hum/W/Ah/uptime. Tai buvo ir anksčiau
(app siuntė 17 laukų vs firmware 22), bet dabar kolizija turininga. Spręsti atskirai
(pvz., app upload'ą žymėti ar lygiuoti į firmware 22 laukus prieš ren_ priedus).

## v37 užduotis (asistentui)

1. B1→B2→B3 taisymai (be jų niekas neveikia) — po to smoke-test: tab'e paspaudus
   „Ieškoti įrenginių" sąraše matosi įrenginiai.
2. P1+P2: registrai/masteliai iš renogy-bt kodo + raw HEX diagnostikos logas.
3. Trūkstami 1–6 (bent BT enable UI ir scan cooldown).
4. Šaknies package.json šiukšlių pašalinimas.
5. Versija v37 pagal tą pačią grandinę; gyvas testas pagal užduoties priėmimo
   kriterijus 2, 3, 3a, 4, 6a, 6b.
