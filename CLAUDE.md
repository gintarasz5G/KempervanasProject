# CLAUDE.md — Kempervanas ESPHome projektas

Šis failas yra pagrindinė instrukcijų bazė Claude Code agentui.
Perskaityk **viską** prieš pradedant bet kokį veiksmą.

> Sinchronizuota su firmware 2026-07-02 (v32.3, commit 8d19a1d). Šis failas aprašo
> **faktinę** sistemos būseną; neįgyvendinti planai — tik skyriuje „Ateities etapai".

---

## Projekto apžvalga

ESP32-S3 pagrindu veikianti kempervano automatizavimo sistema (VW Crafter 2008).
Pagrindinis kontroleris: **KC868-A16v3.1** (ESP32-S3)
WiFi AP: `Kemperis-Valdymas` / `kemperis123` → `192.168.4.1`
Firmware: monitoringas + SMS valdymas + vagystės apsauga + Google Sheets logeris per 4G.
**Apkrovų valdymo (šviesų ir kt.) firmware kol kas NĖRA** — žr. „Ateities etapai".

### 🎯 Autoritetingi failai

> Visi keliai RELATYVŪS repo šakniai: `C:\Users\ginta\KempervanasProject\`

| Komponentas | Failas (repo viduje) |
|---|---|
| **ESPHome firmware** | `firmware/kempervanas.yaml` (paslaptys: `firmware/secrets.yaml`) |
| **Google Apps Script** | `google-sheets/GoogleSheetsScript.js` |
| **Android app frontend** | `android/www/index.html` |
| **Android native bridges** | `android/android/app/src/main/java/lt/kemperis/app/{MainActivity,KemperisService}.java` |
| **Hardware / dalių sąrašas** | `hardware/` (`kempervanas_daliu_sarasas.xlsx`, `schemas/`, `3d/`) |
| **Dokumentacija ir auditai** | `docs/` ir `docs/audits/` |

### ⚠️ Aplankų struktūra (vienas git monorepo)

Projektas yra **vienas git-valdomas repo**: `C:\Users\ginta\KempervanasProject`
(remote: github.com/gintarasz5G/KempervanasProject).

| Aplankas | Turinys |
|---|---|
| `firmware/` | ESPHome (`kempervanas.yaml`, `secrets.yaml`) + `archive/` senos versijos |
| `android/` | Capacitor app: `www/index.html` (frontend), `android/...` (native Java), APK |
| `google-sheets/` | `GoogleSheetsScript.js` (Apps Script debesų logika) |
| `hardware/` | Schemos, jungtys, dalių sąrašas, 3D |
| `docs/` | Dokumentacija ir `audits/` |
| `research/` | Renogy BLE ir kt. tyrimai |
| `release/`, `secrets/`, `tools/`, `data/` | Build artefaktai, paslaptys, įrankiai |

> ⚠️ **PASENĘ — NEBENAUDOTI:** `C:\Users\ginta\KemperosProjektas\ESPHOME\`,
> `C:\Users\ginta\kempervanas\veikiantisKEMPERIS\`, `C:\Users\ginta\kempervanas\CLAUDE\`.
> Tai senos lygiagrečios kopijos (turinys perkeltas į `firmware/archive/` ir `docs/`).
> **Analizuok ir redaguok TIK `C:\Users\ginta\KempervanasProject` failus.**

---

## Build & Flash komandos

> **Vykdyk iš** `firmware/` aplanko. Paslaptys — `firmware/secrets.yaml` (pvz. `app_script_id`).

```powershell
# Validacija (be flash'inimo)
esphome config kempervanas.yaml

# Kompiliacija
esphome compile kempervanas.yaml

# Flash per USB-C
esphome run kempervanas.yaml

# OTA (įrenginys turi būti tinkle)
esphome run kempervanas.yaml --device 192.168.4.1

# Logai realiuoju laiku
esphome logs kempervanas.yaml
```

---

## Hardware architektūra (faktinė)

### Magistralės ir pinai

| Magistralė | Pinai | Įrenginiai |
|-----------|-------|-----------|
| I2C (`i2c_bus`, 100 kHz) | SDA=GPIO9, SCL=GPIO10 | ADXL345 (0x53, skaitomas raw I2C lambda), VL53L0X (0x29), **BME680 (0x77)**, SCD41 (0x62) |
| UART (`uart_a7670e`) | TX=GPIO40, RX=GPIO41, 115200 | A7670E GSM/GPS (SMS, HTTP, CGNSSINFO, CLBS) |
| BLE (`ble_client`) | — | Junctek KH-F150A šuntas (MAC `38:3B:26:79:DB:64`, service FFF0 / char FFF1, notify) |
| GPIO | DOUT=GPIO47, CLK=GPIO48 | HX711 (dujų baliono svoris) |
| ADC | **GPIO6** | TDS vandens kokybės jutiklis |
| ADC | GPIO7 (CH3), GPIO5 (CH4) | 220V aptikimas: CH3 LOW (<1.5 V) = **yra** išorinė 220V (apversta logika); CH3 HIGH + CH4 HIGH = keitiklis veikia |

> **NEPRIJUNGTA / firmware nėra:** BMP180, PCF8574 plėtikliai, SSD1306 OLED, AM2301,
> šviesų išėjimai/jungikliai, RS485. Jei jų prireiks — žr. „Ateities etapai".

### Žinomi apribojimai ir kritinės pastabos

- **ADXL345**: `on_boot` I2C init — `{0x31,0x0B}` (±16g), `{0x2C,0x0A}` (100 Hz), `{0x2D,0x08}` (measurement). Skaitoma raw per `i2c_bus` lambda kas 1 s (ne ESPHome komponentu). Sensorius ant **kairės vertikalios sienos**, Y → priekis: roll=`atan2(z,x)`, pitch=`atan2(y,x)`. Dinaminis dažnis: 100 Hz važiuojant, 12.5 Hz stovint.
- **BME680**: adresas **0x77** (vienintelis aplinkos jutiklis — BMP180 nėra, konflikto nebeliko). Temp/drėgmės ofsetai per `num_temp_offset`/`num_humid_offset`.
- **SCD41**: 0x62, miegamojo CO2/temp/drėgmė, `automatic_self_calibration: false`.
- **VL53L0X**: 0x29, be XSHUT. Filtrai: ×100 į cm + offset, median 3, spike >2 cm atmetamas.
- **Junctek KH-F150A**: jungiamas tiesiogiai prie šunto laidų (valdiklio dėžutė sudegė). Skaitomas per **BLE notify**, savas BCD parseris su paketo defragmentacija (paketas gali skilti per 2 notifikacijas). Žymekliai: C0=V, C1=A (D1=kryptis), D2=Ah likutis, D3=Total Ah, D6=likęs laikas, D8=W, D9=temp (val−100). Checksum netikrinamas — tik ribų filtrai. SOC skaičiuojamas: `Ah likutis / num_bat_capacity × 100`.
- **BLE watchdog**: jei Junctek tyli >10 min stovint ir paskutinė V >12 V — ESP restart (max 3 kartus, `ble_reboot_count` NVS).
- **A7670E**: 5 V, max ~2 A. Vienas UART dalinamas SMS/HTTP/GPS/CLBS — **`modem_busy` semaforas privalomas** kiekvienai AT sekai; watchdog atlaisvina po 90 s. Init: ATE0, CGNSSPWR=1, CMGF=1, CNMI=2,2, CGDCONT su `txt_apn`, CSSLCFG.
- **SMS koduotė**: tik ASCII (GSM 7-bit) — **jokių lietuviškų raidžių** SMS tekstuose.
- **⚠️ SAUGA (žinoma P0 skola, žr. `docs/audits/projekto_auditas_2026-07-02_rinkos_palyginimas.md`):** SMS komandos nevalidyja siuntėjo (`sms_sender` naudojamas tik istorijai); OTA be slaptažodžio; API be encryption key; web_server be auth; Apps Script be token'o.
- **No internet ESP'ui lokaliai**: WiFi tik AP režimu (app jungiasi tiesiogiai); internetas — tik per A7670E 4G (Google Sheets, SMS).
- **UV LED**: valdomas atskiru krano jungikliu — **neintegruojamas** į šį projektą.
- **RAM sargas**: kas 1 h; jei laisva <50 kB — suplanuotas naktinis perkrovimas 03:00 (pagal GPS laiką; fallback po 7 d.).
- **Nekeisti `web_server:`** (port 80, version 3, local) — SSE veikia, app nuo jo priklauso.

---

## Faktinė jutiklių ir logikos būsena

> Aprašo, kas įgyvendinta. Keisdamas — išlaikyk elgseną, nebent užduotis sako kitaip.

### 1. GPS (A7670E — `+CGNSSINFO`, kas 10 s kai GPS budi)

- Koordinatės (text_sensor, 6 ženklai), aukštis (0 dec), greitis (**1 dec**), kryptis (0 dec).
- Palydovai atskirai: NAVSTAR / GLONASS / BeiDou / **Galileo** + suma.
- Fix statusas (text_sensor): „Nėra" / „2D" / „3D".
- Koordinačių konverteris palaiko NMEA DDMM.MMM ir Decimal Degrees.
- Be fix — sensoriai NaN; `last_known_lat/lon` (NVS) naudojami SMS/logeriui.
- Laikas iš GPS + `num_timezone_offset` (NVS, numatyta +3) → `gps_time_str`.
- **Galios valdymas**: po 30 min stovėjimo GPS užmigdomas (`AT+CGNSSPWR=0`, ~70 mA); žadina vibracija, srovė >15 A, SMS, mygtukas `btn_wake_up`, 4 h heartbeat pre-wake (~7 min prieš) ir `+lokacija`. Pažadinus „gal važiuoja" — GPS pats patvirtina per 8 min arba vėl migdomas. Miegas taip pat kai SOC <30 % ir stovima >2 h.
- GPS recovery: jei nėra fix 600 s — CGNSSPWR ciklas.

### 2. Pokrypis (ADXL345)

- `Pokrypis | Soninis K-D` (roll) ir `Pokrypis | Isilginis P-G` (pitch), publikuojama 0.1° žingsniu.
- Filtrai: spike >8°/s atmetamas; adaptyvi EMA (α=0.6 kai pokytis >1.5°, kitaip 0.15); ribos ±25°/±20°.
- Zero kalibravimas: `btn_adxl_zero` → `cal_roll`/`cal_pitch` (NVS) + filtrų resetas; rankinis — `num_roll_cal`/`num_pitch_cal`.
- `Pokrypis | Vibracija (EMA)` — Manhattan delta, naudojama judėjimo aptikimui (slenkstis `num_vib_threshold`, NVS, ~35).
- **Domkratų patarimas skaičiuojamas APP'e** (`index.html` ~1808–1827: `tan(kampas)×bazė` kiekvienam kampui), naudoja `num_wheelbase_cm` (350) ir `num_track_cm` (180) iš firmware NVS. Firmware text_sensor tam neturi.

### 3. Dujų svoris (HX711, GPIO47/48)

- `Resursai | Dujos likutis %` = `(bendras − tuščias) / (pilnas − tuščias) × 100`, ribojama 0–100.
- Kalibracija (visi NVS): `num_gas_empty_kg` (numatyta **2.8**), `num_gas_full_kg` (**7.8**), `num_hx711_scale` (20000), `num_gas_weight_offset`, `btn_gas_tare`.
- Filtrai: median 3 + EMA 0.85/0.15, spike >10000 raw (~0.5 kg) atmetamas.
- Kg sensoriaus nėra — tik %.

### 4. Aplinka (BME680, 0x77, kas 30 s)

- Temperatūra (+ ofsetas), drėgmė (+ ofsetas, ribos 0–100), slėgis (600–1100 hPa filtras).
- `Aplinka | Slegio tendencija` — 7 reikšmių istorija kas 30 min (`pressure_history`), delta hPa.
- `Aplinka | Aukstis (slegis)` — barometrinis aukštis.
- `Aplinka | Oru prognoze` (tekstai pagal tendenciją): >2 „Saulėta ☀️" / >0.5 „Gerėja ↗" / ±0.5 „Stabilu →" / ≥−2 „Kinta ↘" / kitaip „Audringa ⛈️"; +„(Drėgna)" kai drėgmė >75 %.
- VOC/oro kokybės iš BME680 nėra; oro kokybės tekstas — iš SCD41 CO2 (žr. 8).

### 5. Vanduo (VL53L0X + TDS)

- `Resursai | Vandens atstumas (lazeris)` cm; `Resursai | Vanduo lygis %` = `(tuščias − d) / (tuščias − pilnas) × 100`.
- Kalibracija (NVS): `num_water_empty_cm` (30), `num_water_full_cm` (5), `num_water_offset`; mygtukai `btn_water_empty` / `btn_water_full`.
- Litrų sensoriaus nėra.
- `Resursai | Vandens TDS (ppm)` — ADC GPIO6, temperatūrinė kompensacija pagal BME temp.

### 6. GSM (A7670E)

- CSQ kas 10 s; COPS (operatorius + tinklas iš MCC/MNC: 24601 Telia, 24602 Bitė, 24603 Tele2, 24605 Mezon) ir CREG (roaming statusas) kas 5 min.
- Sensoriai: dBm, %, operatorius, tinklas, roaming.
- **SMS komandos** (prefiksas `+`, ieškoma substring'u, lowercase): `+lokacija`, `+status`/`+info`/`+?`, `+ikelk`/`+upload`, `+start google`, `+stop google`, `+arm`/`+ijungti`, `+disarm`/`+isjungti`, `+apsauga`, `+komandos`/`+help`/`+pagalba`. Rate limit 10 s. Gautas SMS pažadina GPS iš miego. Istorija — 5 text_sensor (`Gauta SMS 1..5`), valymas `btn_clear_sms`.
- SMS statusas (faktinis formatas): `Apsauga: IJUNGTA/ISJUNGTA \n SOC:..% U:..V Van:..% Dujos:..% Temp:..C Oras:..`.
- `+lokacija`: jei GPS miega — pažadina; be fix siunčia seną vietą + follow-up SMS kai pagauna fix (5 min langas).
- Atsakymai siunčiami tik į `txt_admin_number`; **bet komandos vykdomos iš bet kokio siuntėjo — žr. P0 skolą.**

### 7. Energija (Junctek BLE)

- `Energija |`: įtampa (2 dec), srovė (2 dec, ženklas iš D1), galia, Ah likutis, Total Ah, temperatūra, likęs laikas (min), SOC % (skaičiuojamas iš Ah / `num_bat_capacity`, numatyta 110 Ah).
- 220V: `Energija | Isorine 220V`, `Energija | Keitiklio 220V` (binary), `Energija | 220V saltinis` (text) — iš ADC GPIO7/GPIO5, delayed_on/off 3 s.

### 8. Miegamasis (SCD41)

- CO2 ppm, temperatūra, drėgmė (kas 30 s); `CO2 tendencija` ppm/2 min.
- `Miegamasis | Oro kokybe`: <800 „Puiki" / <1200 „Gera" / <1800 „Vidutine" / <2500 „Bloga" / kitaip „PAVOJINGA".
- `Sistema | Patarimai` — apjungia CO2 + drėgmę + slėgio tendenciją + kondensato riziką.

### 9. Sauga (vagystės apsauga)

- Įjungiama `sw_security_armed` arba SMS `+arm`.
- **ADXL aliarmas**: stovint, kai roll/pitch nukrypsta >3° nuo `park_ref` — SMS su koordinatėmis + sekimo režimas (log kas 30 s, upload iš karto, GPS žadinamas).
- **GSM trianguliacija** (`AT+CLBS`): pirmas taškas po 30 s stovėjimo, tikrinimas kas 30 min; jei poslinkis >800 m (ir tikslumas <800 m) — SMS aliarmas + įrašas.
- Aliarmų vėliavos atsistato po perkrovimo ir išjungus apsaugą.

### 10. Hibridinis logeris (Google Sheets per A7670E HTTP)

- **Važiuojant**: įrašas kai pajudėta >20 m (Haversine) IR (kryptis pasikeitė >25° ARBA praėjo 60 s ARBA SOC Δ≥1 %). Sekimo režime — 5° / 30 s.
- **Stovint**: tik kritiniai įvykiai (SOC ≤25 %, vanduo ≤10 %, dujos ≤10 %, CO2 >2500 ppm 2 iš 3 matavimų — su SMS) + 4 h heartbeat. Vanduo/dujos stovint imami iš „stabilių" reikšmių (po 60 s nusistovėjimo).
- Buferiai: `data_batch` → `sending_batch` (max ~9–10 kB, perpildžius retinama kas antra eilutė). Eilučių validacija/atstatymas prieš siuntimą (AT echo apsauga).
- POST į `https://script.google.com/macros/s/<txt_app_script_id>/exec`; sėkmė = HTTP 200 **arba 302** (Apps Script redirect). Buferis valomas tik po patvirtintos sėkmės; po 10 klaidų iš eilės — išmetamas. Jungiklis `sw_google_upload`; priverstinis — `btn_force_send_google` arba SMS `+ikelk`.
- CSV eilutės formatas (22 laukai): `ts,,lat,lon,alt,speed,heading,soc,V,A,water%,tds,gas%,temp,hum,hPa,co2,bed_temp,bed_hum,W,Ah,uptime_min;` — stulpelis 1 tuščias (Apps Script įrašo žemėlapio HYPERLINK). Keisdamas formatą — sinchronizuok su `GoogleSheetsScript.js` stulpelių žemėlapiu ir app `_fetchSheetsNow`.

---

## Dashboard grupavimas (web_server UI)

ESPHome `web_server` rodo sensorius abėcėlės tvarka pagal `name:`.
Grupavimui naudojami **prefiksai su skyrikliu `Grupė |`**.

> ⚠️ **SVARBU app susiejimui:** ESPHome `name` → SSE `object_id` slug paverčiamas pakeičiant
> kiekvieną neleistiną simbolį `_` (ne pašalina). Todėl `" | "` → `___` (trigubas pabraukimas),
> pvz. `"GPS | Greitis"` → `gps___greitis`. App (`android/www/index.html`) SSE grandinė remiasi
> šiais slug'ais. **Pervadinus jutiklį YAML'e — reikšmė app tyliai dingsta.** Žr. naujausią
> `docs/audits/` SSE mapping auditą.

**Naudojami prefiksai (faktiniai):**

| Prefiksas | Grupė |
|-----------|-------|
| `GPS \|` | Palydovai, koordinatės, aukštis, greitis, kryptis, fix statusas |
| `Pokrypis \|` | Šoninis, išilginis, vibracija |
| `Energija \|` | Junctek šuntas + 220V šaltinio aptikimas |
| `Aplinka \|` | BME680: temp, drėgmė, slėgis, tendencija, aukštis, orų prognozė |
| `Resursai \|` | Vanduo (lazeris, %, TDS) + dujos % |
| `GSM \|` | Operatorius, tinklas, signalas, roaming |
| `Miegamasis \|` | SCD41: CO2, temp, drėgmė, tendencija, oro kokybė |
| `Sistema \|` | RAM, ESP temp, uptime, patarimai, Google buferis, modemo būsena, klaidos |
| `Sauga \|` | Judėjimo aliarmo statusas |

Valdymo entity'ės (be grupės prefikso, vardas = id stilius): `txt_app_script_id`,
`txt_admin_number`, `txt_kemperis_number`, `txt_apn`; `num_*` kalibracijos;
`btn_*` mygtukai; `sw_debug_log`, `sw_google_upload`, `sw_security_armed`;
`Gauta SMS 1..5`, `Isvalyti SMS istorija`. **App jas pasiekia pagal šiuos vardus — nepervadinti.**

---

## Kodo konvencijos

- Komentarai YAML — lietuvių kalba.
- Rodomų jutiklių `name:` — lietuvių kalba su grupės prefiksu (pvz. `"GPS | Greitis"`); valdymo entity'ių `name:` = id stilius (`num_...`, `btn_...`, `sw_...`, `txt_...`).
- SMS — tik ASCII.
- `restore_value: true` — visi kalibraciniai parametrai (globals + NVS).
- `accuracy_decimals: 0` — pokrypis, kryptis, aukštis, %, ppm; `1` — temperatūra, slėgis, GPS greitis; `2` — įtampa, srovė; koordinatės — text_sensor su 6 ženklais.
- Visi `number:`/`button:`/`switch:`/`text:` turi unikalų `id:`.
- Kiekviena AT seka į modemą — per `modem_busy` semaforą (`wait_until` + timeout); SMS skriptai `mode: queued`.
- Nekeisti `web_server:` — SSE veikia.
- Prieš commit: `esphome config kempervanas.yaml` privalo praeiti.

---

## Ateities etapai (dar nevykdyti)

### Etapas A — Saugumo P0 (pirmiausias!)
SMS siuntėjo autorizacija; `ota.password` + `api.encryption.key`; web_server auth;
Apps Script token. Žr. `docs/audits/projekto_auditas_2026-07-02_rinkos_palyginimas.md`.

### Etapas B — Šviesos ir jungikliai (KC868-A16v3 borto I/O)
Išėjimai: P-kanalo MOSFET **NCE60P10K (high-side, komutuoja +)** — akum + per saugiklį į banko
„+" įvadą, OUT → šviestuvo +, šviestuvo − į masę. Banko srovės riba ~8–10 A (KCOM gnybtas/takeliai),
ne 10 A × 8. Varikliams (siurblys, šaldytuvas) — per 12 V relę.
Įėjimai: dry contact su optoizoliacija — jungiklis tarp GND/COM ir IN, tik signalinis laidas.
Fiziniai jungikliai — toggle tipo: firmware logika `on_state` → `light.toggle` (būsenos pokytis,
ne absoliuti padėtis). Kiekvienas jungiklis_N valdo sviesa_N + virtualus „išjungti visas" mygtukas.
Grupės prefiksas `Šviesos |`. Laidai dar neprijungti; bent vienai šviesai palikti bypass jungiklį.

### Etapas C — MQ dujų jutiklis (ADC)
Dujų nuotėkio aliarmas. Laisvo ADC kanalo parinkimas — patikrinti konfliktus (GPIO5/6/7 užimti).

### Etapas D — Renogy BLE (DCC50S + Pro 100Ah LiFePO4)
Įranga: DCC50S (`RBC50D1S-BT`) su **BT-2 moduliu** komplekte (`BT-TH-xxxx`);
baterija Pro (`RBT12100LFP-BT`) su integruotu BT ir self-heating.
**1 žingsnis ĮGYVENDINTAS (app v38 / v33.2, 2026-07-02): app skaito tiesiogiai per
planšetės BT** — `android/www/renogy.js` + `capacitor.js` + `ble-plugin.js`;
žr. `docs/uzduotis_renogy_ble_tab.md` ir `docs/audits/auditas_2026-07-02_vakaras_v38.md`
(likę defektai R1–R5). 2 žingsnis (jei prireiks SMS/Sheets):
per ESP `ble_client` (RAM rizika!) arba atskiras ESP32 → ESP-NOW/MQTT.
Protokolas — cyrils/renogy-bt; vietinis tyrimas `research/renogy-dcdc/renogy.yaml`.

### Etapas E — OBD2/CAN (atskiras Waveshare ESP32-S3-RS485-CAN)
M-CAN 500 kbps: RPM, temp, greitis, turbo.

### Kita svarstytina
- Domkratų patarimo text_sensor firmware'e (į web UI ir SMS; app jau skaičiuoja pats).
- Apps Script 30 d. auto-valymas (buvo planuota; vartotojas prašė Apps Script kol kas neliesti).
- Papildoma įranga iš senų planų (SSD1306 OLED, AM2301, BMP180) — neprijungta; integruoti tik jei fiziškai sumontuojama.

---

## Žinynas ir paieškos šaltiniai

Claude Code: prieš siūlant bet kokį sprendimą — patikrink visus aktualius šaltinius.
Nesiremk tik vienu šaltiniu. Palygink sprendimus ir pasirink geriausią.

**ESPHome:**
- Komponentai: https://esphome.io/components/
- BME680: https://esphome.io/components/sensor/bme680.html
- SCD4x: https://esphome.io/components/sensor/scd4x.html
- VL53L0X: https://esphome.io/components/sensor/vl53l0x.html
- HX711: https://esphome.io/components/sensor/hx711.html
- BLE Client: https://esphome.io/components/ble_client.html
- PCF8574 (Etapui B): https://esphome.io/components/pcf8574.html
- Issues: https://github.com/esphome/esphome/issues · Discussions: https://github.com/esphome/esphome/discussions

**Junctek / Renogy BT:**
- cyrils/renogy-bt: https://github.com/cyrils/renogy-bt (PROTOCOL.md)
- Junctek ESPHome bendruomenės komponentai: https://github.com/Sleeper85/esphome-junctek_khf · https://github.com/gianfrdp/esphome-junctek_khf
- ESPHome paieška: https://github.com/search?q=renogy+esphome+ble

**Hardware:**
- KC868-A16v3: https://kincony.com/kc868-a16-v3.html
- A7670E / SIM7670 AT komandos: ieškoti "SIM7670 AT Command Manual PDF"
- ADXL345 datasheet: https://www.analog.com/media/en/technical-documentation/data-sheets/ADXL345.pdf
