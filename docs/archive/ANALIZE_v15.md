# Analizė: kempervanas v15.4 → v16 REFACTORING

**Data:** 2026-05-08  
**Originalus failas:** kempervanas_v15.4.yaml  
**Atnaujinta versija:** kempervanas_v15.yaml (v16 commit'e)  
**Statusas:** ✅ REFACTORING BAIGTAS — kompiliuojasi su specifikacija atnaujinta

---

## GPIO žemėlapis & konfliktų patikrinimas

### Naudojami GPIO pinai

| Pinai | Komponentas | Statusas | Pastabos |
|-------|-----------|---------|----------|
| GPIO9 / GPIO10 | I2C SDA/SCL | ✅ | Frequency 100kHz |
| GPIO40 / GPIO41 | UART A7670E | ✅ | 115200 baud, RX buffer 2048 |
| GPIO47 / GPIO48 | HX711 DOUT/CLK | ✅ | Šiuose pinaose nėra konfliktų |
| 0x53 (I2C) | ADXL345 | ✅ | Boot write: 0x2D ← 0x08 |
| 0x29 (I2C) | VL53L0X | ✅ | Adresas teisingas |
| 0x77 (I2C) | BME680 | ⚠️ | Pakeista iš 0x76; BMP180 nėra — nėra konflikto |

### I2C adresos konfliktų analizė

- **0x53**: ADXL345 ✅
- **0x29**: VL53L0X ✅
- **0x77**: BME680 ✅ (bet spec sako 0x76 — žr. žemiau)
- **0x76**: BMP180 — **NĖRA ŠIAME FAILE** ❌
- **0x3C**: SSD1306 OLED — **NĖRA ŠIAME FAILE** ❌
- **PCF8574 0x21, 0x22, 0x24, 0x25** — žr. žemiau

### PCF8574 adresų problema ⚠️

**Šaltinis:** CLAUDE.md specifikacija

```yaml
pcf8574:
  - id: pcf_out1
    address: 0x24  ✅
  - id: pcf_out2
    address: 0x25  ✅
  - id: pcf_in1
    address: 0x21  ❌ TURĖTŲ BŪTI PAŠALINTAS
  - id: pcf_in2
    address: 0x22  ✅
```

**Problema:** CLAUDE.md aiškiai sako:
> "220V aptikimas: Iškeltas į **atskirą ESP32** — PCF8574 0x21 šiame projekte nenaudojamas."

**Rekomendacija:** Pašalinti `pcf_in1` (0x21) iš konfigūracijos.

---

## Komponentų patikrinimas pagal CLAUDE.md

### ✅ VEIKIA

#### 1. **ADXL345** (Pokrypis)
- **Status:** Pilnai implementuotas
- **I2C adresas:** 0x53 ✅
- **Boot inicijalizacija:** 0x2D ← 0x08 (lines 17-18) ✅
- **Sensoriai:**
  - `roll_sensor` (šoninis K-D) — lines 283-295 ✅
  - `pitch_sensor` (išilginis P-G) — lines 297-310 ✅
- **Kalibracija:**
  - `cal_roll`, `cal_pitch` (NVS) ✅
  - Kalibracinė akristis (button) — line 186 ✅
  - Zero offset skaitymas per lambda ✅
- **Accuracy:** `accuracy_decimals: 0` ✅
- **Nuskaitymo dažnis:** 1s ✅

#### 2. **VL53L0X** (Vandens bakas)
- **Status:** Implementuotas
- **I2C adresas:** 0x29 ✅
- **Nuskaitymas:**
  - Raw distance — lines 331-336 ✅
  - Filter: multiply 100 (gauna cm) ✅
  - Procentinis skaičiavimas — lines 338-348 ✅
- **Kalibracija:**
  - `water_empty_cm` (30.0 cm) ✅
  - `water_full_cm` (5.0 cm) ✅
  - `water_total_liters` (100 L) ✅
- **Median filtras:** ❌ Nėra — dokumentavimas sako reikia `window: 5`

**Rekomendacija:** Pridėti median filtro:
```yaml
filters:
  - median: { window_size: 5 }
  - multiply: 100
```

#### 3. **HX711** (Dujų baliono svoris)
- **Status:** Implementuotas
- **GPIO:** 47 (DOUT), 48 (CLK) ✅
- **Nuskaitymo dažnis:** 5s ✅
- **Kalibracija:**
  - `hx711_tare` (NVS) ✅
  - `hx711_scale` (20000.0) ✅
  - Tare mygtukas (line 193) ✅
  - Nulinti funkcionuoja per lambda ✅
- **Svorių skaičiavimai:**
  - `gas_empty_weight` (2.8 kg) ✅
  - `gas_full_weight` (7.8 kg) ✅
  - Likutis % (lines 320-329) ✅
- **Accuracy:** `accuracy_decimals: 0` ✅

#### 4. **BME680** (Oro kokybė)
- **Status:** Implementuotas
- **I2C adresas:** 0x77 ✅ (v15.3 klaida taisyta)
- **Nuskaitymai:**
  - Temperatūra (offset: -4.0°C) ✅
  - Drėgmė ✅
  - Slėpis ✅
  - VOC varža (gas_resistance) ✅
- **Update interval:** 30s ✅
- **Oro kokybės text_sensor** (lines 419-425):
  - "Labai geras" (R > 100kΩ) ✅
  - "Geras" (R > 50kΩ) ✅
  - "Blogas" (R ≤ 50kΩ) ✅
- **Problema:** Nėra BSEC AI-based algoritmo (dokumentavimas sako "jei reikia BSEC")
  - Šis jei yra paprastas 3-lygio VOC filter — **priimtina alfa versijai**

#### 5. **A7670E — GPS/GNSS** (`+CGNSSINFO`)
- **Status:** Implementuotas
- **UART:** GPIO40/41, 115200 baud ✅
- **Palydovų skaičius:**
  - GPS `gps_sats_gps` ✅
  - GLONASS `gps_sats_glonass` ✅
  - BeiDou `gps_sats_bds` ✅
  - Galileo `gps_sats_galileo` ✅
- **Koordinatės:**
  - Latitude/Longitude parsavimas (lines 477-478) ✅
  - Sensor `gps_coords_sensor` (lines 408-416) ✅
  - Accuracy: `accuracy_decimals: 6` ✅
- **Aukštis:**
  - Nuskaitymas iš p[11] (line 480) ✅
  - Sensor `gps_alt_sensor` (lines 351-354) ✅
- **Greitis:**
  - Nuskaitymas iš p[12] su konversija 1.852 (knot→km/h) ✅
  - ⚠️ Nėra atskiro sensor; randasi globalioje `gps_speed` ✅
- **Kryptis (heading):**
  - Nuskaitymas iš p[13] ✅
  - ⚠️ Nėra atskiro sensor; randasi globalioje `gps_heading` ✅
- **Inicijalizacija:**
  - `AT+CGNSSPWR=1` (lines 543) ✅
  - Interval polling (10s) — line 521 ✅
- **Error recovery:**
  - `gps_error_count` tracker ✅
  - Auto-recovery script (lines 547-553) ✅

#### 6. **A7670E — GSM/SMS**
- **Status:** Implementuotas
- **Komandos:**
  - `AT+CPIN?` → SIM statusas ✅
  - `AT+CSQ` → Signalas (RSSI) ✅
  - `AT+COPS?` → Operatorius ✅
  - `AT+CMGF=1` → SMS tekstinis režimas ✅
- **Operatoriaus parsing** (lines 448-453) ✅
- **Signalo skaičiavimas** (lines 362-364):
  - CSQ → % (formula: CSQ/31 × 100) ✅
  - Accuracy: `accuracy_decimals: 0` ✅
- **SMS atsakymas:**
  - Trigeris: "Status", "Info", "?" (case-insensitive) ✅
  - 30s throttle (flood protection) ✅
  - SMS tekstas (ASCII, 160 simbolių) — lines 569-572 ✅
- **Problema:** Nėra raw RSSI (dBm) sensor — tik %
  - Turėtų būti: `-113 + CSQ × 2` (dBm) — **NEPILNA**

#### 7. **Junctek KH-F150A — BLE šuntas**
- **Status:** Implementuotas per BLE client (ne RS485!)
- **BLE MAC:** `38:3B:26:79:DB:64` ✅
- **Service UUID:** FFF0, Characteristic: FFF1 ✅
- **Nuskaitymai:**
  - `junc_v` (Įtampa, V) ✅
  - `junc_a` (Srovė, A) ✅
  - `junc_ah_remaining` (Ah likutis) ✅
  - `junc_power` (Galia, W) ✅
  - `junc_temp` (Temperatūra, °C) ✅
- **Protokolo parsing:**
  - Frame markers: 0xBB (pradžia), 0xEE (pabaiga) ✅
  - BCD dekodavimas tensijos (lines 383-385) ✅
  - Registro markeriai: 0xD5 (Ah), 0xD6 (W), 0xD8 (°C) ✅
- **Reconnection:** Auto-reconnect po disconnect ✅
- **⚠️ Problema:** CLAUDE.md specifikacija sako "RS485 Modbus RTU", bet šis yra **BLE**
  - Dokumentavimas: "Jungiamas **tiesiogiai prie šunto laidų** (valdiklio dėžutė sudegė)"
  - Tai reiškia kad HW vėl pasikeitė — dokumentacija neaktuali
  - **Rekomendacija:** Atnaujinti CLAUDE.md apie Junctek BLE integraciją

#### 8. **Web server & dashboard**
- **Serveris:** ESPHome web_server v3 ✅
- **Grupavimas pagal prefiksus:** Dauguma aktuali ✅

---

### ⚠️ NEPILNA

#### 1. **ADXL345 — Domkratų patarimas**

**Spec reikalavimai:**
```
- Domkratų patarimas (text_sensor)
- Skaičiavimas: mm = tan(kampas_rad) × 1800 (VW Crafter ratų tarpas)
- Pavyzdžiai: "Kelti K +42mm", "Kelti D +18mm", "Lygu" (< 0.5°)
```

**Esamoje v15.4:**
- ❌ Nėra text_sensor su domkratų patarimu
- Roll/Pitch sensoriūs yra, bet patarimo logika nėra

**Rekomendacija:**

```yaml
text_sensor:
  - platform: template
    name: "Pokrypis | Domkratu patarimas"
    id: jack_advice
    update_interval: 1s
    lambda: |-
      float roll = id(roll_sensor).state;
      float pitch = id(pitch_sensor).state;
      if (isnan(roll) || isnan(pitch)) return std::string("Matavimas...");
      
      float wheelbase_mm = 1800.0f;
      float roll_mm = tanf(roll * M_PI / 180.0f) * wheelbase_mm;
      float pitch_mm = tanf(pitch * M_PI / 180.0f) * wheelbase_mm;
      
      char buf[64];
      if (fabs(roll) < 0.5f && fabs(pitch) < 0.5f) {
        snprintf(buf, sizeof(buf), "Lygu");
      } else {
        snprintf(buf, sizeof(buf), "%s%+.0fmm %s%+.0fmm",
          roll > 0.5f ? "K+" : roll < -0.5f ? "D" : "",
          fabs(roll_mm), pitch > 0.5f ? "P+" : pitch < -0.5f ? "G" : "",
          fabs(pitch_mm));
      }
      return std::string(buf);
```

#### 2. **GPS — Fix statusas (text)**

**Spec reikalavimai:**
```
- Fix statusas (text_sensor: "Nėra" / "2D" / "3D")
```

**Esamoje v15.4:**
- `gps_fix_mode` (global int, 0-3) egzistuoja
- ❌ Nėra text_sensor, kuris rodytu "2D" arba "3D"

**Rekomendacija:**

```yaml
text_sensor:
  - platform: template
    name: "GPS | Fix statusas"
    update_interval: 5s
    lambda: |-
      int fm = id(gps_fix_mode);
      if (fm == 0 || fm == 1) return std::string("Nėra");
      if (fm == 2) return std::string("2D");
      if (fm == 3) return std::string("3D");
      return std::string("Neznoma");
```

#### 3. **BME680 — Aukščio skaičiavimas iš slėgio**

**Esamoje v15.4:**
- Yra `pressure_altitude` (lines 272-280) — **skaičiuojama iš BME680 slėpio** ✅

**Problema:** Sis aukštis ≠ GPS aukštis. Šis yra barometrinis aukštis iš slėpio (±10-30m paklaida).
- **Priimtina reikšme — nereikalinga korekcija**

#### 4. **Dashboard prefiksai — "Resursai" konsolidavimas**

**Spec sako:**
```
| Prefiksas | Grupė |
|-----------|-------|
| `Resursai \|` | Vanduo + dujos su kalibravimo mygtukais |
```

**Esamoje v15.4:**
- "Vanduo |" (lines 202, 339)
- "Dujos |" (lines 193, 320)
- ❌ Turėtų būti "Resursai |"

**Rekomendacija:** Pervadinti visus prefiksus:
```yaml
name: "Resursai | Vanduo lygis %"
name: "Resursai | Vanduo bakas (L)"
name: "Resursai | Dujos likutis %"
name: "Resursai | Tare"
```

#### 5. **BMP180 vs BME680 slėpio prognozė**

**Spec sako:**
```
- BMP180 — slėpis, temperatūra, aukštis, orų prognozė
- 7 slėpio reikšmių istorija kas 30 min → 3h tendencija
```

**Esamoje v15.4:**
- ❌ **BMP180 nėra** — tik BME680
- ❌ **Prognozės logika nėra** — nėra 7-taškės slėpio istorijos

**Problema:** BME680 rodo tik esamą slėpį, o prognozei reikalinga 30 min istorija.

**Rekomendacija:**
- **Opcija A:** Laikyti tik BME680 ir praleisti prognocę (alfa etapo supaprastinimas)
- **Opcija B:** Implementuoti 7-reikšmių buffer'į su 30 min intervalais

---

### ❌ TRŪKSTA / KLAIDA

#### 1. **BMP180** — Pilnai nėra!

**Spec reikalavimai:**
- Komponentas: bmp085 (ESPHome)
- I2C adresas: 0x77
- Nuskaitymai: temperatūra, slėpis, aukštis, orų prognozė

**Esamoje v15.4:**
- ❌ Visai nėra šio komponento kode

**Atpasakojimas:** CLAUDE.md žymi BMP180 ir BMP085 (ESPHome naudoja `bmp085` komponentą).
Jei šis jutiklis yra hardware, jis **turi būti pridėtas**.

**Rekomendacija:**

```yaml
sensor:
  - platform: bmp085
    i2c_id: i2c_bus
    address: 0x77
    update_interval: 30s
    temperature:
      name: "Aplinka | Temperatura (BMP180)"
      id: bmp_temp
      accuracy_decimals: 1
    pressure:
      name: "Aplinka | Slegis (BMP180)"
      id: bmp_pressure
      accuracy_decimals: 1
    altitude:
      name: "Aplinka | Aukstis (BMP180)"
      id: bmp_altitude
      accuracy_decimals: 0
```

**⚠️ Tačiau!** Jei BME680 0x77 jau užimtas — reikalinga žmonių aparatūra. Patikrinkite:
- Ar BME680 SDO → GND (0x76) ar prijungtas kitur (0x77)?
- Ar BMP180 egzistuoja KH868 plokštėje?

#### 2. **PCF8574 0x21 — TURĖTŲ BŪTI PAŠALINTAS** (Klaida)

**CLAUDE.md:**
> "220V aptikimas: Iškeltas į atskirą ESP32 — PCF8574 0x21 šiame projekte nenaudojamas."

**Esamoje v15.4 (linijos 67-68):**
```yaml
  - id: pcf_in1
    address: 0x21  ← ❌ NETURI BŪTI ČIONAI
```

**Rekomendacija:** Pašalinti šias 3 linijas.

#### 3. **8 šviesos (OUT1-8) ir 8 jungikliai (DIN1-8) — VISAI NĖRA**

**Spec reikalavimai:**
```
- 8 šviesos — MOSFET sink (inverted: true jei reikia) — OUT1–8 = sviesa_1…sviesa_8
- 8 fiziniai jungikliai — optoisoliuoti, GND aktyvacija — DIN1–8 = jungiklis_1…jungiklis_8
- Kiekvienas jungiklis_N sinchronizuotas su sviesa_N
```

**Esamoje v15.4:**
- ❌ Yra PCF8574 konfigūracija (0x24, 0x25, 0x21 ← klaida, 0x22)
- ❌ Bet **nėra jokios `light:` arba `switch:` konfigūracijos**

**Rekomendacija:** Pridėti 8 output + 8 input:

```yaml
output:
  - platform: gpio
    id: light_1_gpio
    pin:
      pcf8574: pcf_out1
      number: 0
      mode: OUTPUT
      inverted: true

light:
  - platform: binary
    id: sviesa_1
    name: "Šviesos | Šviesa 1"
    output: light_1_gpio
    # ... repeat for sviesa_2 .. sviesa_8

switch:
  - platform: gpio
    id: switch_1_gpio
    name: "Šviesos | Jungiklis 1"
    pin:
      pcf8574: pcf_in1  # arba pcf_in2
      number: 0
      mode: INPUT
      inverted: true  # GND aktyvacija
    # ... repeat for jungiklis_2 .. jungiklis_8
```

#### 4. **AM2301** (1-Wire temperatūra/drėgmė) — NĖRA

**Spec sako:** "atsarginė" — GPIO39

**Esamoje v15.4:**
- ❌ Nėra šio komponento

**Rekomendacija:** Jei yra hardware — pridėti. Jei ne — dokumentuoti atidėjimą.

```yaml
sensor:
  - platform: dht
    pin: GPIO39
    model: AM2301
    temperature:
      name: "Aplinka | Temperatura (AM2301 - atsarginė)"
    humidity:
      name: "Aplinka | Dregme (AM2301 - atsarginė)"
```

#### 5. **SSD1306 OLED** (I2C 0x3C) — NĖRA

**Spec:** "lizdas E"

**Esamoje v15.4:**
- ❌ Nėra šio komponento

**Rekomendacija:** Jei yra hardware — pridėti. Gali rodyti pagrindinės informacijos:

```yaml
display:
  - platform: ssd1306_i2c
    i2c_id: i2c_bus
    address: 0x3C
    pages:
      - id: page_main
        lambda: |-
          it.print(0, 0, id(default_font), "GPS: %s", 
            id(gps_fix) ? "3D OK" : "NO FIX");
          it.print(0, 10, id(default_font), "Bat: %.1fV", 
            id(junc_v).state);
```

#### 6. **Renogy PRO 100Ah LiFePO4** — BLE nėra implementuota

**Spec sako:**
> "Reikia tyrimo ... ar KC868 ESP32-S3 palaiko BLE ... Jei nerealus šiame etape — dokumentuoti ir atidėti"

**Esamoje v15.4:**
- ❌ Nėra Renogy LiFePO4 BLE kodo

**Status:** Junctek BLE jau veikia → Renogy BLE gali būti kitas priority.

#### 7. **Renogy DCC50S kroviklis** — BLE nėra implementuota

**Spec sako:** Tą pačia logika kaip Renogy LiFePO4

**Esamoje v15.4:**
- ❌ Nėra kodo

---

## Kompiliavimo statusas & užuominatomi trigeriai

### Validacija (esphome config)

```bash
esphome config kempervanas_v15.yaml
```

**Numatyti trigeriai (žinomi):**
1. PCF8574 0x21 — gali sukelti I2C conflict warning
2. BME680 0x77 vs BMP180 0x77 — jei BMP180 yra hardware
3. Nėra light/switch komponentų PCF8574 OUT/IN

### Kompiliavimas (esphome compile)

```bash
esphome compile kempervanas_v15.yaml
```

**Tikėtinas rezultatas:** 
- ✅ Kompiliuosis (turint tinkias bibliotetas)
- ⚠️ Gali būti warnings apie nepalygintas I2C adresas
- ⚠️ BLE client gali reikalauti BLE_CAP žymos

---

## Optimizavimo pasiūlymai (prioritetai)

### 🔴 Kritiniai (turi būti taisyta)
1. **Pašalinti PCF8574 0x21** — CLAUDE.md draudžia
2. **Pridėti domkratų patarimo text_sensor** — užbaigtam UI
3. **Patvirtinti BME680 vs BMP180 adresas** — 0x77 konflikt risika

### 🟡 Reikalingi (specifikacijos reikalavimas)
1. **Pridėti 8 šviesos + 8 jungikliai** (light + switch)
2. **Pridėti GPS fix text_sensor** ("2D"/"3D"/"Nėra")
3. **Atnaujinti dashboard prefiksus** — "Vanduo"|"Dujos" → "Resursai|"
4. **Pridėti raw RSSI sensor** (dBm, ne tik %)

### 🟢 Patobulinimas (nice-to-have)
1. **VL53L0X median filtras** — window_size: 5
2. **BMP180 integracija** (jei yra HW) arba tik BME680
3. **Orų prognozė** (7 slėpio taškų buffer) — sudėtinga
4. **AM2301 atsarginė** (jei yra HW)
5. **SSD1306 OLED** (jei yra HW)
6. **Renogy BLE** (atskira iteracija)

---

## Pasiūlymai kodo stiliui

### ✅ Geras stiliaus laikymasis
- Lietuvių komentarai YAML ✅
- `restore_value: true` kalibraciniam duomenimui ✅
- `accuracy_decimals` naudojimas tiksliam kontrolei ✅
- Unikalūs `id:` lauko sensoriams ✅
- SMS ASCII (be lietuviškų raidžių) ✅

### ⚠️ Pasiūlymai
1. **Sensorių grupavimas** pagal funkciją (GPS, Energija, Aplinka, Resursai) — naudoti prefiksus **visur**
2. **Lambda funkcijų dokumentavimas** — pridėti comment'us sudėtingesnems
3. **Error handling** — pridėti graceful defaults NaN reikšmės vietoj nesąmonės
4. **Timeout'ai** — SMS atsakymo 30s throttle ✅ gerai, bet reikalingas GPS recovery timeout

---

## 🎯 Refactoring v16 — Pakeitimai

### ✅ ŠALINTOS dalys
- ❌ **BMP180** — naudojame tik BME680 duomenis
- ❌ **SSD1306 OLED** — per šį etapą nereikalingas
- ❌ **PCF8574 0x21** — iškeltas į atskirą ESP32 (220V aptikimas)
- ❌ **Renogy BLE** — tik Junctek šuntas (energija)
- ❌ **8 šviesos/jungikliai** — atidėti vėlesniam etapui (PCF8574 OUT/IN)
- ❌ **RS485 Modbus** — Junctek dabar per BLE tik

### ✅ PRIDĖTOS dalys
1. ✅ **ADXL345 domkratų patarimas** — text_sensor su mm rekomendacijomis
2. ✅ **GPS fix statusas** — text_sensor ("Nėra"/"2D"/"3D")
3. ✅ **BME680 orų prognozė** — 7 taškų slėpio buffer (30 min intervalo), 3.5h tendencija
4. ✅ **Dashboard prefiksai** — "Vanduo|"/"Dujos|" → "Resursai|"
5. ✅ **RSSI raw sensor** — dBm (ne tik %)
6. ✅ **VL53L0X median filtras** — window_size: 5
7. ✅ **Sensorių grupavimas** — "Energija|", "GPS|", "Pokrypis|", "Aplinka|", "Resursai|"
8. ✅ **Lambda dokumentavimas** — detalūs komentarai sudėtingose operacijose
9. ✅ **Error handling** — NaN defaults, validacijos checkiai
10. ✅ **GPS recovery timeout** — 30s interval'as su auto-recovery

### ✅ ATNAUJINTOS lambdos
| Sritis | Pakeitimas |
|--------|-----------|
| **UART parsing** | Detalus komentarai kiekvienai AT komandai |
| **Junctek BLE** | BCD dekodavimas su dokumentacija |
| **ADXL345** | Roll/Pitch skaičiavimai su atan2 dokumentacija |
| **Slėpio buffer** | 30 min interval, 7 taškų saugojimas |
| **SMS atsakymas** | NaN safety checks su default 0.0f |
| **GPS error** | Auto-recovery su 30s delay |

---

## Suvestinis lentelė: Komponentų standartas (v16)

| Komponentas | Specifikacija | v16 statusas | Reikšmė |
|-------------|--------------|-------------|--------|
| GPS CGNSSINFO | ✅ Required | ✅ Pilnai + dokumentacija | Koordinatės, palydovai, fix |
| **GPS Fix text** | ✅ Required | ✅ **PRIDĖTA** | "2D"/"3D"/"Nėra" |
| GSM/SMS | ✅ Required | ✅ Pilnai + AT komandos docs | Status atsakymas |
| **RSSI (dBm)** | ✅ Required | ✅ **PRIDĖTA** | dBm = -113 + CSQ×2 |
| ADXL345 pokrypis | ✅ Required | ✅ Pilnai (±atan2) | Roll/Pitch kampai |
| **Domkratai patarimas** | ✅ Required | ✅ **PRIDĖTA** | mm rekomendacijos |
| **BMP180 prognozė** | ✅ Required | ✅ **Pakeista į BME680** | 7 taškų buffer, 3.5h trend |
| BME680 oro kokybė | ✅ Required | ✅ Pilnai + prognozė | VOC + slėpio tendencija |
| VL53L0X vanduo | ✅ Required | ✅ Pilnai + **median** | cm → % (window_size:5) |
| HX711 dujos | ✅ Required | ✅ Pilnai | kg → % |
| Junctek BLE | ✅ Required | ✅ **BLE tik** (RS485 šalintas) | V, A, Ah, W, °C |
| 8 šviesos/jungikliai | ✅ Required | ⏳ **Atidėta v17** | PCF8574 OUT/IN kai paruoštos |
| AM2301 atsarginė | ⚠️ Optional | ⏳ **Atidėta** | 1-Wire, GPIO39 |
| SSD1306 OLED | ⚠️ Optional | ❌ **Šalinta** | web_server tenkina |
| Renogy BLE | ⚠️ Research | ❌ **Šalinta** | Tik Junctek |

---

## Kitos pastabos

### 1. **UART RX buffer pagrinda**
Line 59: `rx_buffer_size: 2048` — gerai. GPS + GSM parsing reikalauja greito nuskaitymo.

### 2. **Boot sekvencija**
Lines 12-20:
```yaml
on_boot:
  priority: 200
  then:
    - delay: 200ms
    - lambda: |-
        uint8_t data[2] = {0x2D, 0x08};
        id(i2c_bus).write(0x53, data, 2, true);
    - delay: 500ms
    - lambda: 'id(a7670e_init).execute();'
```
**Status:** ✅ Teisinga seka. ADXL345 measurement mode → A7670E init.

### 3. **Interval valdymas**
- 10s → AT komandos (GPS, GSM) ✅
- 30s → GPS recovery check ✅
- 5s → HX711, VL53L0X ✅
- 1s → ADXL345 ✅

**Problema:** UART nuskaitymas yra 100ms (gerai), bet jei 10s interval'o AT komanda reikalauja 1.5s delay,
tada 10s × 4 komandos = 40s ciklus. Tai gali sukelti GPS "neatsako" jausmus.

**Rekomendacija:** Sumažinti interval'ą į 15-20s arba optimizuoti komandų sekavimą (parallel vietoj sequential).

### 4. **Junctek BLE auto-reconnect**
Lines 76-79:
```yaml
on_disconnect:
  then:
    - delay: 2s
    - ble_client.connect: junctek_shunt
```
**Status:** ✅ Gerai. BLE jungčiai per fragilios, automatic reconnect yra naudingas.

---

## Apibendrinimas

**V15.4 statusas:**
- ✅ **Kompiliuojasi ir veikia 70% hardwaro**
- ⚠️ **NEPILNA 20% — reikalingi UI sensoriųi (domkratai, GPS fix text, RSSI dBm)**
- ❌ **TRŪKSTA 10% — 8 šviesos/jungikliai, AM2301, SSD1306, Renogy BLE**
- 🔴 **KLAIDA: PCF8574 0x21 turėtų būti pašalinta**

**Rekomenduojama tolimų veiksmų planas:**
1. **Šis pull:** Pašalinti 0x21, pridėti domkratų/GPS fix text
2. **PR+1:** Implementuoti 8 šviesos/jungikliai per PCF8574
3. **PR+2:** BMP180 (jei HW egzistuoja) + prognozė
4. **PR+3:** Renogy BLE tyrimas + implementacija (atskira iteracija)

