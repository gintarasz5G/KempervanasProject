# Užduotis: naujas „OBD" tab'as programėlėje (ELM327 Bluetooth Classic tiesiai iš planšetės)

**Data:** 2026-07-06 · **Bazinė versija:** app versionCode 46 (versionName "34.0", APK `kemperis_v46.apk`)
**Tipas:** tik APP (frontend + native). **Firmware, Apps Script ir Sheets CSV formato NELIESTI.**

---

## Tikslas

Programėlėje sukurti atskirą tab'ą **„OBD"**, kuriame planšetė **tiesiogiai per savo Bluetooth**
(ne per ESP) prisijungia prie ELM327 adapterio, įjungto į VW Crafter (2008, 2.5 TDI) diagnostikos
lizdą, ir rodo **kiek įmanoma daugiau realių automobilio duomenų**, ne tik variklio. Analogiška
architektūra kaip `renogy.js` (žr. `docs/uzduotis_renogy_ble_tab.md`), bet **transportas
kitoks** — žr. §1.

**Svarbu iš karto:** ši užduotis (app tab'as + ELM327) realiai apima tik **variklio ECU
duomenis** (§3) — tai vienintelis modulis, kurį ELM327 gali pasiekti be papildomo tyrimo (žr.
Fazė 2 apačioje dėl priežasties). Kitų automobilio modulių (ABS/ESP, kėbulo/SAM, transmisijos,
prietaisų skydelio) duomenys **techniškai pasiekiami per tą patį OBD lizdą**, bet reikalauja
kitokio metodo (pasyvaus CAN sniffing su jau turimu ESP32 rig'u, ne ELM327 app'e) — tai atskira,
vėlesnė tyrimo užduotis, aprašyta §8 apačioje. Šis dokumentas dabar aprėpia abu kelius, kad
būtų aišku, kas daroma dabar (app, variklis) ir kas — vėliau (sniffing, visas automobilis).

## Patvirtinti faktai (testuota 2026-07-06 su OBD Auto Doctor Premium 8.3.1)

- Adapteris: **„OBDII v1.5" / „OBDII to RS232 Interpreter"** — pigus ELM327 klonas.
- **Bluetooth Classic (SPP)** — patvirtinta: įrenginys matomas ir susietas (PIN) per
  telefono SISTEMINIUS Bluetooth nustatymus, ne tik per app'o vidinį skenavimą.
  **`@capacitor-community/bluetooth-le` (naudojamas Renogy tab'e) čia NETINKA** — tai grynai
  BLE/GATT biblioteka, nekalba su Classic SPP įrenginiais. Reikia naujo native sluoksnio (§2).
- Protokolas: **ISO 15765-4 (CAN 11-bit ID, 500 kbps)** — patvirtina Gemini spėjimą.
- ECU atsako adresu **`0x7E8`** (užklausa į `0x7E0` arba broadcast `0x7DF`) — sutampa su
  `research/obd2/ESPobd/src/main.cpp` naudojama schema.
- VIN `WV1ZZZ2EZ86028774` — pozicija 10 = `8` → **2008 m.**, patvirtina modelio metus.
- `ATRV` (baterijos įtampa) veikia — 13.8V (variklis suktas) / 12.7V (variklis išjungtas).
  Reiškia adapteris tinkamai reaguoja į standartines AT komandas.
- Calibration ID `074906032AT` — „074" yra žinoma VAG dalies numerio serija naudojama
  5-cilindrų TDI varikliams (T4/T5/LT/Crafter) — patvirtina, kad variklio ECU yra Bosch EDC16.

### Platformos tyrimas (2026-07-06, žr. §3 dėl PID pasekmių)

Crafter (2E, 2006–2016) yra **hibridas su Mercedes Sprinter (W906)** — kėbulą/elektros
architektūrą gamino DaimlerChrysler, bet **variklį VW paliko savo** (2.5 TDI, penkiacilindris,
Bosch EDC16, kodai BJJ/BJK/BJL/BJM). **Ross-Tech wiki patvirtina** (žr. Šaltiniai): iš visų
valdymo blokų Crafter'yje **TIK 3 turi gimtąją VW diagnostiką — Variklis, Imobilaizeris,
Centrinis užraktas**. Likę blokai (kėbulo, komforto, pavarų dėžės) yra Mercedes kilmės ir
reikalauja kito protokolo/adresavimo (VCDS jų nemato ar mato ribotai).

**Praktinė išvada mūsų užduočiai:** kadangi mums rūpi TIK variklio duomenys (RPM, temp,
slėgiai, DPF), o variklio ECU yra **tikras VAG modulis** — VAG UDS metodas gilesniems
duomenims (§3) yra architektūriškai teisingas kelias, ne klaidingas spėjimas. Nereikia
tirti Mercedes diagnostikos protokolo apskritai.

**Papildomas radinys — dalis „gilių" parametrų yra STANDARTINIAI, ne VAG-specifiniai:**
TDIClub forumo diskusijos (žr. Šaltiniai) rodo, kad kai kurie dyzelinių variklių parametrai,
kurie atrodo kaip „pažangūs"/gamintojo duomenys, iš tikrųjų yra **SAE J1979 standartiniai
išplėstiniai Mode 01 PID**, veikiantys nepriklausomai nuo gamintojo (jei ECU juos palaiko):
- `0x23` — Degalų rail slėgis (kPa), formulė `(A*256+B)*10`
- `0x5C` — Alyvos temperatūra (°C), formulė `A-40`
- `0x2C`/`0x2D` — EGR komanda % / EGR paklaida %
- `0x7A` — DPF diferencinis slėgis (kPa)
- `0x7C` — DPF temperatūra
Tai reiškia: **prieš bandant VAG Mode 22 spėjimus, pirma reikia patikrinti šiuos standartinius
PID paprastu `01 XX` užklausimu** — jei ECU atsako, gauname patikimus duomenis be jokio
VAG-specifinio tyrimo. Žr. atnaujintą §3 lentelę.

---

## Sprendimas: ELM327, ne OBDeleven (2026-07-06)

Svarstyta alternatyva — OBDeleven adapteris (BLE, oficialiai palaiko VW Crafter, PRO/ULTIMATE
lygiu turi verifikuotą VAG „Live Data UDS"). **Atmesta integracijai į KemperisApp**, nes jo BLE
protokolas niekur viešai nedokumentuotas (skirtingai nuo Renogy, kur yra atviras `renogy-bt`
projektas) — integracija reikalautų nuosavo reverse-engineering nuo nulio, žymiai didesnė
neapibrėžtis nei ELM327 kelias. **Sprendimas: tęsti su pigiu ELM327** (§1-§7 aukščiau) — viešas,
dokumentuotas protokolas, jau patvirtinta veikiantis su šiuo automobiliu. Jei ateityje prireiks
giliųjų VAG duomenų su didesniu pasitikėjimu, OBDeleven galima naudoti **atskirai, jo paties
oficialiame app'e** (ne integruojant į KemperisApp).

## Apimtis

**Daroma:** naujas native Bluetooth Classic tiltelis (Java) su **skenavimu + poravimu tiesiai
iš app'o** (ne tik jau susietų įrenginių sąrašas — žr. §1a), naujas `www/obd.js`, naujas tab UI,
ELM327 AT init + Mode01 PID polling (standartiniai + SAE išplėstiniai), standartinių PID
kortelės, „Gilieji duomenys" (VAG UDS 22) sekcija pažymėta eksperimentine, **dedikuotas OBD-only
diagnostikos žurnalas** (atskiras nuo bendro app `sysLog`, žr. §4) su išsaugojimu faile,
modulių/DID skenavimo įrankis (§3 Pakopa C), versijos bump, APK.

**Rekomenduojama implementacijos tvarka (net vienoje sesijoje):** (a) prisijungimas + poravimas
+ Mode 01 Pakopa A + dedikuotas žurnalas — veikiantis MVP; (b) Pakopa B/C (VAG spėjimai +
skenavimas) — po to, kai (a) patikrinta realiu automobiliu. Doko apimtis išaugo per iteracijas
(BT poravimas, du skenavimo įrankiai, keli nesutampantys DID kandidatų sąrašai) — verta
neimplementuoti visko vienu ypu be tarpinio patikrinimo.

**NEdaroma:** firmware pakeitimai; SSE grandinės pakeitimai; Google Sheets CSV formato
pakeitimai; veikimas fone kai app uždarytas (v1 — tik kai app atidarytas); Renogy BLE logikos
lietimas (visiškai atskiras kodas/transportas); `research/obd2/ESPobd` (atskiras CAN-per-ESP32
kelias, Etapas E CLAUDE.md — nesusijęs su šia užduotimi, nekeisti).

---

## Techniniai reikalavimai

### 1. Kodėl Classic SPP, ne BLE — ir ką tai reiškia

Cheap „ELM327 v1.5" dongle'ai naudoja HC-05/HC-06 tipo Bluetooth Classic modulį, kuris
eksponuoja **Serial Port Profile (SPP)**, standartinis UUID `00001101-0000-1000-8000-00805F9B34FB`.
Tai reiškia:
- Jungiamasi per `BluetoothSocket.createRfcommSocketToServiceRecord(SPP_UUID)` ant
  **susieto (paired)** `BluetoothDevice`. **NAUJA (vartotojo prašymu, 2026-07-06): poravimas
  turi vykti tiesiai iš app'o**, ne tik per sisteminius nustatymus prieš tai — žr. §1a.
- Skaitymas/rašymas — paprastas `InputStream`/`OutputStream` per socket, **ne GATT
  read/write/notify**. Reikia nuolatinės skaitymo gijos (blocking read loop), kaip esami
  `new Thread(() -> {...})` pavyzdžiai `MainActivity.java` (pvz. `httpGet`/`httpPost` async
  pattern, arba TTS bridge su `runOnUiThread` + `evaluateJavascript` callback).

### 1a. Poravimas (pairing) tiesiai iš app'o

Vartotojas nori pasirinkti/suporuoti ELM327 programėlėje, ne eiti į sistemos Bluetooth
nustatymus atskirai. Reikia:

- **Neporuotų įrenginių paieška:** `BluetoothAdapter.startDiscovery()` (reikia `BLUETOOTH_SCAN`
  — jau deklaruota manifest'e) + `BroadcastReceiver` ant `BluetoothDevice.ACTION_FOUND`,
  renkant `EXTRA_DEVICE` + RSSI. Rodyti sąrašą kartu su jau susietais (`getBondedDevices()`) —
  susieti pažymėti kaip „Susietas", nesusieti — su mygtuku „Suporuoti".
- **Poravimas be vartotojo PIN įvedimo (auto-PIN):** dauguma pigių ELM327/HC-05 klonų naudoja
  fiksuotą PIN (dažniausiai `1234`, kartais `0000` arba `6789`). Native pusėje:
  1. `device.createBond()` — inicijuoja poravimą.
  2. `BroadcastReceiver` ant `BluetoothDevice.ACTION_PAIRING_REQUEST`: iškviesti
     `device.setPin("1234".getBytes())` (per viešą `BluetoothDevice.setPin()` arba reflection,
     priklausomai nuo API), tada `abortBroadcast()`, kad sistema neatidarytų PIN įvedimo
     dialogo vartotojui.
  3. `BroadcastReceiver` ant `BluetoothDevice.ACTION_BOND_STATE_CHANGED`: jei `BOND_NONE` po
     bandymo su `1234` — automatiškai bandyti `0000`, tada `6789` (žinomi ELM327 numatytieji
     PIN pagal bendruomenės patirtį), tik po visų nesėkmių rodyti vartotojui klaidą su
     pasiūlymu suporuoti rankiniu būdu per sistemos nustatymus.
  4. Sėkmingo bond'o atveju: `window.onObdPaired && window.onObdPaired(mac, name)`, toliau
     automatiškai kviesti `connect(mac)`.
- **Manifest:** `BLUETOOTH_SCAN`, `BLUETOOTH_CONNECT` jau yra; papildomai galimai reikės
  `BLUETOOTH_ADMIN` maxSdk 30 analogiškai Renogy blokui, jei `startDiscovery()`/`createBond()`
  to pareikalaus senesnėse Android versijose — patikrinti realiu build'u.
- **UI:** naujas „Ieškoti ELM327" mygtukas OBD tab'e (analogiškai Renogy „Ieškoti įrenginių"),
  sąrašas su būsenom (Susietas / Suporuoti / Jungiamasi), po sėkmingo prisijungimo — įrašyti
  MAC į `localStorage` (kaip Renogy `ren_dev_*`), kad kitą kartą jungtųsi be paieškos.

### 2. Native tiltelis — `KemperisObdBridge`

Naujas failas `android/android/app/src/main/java/lt/kemperis/app/KemperisObdBridge.java`,
registruojamas `MainActivity.onCreate()` analogiškai esamiems:
```java
this.getBridge().getWebView().addJavascriptInterface(new KemperisObdBridge(this), "KemperisObd");
```

Metodai (visi `@JavascriptInterface`):
- `String listPairedDevices()` — grąžina JSON sąrašą susietų BT įrenginių (pavadinimas + MAC)
  iš `BluetoothAdapter.getBondedDevices()`. JS pusėje filtruoti/rodyti vartotojui pasirinkti.
- `void startDiscovery()` — `BluetoothAdapter.startDiscovery()`; per `ACTION_FOUND` broadcast
  kiekvienam rastam įrenginiui kviesti `window.onObdDeviceFound && window.onObdDeviceFound(mac, name, rssi)`.
  `void stopDiscovery()` — sustabdo (Android leidžia ribotą skenavimų dažnį — cooldown UI pusėje,
  kaip Renogy).
- `void pair(String mac)` — `device.createBond()` + auto-PIN seka (§1a) background gijoje;
  rezultatas per `window.onObdPaired`/`window.onObdPairFailed`.
- `void connect(String mac)` — background gijoje: `device.createRfcommSocketToServiceRecord(SPP_UUID)`,
  `socket.connect()`. **Pasala:** kai kurie klonai numeta standartinį `connect()` — jei nepavyksta,
  fallback per reflection `device.getClass().getMethod("createRfcommSocket", int.class).invoke(device, 1)`
  (žinomas Android SPP workaround, plačiai dokumentuotas StackOverflow/GitHub issues dėl HC-05 klonų).
  Po sėkmingo connect: `window.onObdConnected && window.onObdConnected()`.
- `void send(String hexOrAscii)` — rašo į `OutputStream`, seka su komanda po komandos (žr. §3
  dėl pacing).
- `void disconnect()` — uždaro socket/streams švariai.
- Nuolatinė skaitymo gija (paleidžiama `connect()` viduje): kaupia **žalius baitus** iš
  `InputStream` (ELM327 atsakymai baigiasi `>` promptu), konvertuoja į String **eksplicitiškai
  nurodant charset** `US-ASCII` (pvz. `new String(bytes, StandardCharsets.US_ASCII)`, NE
  pasikliauti Java platform default charset, kuris priklauso nuo Android lokalės/versijos) —
  saugu, nes ELM327 įprastu AT-komandų režimu visada siunčia tik spausdinamus ASCII simbolius
  (net VIN baitai koduojami kaip hex-pora ASCII tekstas, pvz. `0x57` siunčiamas kaip du
  simboliai `"57"`, ne žalias baitas), kai gaunama pilna eilutė —
  `runOnUiThread(() -> webView.evaluateJavascript("window.onObdData && window.onObdData(" + JSONObject.quote(line) + ")", null))`.
  Atjungimo/klaidos atveju — `window.onObdDisconnected && window.onObdDisconnected(reason)`.

Manifest: **BLUETOOTH_CONNECT jau deklaruota** (`AndroidManifest.xml:25`) — papildomų leidimų
nereikia (skirtingai nei Renogy BLE, kuriam reikėjo naujo bloko).

### 3. ELM327 protokolas (JS pusė, `www/obd.js`)

Init seka po `connect()` (siųsti po vieną, laukiant `>` prompto arba ~200ms tarp komandų —
Gemini teisingai perspėjo dėl mažo buferio pigiuose klonuose):
```
ATZ
ATE0
ATL0
ATH0       (numatyta veikla — be CAN headers, paprastesnis Mode01/Pakopa A/B parsinimas)
ATSP6      (priverstinai ISO 15765-4 CAN 11/500 — jau patvirtinta veikianti šiam automobiliui)
ATSH 7E0   (tik jei reikės tikslinių/deep PID; standartiniams Mode01 nebūtina)
```
**Pastaba:** `ATH1` (headers įjungti) naudoti TIK Pakopos C modulių skenavimo metu (§3), kur
būtina matyti, koks adresas atsakė — po skenavimo grąžinti į `ATH0` prieš tęsiant normalų
polling'ą.

**Pakopa 0 — bazinis Mode 01 (visada veikia).** Naudoti tuos pačius PID kaip
`research/obd2/ESPobd/src/main.cpp` (jau parašytos formulės, patikrintos su tuo pačiu
automobiliu per CAN):

| PID | Pavadinimas | Formulė |
|---|---|---|
| 0C | RPM | (A*256+B)/4 |
| 0D | Greitis km/h | A |
| 05 | Aušinimo skystis °C | A-40 |
| 04 | Apkrova % | A*100/255 |
| 0B | Įsiurbimo slėgis kPa | A |
| 10 | Oro srautas g/s | (A*256+B)/100 |

Papildomai lengva pridėti analogiškai: `0F` įsiurbimo oro temp (A-40), `11` droselis %
(A*100/255), `1F` veikimo laikas s (A*256+B), `46` aplinkos temp (A-40), Mode `03`/`04`
klaidų kodai. Užklausa: `01XX\r`, atsakymas `41 XX A B ... \r>` (be CAN headers, nes `ATH1`
šiuo atveju **nebūtinas** standartiniam Mode01 — įjungti `ATH0` numatytai veiksenai, kad
atsakymus būtų paprasčiau parsinti; `ATH1` naudoti tik jei reikės matyti iš kurio ECU atėjo
atsakymas, kas 1-ECU sistemai nebūtina).

**„Gilieji duomenys" — DVI PAKOPOS** (svarbus atnaujinimas po 2026-07-06 online tyrimo, žr.
Patvirtinti faktai → Platformos tyrimas):

**Pakopa A — standartiniai SAE J1979 išplėstiniai Mode 01 PID.** Šie NĖRA gamintojo-specifiniai,
tiesiog rečiau naudojami — bandyti **pirma**, be jokio papildomo tyrimo, paprasta `01 XX`
užklausa (tas pats formatas kaip Pakopos 0 lentelėje aukščiau):

| PID | Pavadinimas | Formulė | Pastaba |
|---|---|---|---|
| 23 | Degalų rail slėgis | (A*256+B)*10 kPa | dyzeliams dažnai palaikoma |
| 5C | Alyvos temperatūra | A-40 °C | |
| 2C | EGR komanda | A*100/255 % | |
| 2D | EGR paklaida | (A-128)*100/128 % | |
| 7A | DPF diferencinis slėgis | (A*256+B)/100 kPa | TDIClub patvirtinta panašiems dyzeliams |
| 7C | DPF temperatūra | (A*256+B)/10 - 40 °C | |

Jei ECU atsako reikšme (ne `NO DATA`) — rodyti kaip normalią kortelę, **be** eksperimentinio
žymėjimo (tai standartas, ne spėjimas). Jei `NO DATA` — tiesiog nerodyti tos kortelės (šis
konkretus EDC16 diegimas jos gali nepalaikyti).

**Pakopa B — VAG-specifinis Mode 22 UDS** (DPF suodžių masė, turbo slėgis prašomas/faktinis,
purkštukų korekcijos ir pan.) — **eksperimentinė sekcija**, aiškiai pažymėta UI (pvz. įspėjimas
„Neverifikuota — gali rodyti klaidingas reikšmes"). Gemini pasiūlyti PID (`22 11 4C` DPF
suodžiai, `22 11 1F` alyvos temp, `22 11 6B`/`6C` turbo slėgis, `22 11 93` rail slėgis,
`22 12 A0..A4` purkštukų korekcijos) **NEVERIFIKUOTI** — LLM sugeneruoti be realaus šaltinio
patvirtinimo.

**Papildomas tyrimas (2026-07-06) — praktinis empirinis kelias vietoj tolimesnio spėliojimo:**
- **Torque Scan** (Torque Pro papildinys, TDIClub bendruomenėje minimas) automatiškai peržvelgia
  Mode 22 DID diapazoną konkrečiame automobilyje ir parodo, kurie realiai atsako — tai
  **žymiai patikimesnis** startas nei bet koks kito modelio/kitos kartos kodų sąrašas.
  **Rekomendacija: prieš (arba lygiagrečiai su) app implementacija, paleisti Torque + Torque
  Scan su tuo pačiu ELM327 adapteriu ir šiuo automobiliu** — gauti realų, šiam konkrečiam
  EDC16 patvirtintą DID sąrašą, kurį tada galima tiesiog perkelti į `obd.js`, vietoj spėjimo.
- Rastas dar vienas kandidatas alyvos temperatūrai — bendruomenės XGauge→Torque konversijos
  pavyzdyje minimas `22 13 10` (Mode 22, DID `0x1310`) su formule
  `((A*256+B))*100*(9/5)-4001` (F°, konvertuoti į C° jei pasitvirtins) — **testuoti ABU**
  kandidatus (`22 11 1F` ir `22 13 10`) realiu automobiliu, žr. kuris grąžina prasmingą,
  laikui bėgant logiškai kintančią reikšmę.
- TDIClub bendruomenė istoriškai naudojo bendrą „VW.csv" Torque custom PID rinkinį (fuel/rail
  slėgis, boost gauge, Charged Air Cooler temp, EGT1-4) — originalus Dropbox saugojimo nuoroda
  **nebeveikia** (failas ištrintas), bet paties rinkinio egzistavimas patvirtina, kad šie
  parametrai VW TDI bendruomenėje buvo pasiekiami per Torque custom PID. Jei turi TDIClub
  paskyrą, verta patikrinti gijas `showthread.php?t=415791`, `t=456913`, `t=430012` — gali būti
  atnaujinta nuoroda prisijungusiems nariams (šis agentas neturi prisijungimo, todėl negalėjo
  pasiekti).
- Bendra Mode 22 + 2-baitų DID konvencija **patvirtinta teisinga** dviem nepriklausomais
  šaltiniais skirtingoms VAG kartoms: senos EDC16/XGauge tradicijos (aukščiau) IR naujesnio
  atviro projekto `bri3d/MQBSimosLogVariables` (MQB/Simos platformoms) — abu naudoja tą patį
  „0x22 + DID" formatą, tik skirtingi adresai skirtingoms ECU kartoms. Tai stiprina
  pasitikėjimą pačiu METODU, net jei konkretūs adresai lieka po testavimo.

**Papildomas kandidatų sąrašas (vartotojo pateiktas 2026-07-06, kito LLM sugeneruotas) — kritiškai
įvertintas:**

| Pasitikėjimas | DID | Aprašymas | Kodėl |
|---|---|---|---|
| ✅ AUKŠTAS | `22 F1 90` | VIN | Standartizuotas ISO 14229-1 (`F180-F1A0` diapazonas privalomas visiems UDS ECU) — **naudoti kaip sanity-check**: jei atsakymas sutampa su jau žinomu VIN (`WV1ZZZ2EZ86028774`), tai patvirtina, kad Mode 22 užklausimo grandinė veikia teisingai. |
| ✅ AUKŠTAS | `22 F1 8C` | ECU serijos numeris | Taip pat standartizuotas ISO 14229-1 diapazonas. |
| ⚠️ VIDUTINIS | `22 F1 9F` | Programinės įrangos versija | Tikėtinai standartizuoto diapazono viduje, bet tikslus sub-ID gali skirtis. |
| ⚠️ TESTUOTI | `22 03 80`/`81-85` | Purkštukų korekcijos (visos/cilindrais) | **PRIEŠTARAUJA** anksčiau minėtam Gemini `22 12 A0..A4` tam pačiam parametrui — du LLM šaltiniai, du skirtingi atsakymai = nė vienas nepatvirtintas, testuoti abu. |
| ⚠️ TESTUOTI | `22 03 70` | Smooth Running / RPM deviation | Neverifikuota, bet adresas (variklio ECU) architektūriškai tinkamas. |
| ⚠️ TESTUOTI | `22 01 08` | Fuel injection quantity | Neverifikuota. |
| ⚠️ TESTUOTI | `22 03 90` | Išmoktos purkštukų vertės (adaptacija) | Neverifikuota. |
| ❌ NEBANDYTI VARIKLIO ADRESU | `22 41 0x`, `22 F1 D0`, `22 40 1x/2x/3x/4x`, `22 F1 42/50/20`, `22 F1 A8` (SAM/komfortas: durys, užraktai, apšvietimas, akum/alternatorius, lauko temp) | **Architektūriškai neįmanoma siunčiant į `0x7E0`** — SAM/kėbulo moduliai naudoja VISIŠKAI KITĄ CAN adresą nei variklis (žr. §8 — pvz. prietaisų skydelis `0x714`, ne `0x7E0`). Siunčiant šiuos DID į variklio ECU adresą beveik tikrai gausime „service not supported". **BET** — jei §3 Pakopa C „Skenuoti modulius" ras kitą atsiliepiantį adresą, ŠIUOS DID galima (atsargiai) pabandyti TAME rastame adrese, ne §8 pasyviame sniffing'e laukti. |

**Praktinė rekomendacija:** pirmiausia testuoti `22 F1 90` (VIN sanity-check) kaip Mode 22
grandinės patikrą variklio adresu, tada likusius „TESTUOTI" žymėtus variklio kandidatus. Kai
Pakopa C „Skenuoti modulius" (žemiau) atras kitus atsiliepiančius adresus — tada, ir tik tada,
bandyti SAM/komforto DID tuose naujai rastuose adresuose (ne aklai `0x7E0`).

Prieš rodant Pakopos B reikšmę kaip realią:
1. Prieš Mode 22 — pirma nusiųsti `10 03` (extended diagnostic session), nes EDC16 gali
   atsisakyti Mode 22 be sesijos (žr. pasala #5).
2. Siųsti užklausą, loginti raw HEX atsakymą (§4).
3. Palyginti su antru šaltiniu (Torque Scan rezultatas, Ross-Tech forumas, TDIClub, T5 2.5 TDI
   bendruomenė), arba faktinis stebėjimas (pvz. užvedus šaltą variklį alyvos temp turi kilti
   pamažu, ne šokinėti; DPF slėgis turi kisti su apsukom).
4. Tik patvirtinus — perkelti iš „raw/debug" į normalią kortelę su verte.
Jei ECU negrąžina atsakymo (NO DATA / negative response `7F 22 <NRC>`) — rodyti „Nepalaikoma",
ne klaidą ar 0.

**Buferio apribojimas:** siųsti po vieną komandą, laukti atsakymo (arba max ~300ms timeout)
prieš siunčiant kitą — **jokių sudėtų/batch užklausų**. Polling ciklas — kaip Renogy
`pollCycle`, kas ~500ms-1s viena komanda paeiliui per masyvą (žr. ESPobd `pids[]` masyvo pavyzdį).

**Pakopa C — Skenavimo/testavimo įrankis (vartotojo prašymu, 2026-07-06): vietoj tolimesnio
spėliojimo, statome empirinio atradimo funkciją tiesiai į app'ą.** Vietoj to, kad pasitikėtume
bet kuriuo neverifikuotu sąrašu, tab'e bus du „Skenuoti" mygtukai, kurie sistemingai bando
kandidatus ir viską loguoja/išsaugo analizei:

1. **„Skenuoti modulius"** — siunčia lengvą zondą paeiliui į kandidatų CAN adresų sąrašą:
   standartinį diapazoną `0x7E1`-`0x7E7` (jei Mercedes juos vis dėlto naudotų), žinomus iš
   tyrimo Mercedes-specifinius adresus (`0x714` prietaisų skydelis, `0x716` aktyvi vairo
   sistema — žr. §8 šaltinius), ir **pilną brute-force diapazoną `0x700`-`0x7FF`** (256
   adresai). Prieš kiekvieną — `ATSH <hex>`. **Kiekvienam adresui bandyti DU zondus** (ne
   tik vieną): `10 01` (standard session) IR `10 03` (extended session) — kai kurie Mercedes
   ABS/SRS moduliai ignoruoja `10 01` iš neautorizuoto adapterio, bet atsako (net su klaidos
   kodu) į `10 03`. Tai reiškia ~2× ilgesnis skenavimas (256 adresai × 2 užklausos × ~300ms ≈
   150s) — priimtina, bet UI progreso indikatorius turi tai atspindėti. Loguoja: adresas →
   atsakė/neatsakė kiekvienai sesijai (net neigiamas UDS atsakymas `7F ...` reiškia „kažkas
   ten yra", tuščia = nieko). **Tai ir yra praktinis atsakymas į „ar galima pasiekti kitus
   modulius" klausimą — vietoj spėjimo, tiesiog pamatome, kas realiai atsiliepia.**
2. **„Skenuoti DID (variklis)"** — patvirtintam variklio adresui (`0x7E0`) paeiliui siunčia
   `22 XX XX` iš viso kandidatų sąrašo (aukščiau esanti lentelė — Gemini pasiūlyti + naujas
   vartotojo sąrašas + standartizuoti `F1xx`), loguoja kiekvieno raw atsakymą.

**Pakopa C+ (papildyta 2026-07-06, dar vieno LLM pasiūlymai — patikrinti, dauguma pagrįsti):**

3. **ECU identiteto nustatymas prieš skenavimą (Mode 09)** — standartinis SAE J1979 servisas,
   NE spėjimas: `09 02` (VIN — jau žinomas kaip sanity-check kryžminei patikrai su Mode 22
   `F1 90`), `09 04` (Calibration ID — **jau žinomas iš OBD Auto Doctor testo**:
   `074906032AT 5175`, tad šis PID tikrai veikia šiam automobiliui), `09 0A` (ECU Name —
   dar netestuota, verta pridėti). Naudinga vėliau ieškant tikslių Map/DAMOS failų pagal
   konkretų calibration ID.
4. **Protokolo zondas (KWP vs UDS) + Service 21 blokų skenavimas** — **patvirtinta realus
   mechanizmas** (2026-07-06 papildoma paieška): KWP2000 standarte „Measuring Blocks" (VCDS
   „Group" skaitymai) yra tiesiog `ReadDataByLocalIdentifier` (Service `0x21`) su 1-baitu local
   identifier — patvirtinta ir realiu atviro kodo projektu `jazdw/vag-blocks` (žr. Šaltiniai),
   kuris būtent tai implementuoja VAG varikliui/pavarų dėžei per KWP2000. Prieš pasitikint vien
   Mode 22 (UDS), papildomai: (a) išbandyti `21 01` prieš `22 F1 90` kaip pirminį zondą — jei
   atsako į `21 01`, bet ne `22 F1 90`, šis EDC16 orientuotas į KWP „grupes"; (b) jei taip —
   atlikti **pilną blokų sweep `21 01`-`21 FF`** (256 galimos grupės, analogiškai adresų
   sweep §3 punkte 1), loguojant kiekvieno atsakymą. **Svarbu:** konkretūs Group→parametras
   priskyrimai (pvz. „Group 13 = purkštukai") kitų šaltinių NEVERIFIKUOTI — net `vag-blocks`
   projektas patvirtina, kad kiekvienam moduliui/ECU reikia atskirų „label failų", t.y.
   grupių reikšmės NĖRA universalios. Bet kokie konkretūs Group numeriai — tik testavimo
   kandidatai, tokie patys netikri kaip Mode 22 DID sąrašai, ne patvirtinti faktai.
   **Automatinis fallback (naujas saugiklis):** jei Pakopa B/C DID skenavimas (`22 XXXX`)
   grąžina VISUR `NO DATA`/negative response — automatiškai paleisti Service 21 block sweep
   kaip „Planą B", kad testas negrįžtų su tuščiu logu vien dėl neteisingo protokolo prielaidos.
5. **Latency matavimas** — kiekvienai skenavimo/polling užklausai fiksuoti laiką (ms) tarp
   komandos išsiuntimo ir `>` prompto gavimo. Naudinga vėliau sprendžiant realistišką polling
   dažnį (pvz. turbo slėgiui reikėtų ~10Hz, temperatūroms pakanka ~1Hz — priklauso nuo
   faktinio šio adapterio greičio, ne prielaidos).
6. **CAN magistralės klaidų stebėjimas** — **patvirtinta**: `ATCS` yra tikra ELM327 komanda
   (nuo v1.0), rodanti CAN Tx/Rx klaidų skaitiklius hex formatu. Periodiškai (pvz. kas 20-30
   skenavimo žingsnių) siųsti `ATCS` ir loguoti — jei klaidų skaitiklis auga, polling dažnis
   per agresyvus, reikia didinti pauzes tarp komandų.
7. **Multiframe/Flow Control testas per VIN** — `09 02` (arba `22 F1 90`) grąžina 17 simbolių
   VIN, kuris netelpa į vieną CAN kadrą — priverčia ELM327 naudoti ISO-TP daugiakadrį
   perdavimą (First Frame + Consecutive Frames + Flow Control). Jei mūsų native/JS
   implementacija gauna NUKIRSTĄ VIN (ne pilnus 17 simbolių) — tai signalas, kad reikia
   rankinio flow control valdymo. **Pataisymas** (originaliame pasiūlyme buvo `ATAFC 1`, kurio
   NĖRA ELM327 komandų sąraše): tikri komandų pavadinimai —
   `ATCFC1`/`ATCFC0` (automatinis flow control įjungti/išjungti) arba rankinis
   `ATFCSH`/`ATFCSD`/`ATFCSM` (flow control header/data/mode), jei automatinio nepakaktų.
8. **Būsenos žymėjimas loge (naujas, 2026-07-06)** — UI mygtukai/laukas testavimo metu pažymėti
   momentus: „Degimas įjungtas, variklis išjungtas" / „Laisva eiga" / „Dujinu iki N RPM" ir t.t.
   — laiko žyma įrašoma OBD žurnale kartu su HEX duomenimis. Būtina, nes pvz. HEX `05 A2`
   nieko nereiškia analizei, jei nežinome, ar tuo metu variklis stovėjo, ar suko apsukas —
   lyginant HEX vertes tarp pažymėtų būsenų, lengviau nustatyti, kuris baitas atitinka RPM/
   turbo slėgį/kitą parametrą.

**Atmesta kaip techniškai klaidinga (2026-07-06, patikrinta):** kito pasiūlymo punktas apie
`10 85` kaip „pažadinimo" subfunkciją miegantiems moduliams — **patikrinau: `0x85` yra
VISIŠKAI ATSKIRAS UDS servisas** („Control DTC Setting", ne Service `0x10` subfunkcija) —
tai protokolo elementų sumaišymas, ne realus mechanizmas. Taip pat pasiūlyta `ATST 64` kaip
„adaptive timing" — **netiksliai**: `ATST` nustato fiksuotą timeout'ą (hex reikšmė×4ms), o
tikra adaptyvaus laiko komanda yra `ATAT1`/`ATAT2`. Miegantiems moduliams lieka galioti tai,
ką jau turime §3 punkte 1 — bandyti `10 01` IR `10 03` (tikras, standartizuotas extended
session), ne fabrikuotą `10 85`.

**Žurnalo formatas (visiems Pakopa C+ testams):** kiekviena eilutė — susietas TX→RX porinis
įrašas, ne atskiros nesusijusios eilutės, pvz.:
```
TX: 22 11 4F -> RX: 62 11 4F 05 A2 (Δ42ms)
```
Tai leidžia vėliau analizuoti tikslų HEX atsakymą, jei UI formulė duotų nesąmonę (pvz.
temperatūrą 500°C dėl blogos formulės) — su raw HEX galima nustatyti teisingą formulę
retrospektyviai, be HEX — testas beviertis.

**⚠️ Saugumo pastaba (svarbu):** šis skenavimas turi būti vykdomas **TIK stovint** (variklis
išjungtas arba tuščia eiga, automobilis nejuda) — nežinome, kaip nepažįstami Mercedes moduliai
(ypač ABS/ESP, saugos su airbag susiję) reaguoja į netikėtą diagnostikos sesijos užklausą; kai
kurie automobiliai laikinai išjungia tam tikras funkcijas kol aktyvi diagnostikos sesija. UI
privalo turėti aiškų patvirtinimo dialogą („Ar automobilis stovi? Tęsti tik jei TAIP") prieš
paleidžiant bet kurį skenavimą, ir modulių skenavimas (1) turėtų būti aiškiai atskirtas nuo
variklio DID skenavimo (2) kaip rizikingesnis (nežinomi moduliai) veiksmas.

Rezultatai (adresas/DID, raw hex, laiko žyma) rašomi į naują dedikuotą OBD logą (§4) ir
išsaugomi failan analizei (§4), kad būtų galima peržiūrėti po fakto, ne tik gyvai ekrane.

### 4. OBD diagnostikos žurnalas (atskiras nuo bendro app debug logo)

Vartotojo prašymu — **atskiras** OBD-only diagnostikos žurnalas, ta pačia logika kaip esamas
bendras `sysLog`/terminalas (`index.html` — `sys-log-output`, `saveTerminal()`), bet **savo
atskirame UI bloke OBD tab'e**, kad ELM327/CAN srautas neužterštų bendro app žurnalo:

- Nauja funkcija `obdLog(msg, type)` (analogiška `sysLog`) rašanti į naują
  `<div id="obd-log-output">` OBD tab'e — spalvinimas kaip esamas (`info`/`warn`/`error`/`data`/
  `success`), su laiko žyma.
- Kiekvienas siųstas/gautas ELM327 baitas/eilutė (tiek įprastas polling'as §3, tiek skenavimo
  rezultatai §3 Pakopa C) — visada į šį žurnalą, ne tik kai bendras debug jungiklis įjungtas
  (nes tai ir yra pagrindinis šio tab'o diagnostikos/testavimo įrankis, ne pridėtinė detalė).
- **„Išsaugoti OBD logą"** mygtukas — analogiškai `saveTerminal()`: naudoti
  `KemperisFile.saveTextFile(filename, 'text/plain', content)` (jau egzistuojantis native
  bridge, žr. `index.html` `saveTerminal()` pavyzdį), failo vardas
  `obd_log_<ISO-laikas>.txt`, į Downloads. Tai leis analizuoti skenavimo rezultatus vėliau
  (pvz. atsiuntus man kaip šiame pokalbyje jau darėme su bendru terminalo logu).
- **„Išvalyti OBD logą"** mygtukas (analogiškai `clearTerminal()`).

### 5. UI — naujas tab „OBD"

- Pridėti `'obd'` į `SWIPE_TABS` masyvą (`index.html:1532`) ir meniu.
- Viršuje: mygtukas **„Ieškoti ELM327"** (paleidžia `startDiscovery()`), sąrašas su rastais +
  jau susietais įrenginiais, būsenom (Susietas / Suporuoti / Jungiamasi) ir mygtuku
  „Suporuoti"/„Prisijungti" prie kiekvieno (žr. §1a). Po sėkmingo prisijungimo MAC įsimenamas
  `localStorage`, kitą kartą jungiamasi automatiškai be paieškos.
- Ryšio būsena: prijungta / jungiamasi / atjungta (pattern kaip Renogy).
- Standartinių PID kortelės (RPM, greitis, temp, apkrova, MAP, MAF, + Pakopa A iš §3 jei ECU
  palaiko) — atsinaujina gyvai kol tab'as atidarytas.
- Mygtukas/perjungiklis „Gilieji duomenys" — atidaro Pakopos B eksperimentinę sekciją (§3).
- **„Skenavimas/testavimas" sekcija** (nauja, žr. §3 Pakopa C): du mygtukai „Skenuoti modulius"
  ir „Skenuoti DID (variklis)", abu su patvirtinimo dialogu („Ar automobilis stovi?") prieš
  paleidžiant. Progreso indikatorius skenavimo metu (adresų/DID kiekis ×300ms gali užtrukti
  iki ~1-2 min).
- **Dedikuotas OBD žurnalo blokas** (žr. §4) — atskiras nuo bendro app terminalo, su
  „Išsaugoti"/„Išvalyti" mygtukais.
- Pagrindinis ON/OFF jungiklis tab'o viršuje (kaip Renogy) — OFF atsijungia, kad vartotojas
  galėtų naudoti OBD Auto Doctor ar kitą app (SPP paprastai leidžia tik vieną aktyvų klientą).

### 6. Integracija su esamu app

- Nauja logika **atskirame** `www/obd.js` (script tag), ne monolite `index.html` —
  ta pati priežastis kaip Renogy: mažiau merge konfliktų, lengvesnis būsimas perkėlimas.
- **Nekeisti:** SSE grandinės, `_fetchSheetsNow`, cloud aliarmų, Sheets CSV (22 laukai),
  Renogy BLE kodo, `research/obd2/ESPobd` CAN firmware.
- OBD duomenys **nepatenka** į lokalų CSV log v1 (skirtingai nei Renogy) — tai atskiras
  duomenų šaltinis be ryšio su energijos/aplinkos logika; jei prireiks vėliau, atskira užduotis.

### 7. Versijos ir build — taikinys **v47**

| Kas | Dabar | Naujas |
|---|---|---|
| `version.json "version"` | 46 | **47** |
| `versionCode` (build.gradle) | 46 | **47** |
| `versionName` | "34.0" | **"35.0"** |
| APK failas | `kemperis_v46.apk` | `kemperis_v47.apk` |
| `version.json "apk_url"` | .../kemperis_v46.apk | .../kemperis_v47.apk |

- `version.json "notes"` — trumpas v47 changelog (OBD/ELM327 tab, Bluetooth Classic, deep PID
  eksperimentinė sekcija, debug HEX logas).
- Build: `npx cap sync android` **PRIVALOMA** prieš Gradle build (žinoma projekto pasala —
  žr. atmintį apie APK release/cap sync).

---

## Žinomi apribojimai (dokumentuoti UI/README)

- Vienu metu prie SPP adapterio gali būti prisijungęs tik vienas klientas — jei atidaryta
  OBD Auto Doctor ar kita app, mūsų app'as neprisijungs (klaidos pranešime tai paminėti).
- „Gilieji duomenys" PID adresai neverifikuoti realiu VAG šaltiniu — gali reikėti koregavimo
  po pirmo realaus testo su automobiliu.
- V1 veikia tik kai tab'as atidarytas, app atidarytas (jokio foreground polling fone).

## Žinomos pasalos (privaloma atsižvelgti)

1. **Standartinis SPP `connect()` gali nepavykti** su kai kuriais HC-05 klonais (žinoma
   Android+ELM327 problema) — būtinas reflection fallback (`createRfcommSocket(1)`), žr. §2.
2. **Buferio perpildymas** („BUFFER FULL") — siųsti tik po vieną komandą, laukti atsakymo,
   ~50-300ms pauzė, jokių sudėtinių užklausų vienu metu.
3. **Atsakymo pabaigos žymė** — ELM327 baigia atsakymą `>` simboliu (prompt), o ne newline;
   parseris turi kaupti baitus, kol pasirodo `>`, tada išvalyti buferį.
4. **`ATSP6` priverstinis protokolas** patvirtintas veikiantis šiam automobiliui — nenaudoti
   `ATSP0` (auto-detect) kaip pirminės parinkties, kad išvengtume lėto derėjimosi kiekvieną
   kartą prisijungus.
5. **Neigiamas UDS atsakymas** `7F 22 <NRC>` reiškia PID nepalaikomas arba reikia sesijos
   (`10 03` extended diagnostic session) prieš Mode 22 — jei deep PID negrąžina nieko,
   pabandyti pirma nusiųsti `10 03`, tik tada `22 XX XX`.
6. **Socket disconnect** ne visada iškarto grąžina klaidą — read gija gali „pakibti" ant
   blokuojančio read; naudoti timeout arba atskirą stop flag, kad gija baigtųsi švariai.

## Priėmimo kriterijai

1. `OBD` tab'as atsidaro, swipe veikia, kiti tab'ai (ypač `renogy`, SSE, Sheets) — be regresijų.
2. Paieška randa adapterį (susietą arba nesusietą); poravimas iš app'o pavyksta automatiškai
   (auto-PIN, be vartotojo įvedimo) arba aiškiai nurodo, kad reikia rankinio poravimo; kitą
   kartą prisijungiama iš `localStorage` be pakartotinės paieškos.
3. Standartiniai PID (RPM, greitis, temp, apkrova, MAP, MAF) atsinaujina gyvai ≤1s intervalu
   variklio veikimo metu ir keičiasi logiškai (RPM kyla dujinant, temp kyla šylant).
4. Kiekviena siųsta/gauta ELM327 eilutė (polling IR skenavimas) matosi dedikuotame OBD žurnale
   (§4), nepriklausomai nuo bendro app debug jungiklio; „Išsaugoti" mygtukas sukuria failą
   Downloads su pilnu žurnalo turiniu.
5. Atjungus adapterį (arba OFF jungikliu) — kortelė rodo „atjungta", app nekrenta.
6. „Gilieji duomenys" sekcija aiškiai pažymėta kaip eksperimentinė; jei PID negrąžina
   validžių duomenų — rodo „Nepalaikoma/Neverifikuota", ne šiukšles kaip realią vertę.
7. „Skenuoti modulius" ir „Skenuoti DID" mygtukai veikia tik po patvirtinimo dialogo („ar
   automobilis stovi?"); skenavimo metu rodomas progresas (įskaitant dvigubą 10 01/10 03
   testavimą per adresą — ~150s bendra trukmė); rezultatai patenka į OBD žurnalą TX→RX pora su
   ms trukme IR išsaugomi faile; app nekrenta net jei skenuojamas adresas neatsako arba grąžina
   klaidą.
7a. Prieš Pakopos B/C DID skenavimą — Mode 09 identiteto užklausos (`09 02`/`09 04`/`09 0A`) ir
    protokolo zondas (`21 01` vs `22 F1 90`) atlikti ir rezultatai matomi OBD žurnale.
8. Versijų identifikatoriai sinchronizuoti (v47); APK vardas == `apk_url`.
9. Įgyvendinus — trumpas audito įrašas `docs/audits/` ir git commit.

### 8. Fazė 2 — viso automobilio duomenys (ATSKIRAS tyrimas, NE šio app tab'o dalis)

**Kontekstas:** vartotojas nori ne tik variklio, o kiek įmanoma daugiau duomenų iš automobilio.
Kadangi Crafter yra hibridas su Mercedes Sprinter (W906) — kėbulo/komforto/ABS/transmisijos
moduliai yra Mercedes kilmės, ne VW (žr. Platformos tyrimas aukščiau). 2026-07-06 online
tyrimas (šaltinis: `rnd-ash/mercedes-hacking-docs`, atviro kodo Mercedes diagnostikos
reverse-engineering projektas) atskleidė esminį architektūros faktą:

> „OBD-II jungtis visuose modeliuose yra filtruojama (per EIZ arba SAM modulį) — ji **leidžia
> SKAITYTI** visų CAN tinklų (M-CAN galios/pavaros, I-CAN/B-CAN kėbulo, D-CAN diagnostikos)
> pranešimus, bet **RAŠYTI** (t.y. aktyviai užklausti) galima tik diagnostiniais CAN ID."

**Praktinės pasekmės:**
1. **Aktyvus užklausimas** (kaip variklio Mode 01/Mode 22 §3) veikia TIK varikliui, nes tai
   vienintelis modulis, atsakantis standartiniu legislated OBD adresu (`0x7DF`/`0x7E0`/`0x7E8`) —
   patvirtinta paties vartotojo testu (OBD Auto Doctor ECU auto-scan rado TIK vieną ECU).
   Kiti Mercedes moduliai (ABS/ESP, SAM/kėbulas, transmisija, prietaisų skydelis) naudoja
   **Mercedes nuosavus (proprietary) CAN ID**, ne standartinį `0x7Ex` diapazoną — pvz. rasti
   pavyzdžiai kitiems modeliams: prietaisų skydelis `0x714`→`0x77E`, aktyvi vairo sistema
   `0x716`→`0x780` (žr. Šaltiniai). **W906 (Sprinter/Crafter) konkretūs ID viešai nerasti** —
   cituojamas sąrašas apima tik lengvuosius W-serijos modelius (W164/169/203/209/211/215/216/
   219/221/240/245/251), W906 tarp jų NĖRA. Reikėtų atskiro tyrimo/sniffing šiam konkrečiam
   modeliui.
2. **Pasyvus klausymasis (sniffing)** — PATVIRTINTA veikia be jokio papildomo protokolo
   tyrimo: kadangi OBD lizdas leidžia skaityti VISŲ tinklų srautą, pakanka **klausytis**, ne
   klausti. Daug automobilio duomenų (rato greitis iš ABS, durų/šviesų statusas iš kėbulo
   modulio, vairo kampas ir pan.) transliuojami CAN magistrale periodiškai BE užklausimo — tai
   standartinė automobilių CAN architektūra. Tam **jau turime paruoštą aparatūrą**:
   `research/obd2/ESPobd/src/main.cpp` jau turi pilną RAW logavimą (`Serial.print("RAW,...")`
   kiekvienam CAN kadrui, nepriklausomai nuo ID) — kodo keisti nereikia, tereikia sesijos.

**Atnaujinta 2026-07-06:** §3 dabar apima „Pakopa C" — aktyvų modulių adresų skenavimą tiesiai
iš telefono/ELM327 app'e (brute-force `0x700`-`0x7FF` + žinomi kandidatai, žr. §3). Tai
GREITESNIS pirmas žingsnis nei aukščiau (2) aprašytas ESP32 sniffing — galima paleisti iš karto,
kai tik OBD tab'as veikia, nereikia atskiro važiavimo/sesijos. Jei skenavimas ras atsiliepiantį
adresą, tolimesnė analizė (kokie DID ten veikia) daroma TAME PAČIAME app'e (§3 Pakopa C punktas
2, pritaikytas naujam adresui). ESP32 pasyvus sniffing (2) lieka vertingas PAPILDOMAI — jis
atskleidžia duomenis, kurie transliuojami BE užklausimo (ko aktyvus skenavimas nerodo), bet
nebėra vienintelis/pirmas žingsnis kitiems moduliams pasiekti.

**Rekomenduojamas veiksmų planas šiai daliai (atskira užduotis, ne dabar):**
1. Prijungti ESP32-S3 CAN rig'ą (jau egzistuoja) prie OBD lizdo, važiuoti/naudoti automobilį
   (posūkiai, šviesos, durys, stabdžiai, vairas) su USB logavimu į `crafter_log.csv` arba
   Serial monitorių.
2. Koreliuoti: kai atliekamas veiksmas (pvz. įjungtas posūkis), ieškoti NAUJŲ arba
   PASIKEITUSIŲ CAN ID sraute tuo momentu — klasikinis CAN reverse-engineering metodas (nereikia
   žinoti Mercedes protokolo specifikos, tik stebėti koreliaciją).
3. Rezultatus (atrastus ID + reikšmes) dokumentuoti naujame `docs/` faile
   (pvz. `crafter_can_id_zemelapis.md`) prieš planuojant bet kokią UI integraciją.
4. Tik po šio empirinio tyrimo spręsti, ar/kaip šiuos duomenis rodyti app'e (galimai per ESP32
   CAN rig'ą → WiFi/Serial → app, NE per ELM327/telefono Bluetooth, nes ELM327 čia nereikalingas
   pasyviam klausymuisi — tai atskira architektūra nuo §1-§7).

**Kodėl tai NEdaroma dabar kartu su ELM327 tab'u:** skirtingi įrenginiai (ESP32 CAN rig, ne
telefono Bluetooth), skirtinga metodologija (pasyvus stebėjimas + empirinė koreliacija
važiuojant, ne UI programavimas), ir rezultatas nežinomas iš anksto (kiek/kokių ID ras) —
tai tyrimo, ne implementacijos užduotis. §1-§7 (ELM327 app tab, variklis) galima daryti dabar
nepriklausomai nuo šios dalies.

## Šaltiniai

- Vietinis tyrimas (tas pats automobilis, patvirtinta veikianti PID/protokolo schema):
  `research/obd2/ESPobd/src/main.cpp`
- Esamas native bridge pattern: `android/android/app/src/main/java/lt/kemperis/app/MainActivity.java`
  (pvz. `httpGet`/`httpPost`, TTS bridge — async Thread + evaluateJavascript callback)
- Esamas analogiškas BT tab: `android/www/renogy.js`, `docs/uzduotis_renogy_ble_tab.md`
- Android BluetoothSocket/SPP dokumentacija: https://developer.android.com/guide/topics/connectivity/bluetooth/transfer-data
- **Platformos/protokolo tyrimas (2026-07-06):**
  - Ross-Tech Wiki, VW Crafter (2E) — patvirtina tik 3 modulius su gimtąja VW diagnostika
    (Variklis/Imobilaizeris/Centrinis užraktas), likę Mercedes kilmės:
    https://wiki.ross-tech.com/wiki/index.php/VW_Crafter_(2E)
  - UK Volkswagen Forum — VCDS ribotumas Crafter Mercedes moduliams:
    https://www.volkswagenforum.co.uk/threads/most-complete-diagnostic-tool-for-crafter-vcds-vs-others-for-all-the-mercedes-modules.38596/
  - ECU dalies numeris 074906032AS/0281014134 (Bosch EDC16) patvirtinimas:
    https://www.ecutesting.com/product-catalogue/volkswagen/crafter/ecu-engine-management/eem014134/
  - TDIClub — PID `0x7A` kaip DPF diferencinis slėgis (standartinis SAE PID, ne VAG):
    http://forums.tdiclub.com/showthread.php?t=379398
  - TDIClub — DPF soot level bendrai (measured vs calculated soot modeliai):
    http://forums.tdiclub.com/showthread.php?t=455880
  - TDIClub — boost pressure VCDS measuring block Gr.11 (2.5/1.9 TDI, tas pats variklis kaip T5,
    naudinga kryžminei patikrai net jei Crafter'yje VCDS grupių neturime): matavimo blokų
    aptarimas per T5 forumus (žr. paieškos rezultatus aukščiau šiame pokalbyje).
- VAG UDS/KWP diagnostika (Pakopos B PID verifikacijai): Ross-Tech wiki/forumas, TDIClub, T5
  2.5 TDI bendruomenė — PRIEŠ naudojant bet kurį Mode 22 adresą realiuose duomenyse.
- **Papildomas Pakopos B tyrimas (2026-07-06):**
  - Torque Pro + „Torque Scan" papildinys — automatinis Mode 22 DID diapazono peržvelgimas
    konkrečiame automobilyje (rekomenduojamas praktinis žingsnis prieš spėliojant kodus):
    minimas TDIClub gijoje https://forums.tdiclub.com/showthread.php?t=430012
  - XGauge→Torque konversijos pavyzdys su alyvos temp DID `22 13 10`:
    http://forums.tdiclub.com/showthread.php?t=326055
  - TDIClub gijos su (dabar nebeveikiančia) „VW.csv" Torque custom PID rinkinio nuoroda —
    galimai atnaujinta prisijungusiems nariams:
    https://forums.tdiclub.com/showthread.php?t=415791 ,
    https://forums.tdiclub.com/showthread.php?t=456913 ,
    https://forums.tdiclub.com/showthread.php?t=430012
  - `bri3d/MQBSimosLogVariables` — atviras Mode 22 DID + konversijų projektas (MQB/Simos, ne
    mūsų EDC16, bet patvirtina „0x22 + DID" formato pagrįstumą kaip bendrą VAG konvenciją):
    https://github.com/bri3d/MQBSimosLogVariables
  - `ConnorHowell/vag-uds-ids` — VAG modulių (variklis/transmisija/ir kt.) CAN request/response
    ID sąrašas (naudinga Fazei 2, ne tik varikliui): https://github.com/ConnorHowell/vag-uds-ids
- **Pakopa C+ patikrinimas (2026-07-06):** `ATCS` (CAN Tx/Rx klaidų skaitikliai) ir flow
  control komandų (`ATCFC0/1`, `ATFCSH/SD/SM`) tikri pavadinimai patvirtinti prieš oficialų
  ELM327 datasheet: https://www.elmelectronics.com/wp-content/uploads/2016/07/ELM327DS.pdf
- **Service 21 (measuring blocks) patikrinimas (2026-07-06):** patvirtinta, kad KWP2000
  „Measuring Blocks" yra `ReadDataByLocalIdentifier` (Service `0x21`) su 1-baitu local
  identifier — realus, veikiantis atviro kodo pavyzdys (archyvuotas, beta, bet techniškai
  validus): https://github.com/jazdw/vag-blocks . Patvirtinta ir kad `0x85` NĖRA Service `0x10`
  subfunkcija, o atskiras UDS servisas („Control DTC Setting") — žr. bendrą UDS/ISO 14229
  serviso sąrašo apžvalgą (paieškos rezultatai šiame pokalbyje, pvz.
  https://piembsystech.com/control-dtc-setting-0x85-service-in-uds-protocol/).
- **Fazė 2 (viso automobilio duomenys) tyrimo šaltiniai (2026-07-06):**
  - `rnd-ash/mercedes-hacking-docs` — OBD-II filtravimo (read all / write diagnostic-only)
    faktas ir CAN tinklų (M-CAN/I-CAN/D-CAN/MOST) architektūra:
    https://github.com/rnd-ash/mercedes-hacking-docs/blob/master/Chapter%201%20Vehicle%20Bus%20Protocols%20and%20Diagnostics.md
  - To paties projekto pavyzdinis Mercedes UDS ID sąrašas (kitiems modeliams, ne W906) ir
    wake-up paketų lentelė W-serijos automobiliams:
    https://github.com/rnd-ash/mercedes-hacking-docs/blob/master/Chapter%203%20Connecting%20to%20the%20Vehicle.md
  - Sprinter-Source forumas — CAN magistralių fizinės vietos/jungtys (M-CAN/I-CAN/D-CAN/MOST
    keturi tinklai 2007-2010 modeliams, SAM CAN B jungtis):
    https://sprinter-source.com/forums/index.php?threads/74676/
  - Fleet Maintenance — Sprinter elektros sistemų apžvalga (EIS kaip M-CAN/I-CAN/D-CAN
    tarpininkas):
    https://www.fleetmaintenance.com/in-the-bay/diagnostic-and-repair/article/10329448/sprinter-electrical-systems
  - Vietinis įrankis pasyviam sniffing'ui (jau egzistuoja, kodo keisti nereikia):
    `research/obd2/ESPobd/src/main.cpp` (RAW CAN logavimas visiems ID)
