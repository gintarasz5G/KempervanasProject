# CLAUDE.md — Kempervanas ESPHome projektas

Šis failas yra pagrindinė instrukcijų bazė Claude Code agentui.
Perskaityk **viską** prieš pradedant bet kokį veiksmą.

---

## Projekto apžvalga

ESP32-S3 pagrindu veikianti kempervano automatizavimo sistema (VW Crafter 2008).
Pagrindinis kontroleris: **KC868-A16v3.1** (ESP32-S3)
WiFi AP: `Kemperis-Valdymas` / `kemperis123` → `192.168.4.1`

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

Projektas dabar yra **vienas git-valdomas repo**: `C:\Users\ginta\KempervanasProject`
(remote: github.com/gintarasz5G/KempervanasProject, „Gold Master v18").

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

## Hardware architektūra

### Magistralės

| Magistralė | Pinai | Įrenginiai |
|-----------|-------|-----------|
| I2C | SDA=GPIO9, SCL=GPIO10 | ADXL345 (0x53), VL53L0X ×1 (0x29), BMP180 (0x77), BME688 (0x76, SDO→GND), PCF8574 ×3 (0x22, 0x24, 0x25), SSD1306 OLED (0x3C, lizdas E) |
| UART0 | TX=GPIO40, RX=GPIO41 | A7670E GSM/GPS (115200 baud) |
| BLE | — | Junctek KH-F150A šuntas (akum. duomenys per Bluetooth, UUID FFF0/FFF1) |
| 1-Wire | GPIO39 | AM2301 temperatūra/drėgmė (atsarginė) |
| GPIO | GPIO47 (DOUT), GPIO48 (CLK) | HX711 (dujų baliono svoris) |
| ADC | GPIO7 | TDS vandens kokybės jutiklis |
| PCF8574 0x22/0x24/0x25 | — | 8 šviesos išėjimai (sviesa_1…sviesa_8) + 8 jungiklių įėjimai |

### Žinomi apribojimai ir kritinės pastabos

- **ADXL345**: Boot metu būtinas I2C write `{0x2D, 0x08}` (measurement mode) prieš nuskaitant duomenis.
- **BME688 adresas**: SDO → GND → adresas 0x76 (ne 0x77, kad nevyktų konfliktas su BMP180 0x77).
- **VL53L0X**: Vienas jutiklis, adresas 0x29. Nėra XSHUT logikos.
- **Junctek KH-F150A**: Jungiamas tiesiogiai prie šunto laidų (valdiklio dėžutė sudegė).
  ⚠️ Realiame firmware skaitomas per **BLE** (`ble_client`, ne RS485 Modbus). Renogy atskiro BLE
  sensoriaus firmware nėra — „Energija" duomenys ateina iš Junctek šunto.
- **Renogy DCC50S Bluetooth**: Turi integruotą BT. Reikia BLE client sprendimo — žr. tyrimą `research/`.
- **Renogy PRO 100Ah LiFePO4**: Turi integruotą BT. Reikia BLE client sprendimo.
- **HX711**: DOUT=GPIO47, CLK=GPIO48 — patikrink konfliktus.
- **A7670E**: 5V, max ~2A. UART TX=GPIO40, RX=GPIO41.
- **RS485**: AUTO-direction per MAX13487EESA — YAML **neturi** turėti `direction_pin`.
- **SMS koduotė**: Tik ASCII (GSM 7-bit) — **jokių lietuviškų raidžių** SMS tekstuose.
- **GPS odometras**: Minimalus greitis = 5 km/h — multipath apsauga.
- **No internet**: Tik WiFi AP, jokio STA/cloud.
- **BMP180 vs BMP085**: ESPHome naudoja `bmp085` komponentą — normalu.
- **220V aptikimas**: PCF8574 skaitmeniniai įėjimai → `power_source_220` / `shore_power_present` / `inverter_220_active`.
- **UV LED**: Valdomas atskiru krano jungikliu — **neintegruojamas** į šį projektą.
- **8 LED + 8 jungikliai**: OUT1–8 = šviesos (MOSFET sink). DIN1–8 = fiziniai jungikliai (optoisoliuoti, aktyvuojami GND). Kiekvienas jungiklis valdo atitinkamą šviesą bei vienas virtualus mygtukas išjungti visas šviesas.

---

## Reikalavimai kiekvienam jutikliui

> Tikslinė būsena. Claude Code tikrina ar YAML atitinka — jei ne, taiso ir pagrindžia.

### 1. GPS (A7670E — `+CGNSSINFO`)

**Rodo:**
- Koordinatės (lat/lon, 6 ženklai), aukštis (m), greitis (km/h), kryptis (°)
- GPS / GLONASS / BeiDou palydovų skaičius (naudojami / matomi)
- Fix statusas (text_sensor: "Nėra" / "2D" / "3D")

**Reikalavimai:**
- Atskiri `sensor:` kiekvienam parametrui
- `accuracy_decimals: 0` greičiui, krypčiai, aukščiui; `accuracy_decimals: 6` koordinatėms
- Jei nėra fix — paskutinė žinoma reikšmė arba NaN

### 2. Pokrypis (ADXL345)

**Rodo:**
- Šoninis K/D (°, sveikas, + = dešinė aukščiau)
- Išilginis P/G (°, sveikas, + = priekis aukščiau)
- Domkratų patarimas (text_sensor)

**Reikalavimai:**
- `accuracy_decimals: 0`
- Zero kalibravimas: `button:` → offsets į NVS (`restore_value: true`)
- **Domkratų logika**: `mm = tan(kampas_rad) × 1800` (VW Crafter ratų tarpas 1800mm)
  Pvz.: "Kelti K +42mm" / "Kelti D +18mm" / "Lygu" (< 0.5° = lygu)
- Roll/Pitch su `atan2` iš ADXL345 registrų 0x32

### 3. Dujų svoris (HX711)

**Rodo:** Likutis kg (1 ženklas), likutis %

**Reikalavimai:**
- `number:` tuščio baliono svoris (kg, NVS, numatyta 13.5)
- `number:` pilno baliono svoris (kg, NVS, numatyta 23.5)
- `number:` scale factor kalibravimui (NVS)
- `button:` tare (nustatyti nulį)
- Likutis = `max(0, išmatuotas − tuščias)`, procentai ribojami 0–100%

### 4. Temperatūra ir slėgis (BMP180)

**Rodo:** Temperatūra (°C), slėgis (hPa), aukštis (m), orų prognozė (text)

**Reikalavimai:**
- `number:` temp ofset'as (°C, −10..+10, žingsnis 0.1, NVS)
- Orų prognozė — 7 slėgio reikšmių istorija kas 30 min → 3h tendencija:
  - > +4 hPa/3h → "Labai greitai gerėja"
  - +2..+4 → "Geras"
  - +0.5..+2 → "Gerėja"
  - ±0.5 → "Stabilus"
  - −0.5..−2 → "Kinta"
  - −2..−4 → "Blogas"
  - < −4 hPa/3h → "Smarkiai blogėja"
- Atnaujinimas kas 30 min

### 5. Oro kokybė (BME688)

**Rodo:** Temperatūra (°C), drėgmė (%), slėgis (hPa), VOC (Ω), oro kokybė (text)

**Reikalavimai:**
- I2C adresas: 0x76 (SDO → GND)
- ESPHome komponentas: `bme680` arba `bme68x` — patikrink https://esphome.io/components/sensor/bme680.html
- Jei reikia BSEC — naudoti `bme680_bsec`
- `number:` temp ofset'as (NVS, numatyta −4°C — BME688 šyla nuo savęs)
- Oro kokybė text: "Geras" / "Vidutinis" / "Blogas" pagal VOC reikšmę

### 6. GSM ryšys (A7670E)

**Rodo:** Operatorius, RSSI (dBm), signalas %, registracijos statusas

**Reikalavimai:**
- AT: `AT+COPS?`, `AT+CSQ`, `AT+CREG?` — atnaujinimas kas 60s
- **SMS atsakymas** į "Status?" (ASCII, max 160 simbolių):
  ```
  Kemperis OK
  Bat: {V}V {%}%
  GPS: {lat},{lon} {alt}m
  Tilt: {roll}d {pitch}d
  Gas: {kg}kg H2O:{%}%
  Temp:{C}C {hPa}hPa
  ```

### 7. Vandens bakas (VL53L0X)

**Rodo:** Atstumas cm (raw), lygis %, litrai

**Reikalavimai:**
- `number:` tuščio bako cm (NVS), pilno bako cm (NVS), tūris litrais (NVS)
- `button:` "Nustatyti kaip tuščią" ir "Nustatyti kaip pilną"
- Procentai = `(tuščias − dabartinis) / (tuščias − pilnas) × 100`
- `median` filtras window 5, ribojama 0–100%

### 8. Akumuliatorius (Junctek KH-F150A šuntas — BLE; planuota Renogy PRO LiFePO4)

**Rodo:** Įtampa (V), srovė (A), galia (W), SOC %, Ah, temperatūra (°C), likęs veikimo laikas

**Reikalavimai:**
- Realiai duomenys ateina iš **Junctek šunto per BLE** (`ble_client`).
- Renogy PRO LiFePO4 BLE — tyrimo etapas (`research/`), žr. cyrils/renogy-bt protokolą.

### 9. Saulės kroviklis (Renogy DCC50S — Bluetooth)

**Rodo:** Saulės įtampa/srovė/galia, akum įtampa, krovimo statusas, režimas, kraunama iš saulės, kraunama iš variklio

**Reikalavimai:**
- ⚠️ **Reikia tyrimo** (`research/`):
  1. Patikrink ar KC868 ESP32-S3 palaiko BLE
  2. cyrils/renogy-bt protokolas — ar tinka LiFePO4 ir DCC50S?
  3. Ieškok: https://github.com/search?q=renogy+esphome+ble
  4. Alternatyva: atskiras ESP32 su BT → MQTT → KC868 per WiFi
  5. Jei nerealus šiame etape — dokumentuoti ir atidėti

### 10. Šviesos ir jungikliai

**Valdymas:** 8 šviesos (sviesa_1…sviesa_8), 8 fiziniai jungikliai (jungiklis_1…jungiklis_8)

**Reikalavimai:**
- Kiekvienas jungiklis_N sinchronizuotas su sviesa_N
- OUT išėjimai: MOSFET sink — `inverted: true` jei reikia
- DIN įėjimai: aktyvuojami GND (KCOM = teigiama pusė)

---

## Dashboard grupavimas (web_server UI)

ESPHome `web_server` rodo sensorius abėcėlės tvarka pagal `name:`.
Grupavimui naudoti **prefiksus su skyrikliu `Grupė |`** — tada grupės rikiuojasi tvarkingai.

> ⚠️ **SVARBU app susiejimui:** ESPHome `name` → SSE `object_id` slug paverčiamas pakeičiant
> kiekvieną neleistiną simbolį `_` (ne pašalina). Todėl `" | "` → `___` (trigubas pabraukimas),
> pvz. `"GPS | Greitis"` → `gps___greitis`. App (`android/www/index.html`) SSE grandinė remiasi
> šiais slug'ais. **Pervadinus jutiklį YAML'e — reikšmė app tyliai dingsta.** Žr. naujausią
> `docs/audits/` SSE mapping auditą.

**Privaloma naudoti šiuos prefiksus:**

| Prefiksas | Grupė |
|-----------|-------|
| `GPS \|` | Palydovai, koordinatės, aukštis, greitis, kryptis |
| `Pokrypis \|` | Šoninis, išilginis, domkratų patarimas, zero mygtukas |
| `Energija \|` | Akumuliatorius ir saulės kroviklis |
| `Aplinka \|` | BME688 + BMP180 + AM2301 duomenys |
| `Resursai \|` | Vanduo + dujos su kalibravimo mygtukais |
| `GSM \|` | Operatorius, signalas, statusas |
| `Miegamasis \|` | SCD4x CO2, temperatūra, drėgmė |
| `Sistema \|` | RAM, ESP temp, uptime, patarimai, buferis |
| `Šviesos \|` | sviesa_1…8 ir jungiklis_1…8 |

---

## Kodo konvencijos

- Komentarai YAML — lietuvių kalba
- `name:` — lietuvių kalba su grupės prefiksu (pvz. `"GPS | Greitis"`)
- SMS — tik ASCII
- `restore_value: true` — visi kalibraciniai parametrai
- `accuracy_decimals: 0` — greitis, pokrypis, kryptis, aukštis, %
- `accuracy_decimals: 1` — temperatūra, slėgis, svoris, litrai
- `accuracy_decimals: 2` — įtampa, srovė
- `accuracy_decimals: 6` — GPS koordinatės
- Visi `number:` ir `button:` turi unikalų `id:`
- Nekeisti `web_server:` — SSE veikia

---

## Ateities etapai (dar nevykdyti)

### Etapas C — MQ dujų jutiklis (ADC A1)
Dujų nuotėkio aliarmas — integruoti kai BMP180/BME688 bus stabilūs.

### Etapas D — Google Sheets logavimas
Per A7670E 4G, kas 15 min. Kritiniai įvykiai iš karto. 30 dienų langas (Apps Script auto-delete).

### Etapas E — OBD2/CAN (atskiras Waveshare ESP32-S3-RS485-CAN)
M-CAN 500kbps: RPM, temp, greitis, turbo.

---

## Žinynas ir paieškos šaltiniai

Claude Code: prieš siūlant bet kokį sprendimą — patikrink visus aktualius šaltinius.
Nesiremk tik vienu šaltiniu. Palygink sprendimus ir pasirink geriausią.

**ESPHome:**
- Komponentai: https://esphome.io/components/
- BMP085/180: https://esphome.io/components/sensor/bmp085.html
- BME680/688: https://esphome.io/components/sensor/bme680.html
- VL53L0X: https://esphome.io/components/sensor/vl53l0x.html
- HX711: https://esphome.io/components/sensor/hx711.html
- ADXL345: https://esphome.io/components/sensor/adxl345.html
- BLE Client: https://esphome.io/components/ble_client.html
- Issues: https://github.com/esphome/esphome/issues · Discussions: https://github.com/esphome/esphome/discussions

**Renogy BT:**
- cyrils/renogy-bt: https://github.com/cyrils/renogy-bt (PROTOCOL.md)
- ESPHome paieška: https://github.com/search?q=renogy+esphome+ble

**Hardware:**
- KC868-A16v3: https://kincony.com/kc868-a16-v3.html
- A7670E / SIM7670 AT komandos: ieškoti "SIM7670 AT Command Manual PDF"
- ADXL345 datasheet: https://www.analog.com/media/en/technical-documentation/data-sheets/ADXL345.pdf
